#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "outputqueue.h"
#include "test.h"
#include "testcontext.h"
#include "utils.h"

struct test_context_t {
    char *cmd;
    char *dir;
    int use_valgrind;
    OQueue *logs;
    unsigned int test_num;
    unsigned int check_num;
    unsigned int check_failed;
    unsigned int crashed;
    unsigned int skipped;
    VerbosityMode verbose;
};

TestContext * test_context_new(const char *cmd, const char *dir, int mem)
{
    TestContext *tc = calloc(sizeof(TestContext), 1);
    tc->cmd = strdup(cmd);
    tc->verbose = MODE_NORMAL;
    tc->dir = strdup(dir);
    tc->logs = oqueue_new();
    tc->use_valgrind = mem;
    return tc;
}

void test_context_free(TestContext *tc)
{
    if (tc) {
        free(tc->cmd);
        free(tc->dir);
        oqueue_free(tc->logs);
    }
    free(tc);
}

/**
 * Return 1 if test context is to set to be totally quiet.
 */
#define TC_IS_QUIET(tc) (tc->verbose == MODE_QUIET)

/**
 * Check if condition holds and print appropriate message to stdout.
 *
 * @param tc    test context
 * @param t     test run
 * @param cond  condition to check
 * @param fmt   printf-style formatting string with error message
 * @return 0 if test passed, 1 otherwise
 */
static int
test_context_handle_result(TestContext *tc,
                           Test *test,
                           int cond,
                           const char *fmt,
                           ...)
{
    if (cond) {
        if (!TC_IS_QUIET(tc)) print_color(GREEN, ".");
        return 0;
    }
    va_list args;
    va_start(args, fmt);
    if (!TC_IS_QUIET(tc)) print_color(RED, "F");
    oqueue_pushf(tc->logs, "Test %s%s%s failed:\n", BOLD, test->name, NORMAL);
    oqueue_pushvf(tc->logs, fmt, args);
    return 1;
}

/**
 * Check if exit status of the program tested matches the expected one.
 *
 * @param tc    test context
 * @param t     test run
 * @param status    status returned by wait()
 * @return 0 if test passed, 1 otherwise
 */
static int
test_context_check_return_code(TestContext *tc, Test *test, int status)
{
    char *filename;
    FILE *fh;
    int expected, actual;

    expected = test_get_exit_code(test);
    if (expected < 0) exit(EXIT_FAILURE);

    actual = WEXITSTATUS(status);
    return test_context_handle_result(tc, test, expected == actual,
            "expected exit code %d, got %d\n\n", expected, actual);
}

/**
 * Check if given output matches the expected one.
 *
 * @param tc    test context
 * @param t     test run
 * @param fname file with actual output of the program
 * @param ext   extension specifying type of check to perform
 * @return 0 if test passed, 1 otherwise
 */
static int
test_context_check_output_file(TestContext *tc,
                               Test *t,
                               const char *fname,
                               const char *ext)
{
    char *expected;
    pid_t child;
    int status, res;
    int mypipe[2];
    size_t line_num = 0;

    if (pipe(mypipe) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    expected = get_filepath(tc->dir, t->name, ext);

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* child */
        close(mypipe[PIPE_READ]);
        dup2(mypipe[PIPE_WRITE], STDOUT_FILENO);
        execlp("diff",
                "diff", "-u", "-d", "--label=expected", "--label=actual",
                expected, fname, NULL);
        perror("exec");
        exit(EXIT_FAILURE);
    }
    free(expected);
    close(mypipe[PIPE_WRITE]);
    wait(&status);
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "Diff failed\n");
        exit(EXIT_FAILURE);
    }

    res = test_context_handle_result(tc, t,
            WEXITSTATUS(status) == 0, "std%s differs", ext);
    if (res != 0) {             /* checking failed */
        if (tc->verbose == MODE_VERBOSE) {
            /* Verbose means copying diff */
            oqueue_push(tc->logs, " - diff follows:\n");
            oqueue_copy_from_fd(tc->logs, mypipe[PIPE_READ]);
            oqueue_push(tc->logs, "\n");
        } else {                /* Otherwise only print how big the diff is */
            line_num = count_lines_on_fd(mypipe[PIPE_READ]);
            oqueue_pushf(tc->logs, " - diff has %zu lines\n\n", line_num - 2);
        }
    }
    close(mypipe[PIPE_READ]);
    return res;
}

/**
 * Check if a test should be performed and optionally perform it. Please note
 * that checking function is only called if the test should be performed.
 * First and second argument of checking function should be test context and
 * test, respectively.
 *
 * @param tc    test context
 * @param t     test
 * @param part  part of a test
 * @param f     function that performs the check
 * @param ...   additional arguments to checking function
 */
#define test_context_check(tc, t, part, f, ...)  do {   \
    if (FLAG_SET(t->parts, part)) {                     \
        tc->check_num++;                                \
        tc->check_failed += f(tc, t, __VA_ARGS__);      \
    } } while (0)

/**
 * Check outputs of a program and update counters accordingly.
 *
 * @param tc    test context
 * @param test  test run
 * @param out_file  where stdout was stored
 * @param err_file  where stderr was stored
 * @param status    status received from wait()
 */
static void
test_context_analyze_test_run(TestContext *tc,
                              Test *test,
                              const char *out_file,
                              const char *err_file,
                              int status)
{
    tc->test_num++;
    if (!WIFEXITED(status)) {
        tc->crashed++;
        if (!TC_IS_QUIET(tc)) print_color(RED, "C");
        if (tc->verbose == MODE_VERBOSE) {
            oqueue_pushf(tc->logs, "Crash in %s%s%s: %s (%d)\n\n",
                    BOLD, test->name, NORMAL, strsignal(WTERMSIG(status)),
                    WTERMSIG(status));
        }
        return;
    }

    test_context_check(tc, test, TEST_RETVAL,
            test_context_check_return_code, status);
    test_context_check(tc, test, TEST_OUTPUT,
            test_context_check_output_file, out_file, EXT_OUTPUT);
    test_context_check(tc, test, TEST_ERRORS,
            test_context_check_output_file, err_file, EXT_ERRORS);
}

/**
 * Create temporary files for stdout and stderr, open them and then return
 * their file descriptors. The (char *) arguments will be modified and
 * therefore can not point to string constant.
 *
 * @return whether files were opened
 */
static int
test_context_prepare_outfiles(char *out_file,
                              int *out_fd,
                              char *err_file,
                              int *err_fd)
{
    *out_fd = mkstemp(out_file);
    *err_fd = mkstemp(err_file);
    return *err_fd > 0 && *out_fd > 0;
}

/**
 * Build argument vector to be passed to tested program. First element will be
 * the executed command. This function always returns vector with at least one
 * element.
 *
 * @param tc    test context
 * @param t     test from which to retrieve arguments
 * @return NULL-terminated array of strings or NULL on failure
 */
static char **
test_context_get_args(TestContext *tc, Test *t)
{
    char **args = NULL;
    size_t count = 0;

    args = test_get_args(t, &count);
    if (args == NULL) {
        return NULL;
    }
    args = realloc(args, (count + 1) * sizeof(char *));
    memmove(args+1, args, count * sizeof(char *));
    args[0] = get_command_name(tc->cmd);
    return args;
}

/**
 * Display information about skipped test and increase counter.
 *
 * @param tc    test context
 * @param t     test
 * @param msg   detailed message
 */
static void
test_context_skip(TestContext *tc, Test *t, const char *msg)
{
    print_color(YELLOW, "S");
    if (tc->verbose == MODE_VERBOSE) {
        oqueue_pushf(tc->logs, "Skipping test %s%s%s: %s.\n\n",
                BOLD, t->name, NORMAL, msg);
    }
    tc->skipped++;
}

static void
test_context_execute_test(TestContext *tc, Test *t,
        int in_fd, int out_fd, int err_fd, char **args)
{
    static char *env[] = { "MALLOC_CHECK_=2", NULL };
    int child;

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* Child */
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        dup2(err_fd, STDERR_FILENO);
        execve(tc->cmd, args, env);
        perror("exec");
        exit(EXIT_FAILURE);
    }                           /* Parent */
    close(in_fd);
    close(out_fd);
    close(err_fd);
}

static char *
test_context_execute_test_with_valgrind(TestContext *tc, Test *t,
        int in_fd, int out_fd, int err_fd, char **args)
{
    int child;
    char **realargs;
    int len = 5, i;
    for (i = 0; args[i] != NULL; i++) len++;
    realargs = calloc(sizeof(char *), len);
    realargs[0] = "valgrind";
    realargs[1] = "--tool=memcheck";
    realargs[2] = calloc(sizeof(char), 128);
    realargs[3] = tc->cmd;
    for (i = 1; args[i] != NULL; i++) realargs[i+2] = args[i];
    char *file = calloc(sizeof(char), 30);
    strcpy(file, "/tmp/stest-memory-XXXXXX");
    int log_fd = mkstemp(file);
    if (log_fd < 0) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }
    close(log_fd);

    snprintf(realargs[2], 128, "--log-file=%s", file);

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        dup2(err_fd, STDERR_FILENO);
        execvp("valgrind", realargs);
        perror("exec");
        exit(EXIT_FAILURE);
    }
    free(realargs[2]);
    free(realargs);
    close(in_fd);
    close(out_fd);
    close(err_fd);
    return file;
}

static void
test_context_analyze_memory(TestContext *tc, Test *t, char *file)
{
    static const char *names[] = { "error", "errors" };
    char *line, c;
    int errors = 0;
    FILE *fh = fopen(file, "r");
    fseek(fh, -10, SEEK_END);
    do {
        c = fgetc(fh);
        fseek(fh, -2, SEEK_CUR);
    } while (c != 'Y');
    fseek(fh, 3, SEEK_CUR);
    fscanf(fh, " %d", &errors);

    tc->check_num++;
    if (errors == 0) {
        print_color(GREEN, ".");
    } else {
        print_color(YELLOW, "M");
        tc->check_failed++;
        oqueue_pushf(tc->logs, "Test %s%s%s failed:\ndetected %d memory %s\n\n",
                BOLD, t->name, NORMAL, errors,
                errors > 1 ? names[1] : names[0]);
    }

    fclose(fh);
    unlink(file);
    free(file);
}

/**
 * Run a test in given context and analyze the results.
 *
 * @param t     test to be run
 * @param tc    test context
 */
static void test_context_run_test(Test *t, TestContext *tc)
{
    int in_fd, out_fd, err_fd, i;
    char out_file[] = "/tmp/stest-stdout-XXXXXX";
    char err_file[] = "/tmp/stest-stderr-XXXXXX";
    pid_t child;
    int status;
    char **args, *file;

    if (!test_context_prepare_outfiles(out_file, &out_fd, err_file, &err_fd)) {
        test_context_skip(tc, t, "can not open temporary files");
        goto out;
    }
    if ((in_fd = test_get_input_fd(t)) < 0) {
        test_context_skip(tc, t, "can not open input file");
        goto out;
    }
    if ((args = test_context_get_args(tc, t)) == NULL) {
        test_context_skip(tc, t, "can not read arguments");
        goto out;
    }

    if (tc->use_valgrind) {
        file = test_context_execute_test_with_valgrind(tc, t, in_fd, out_fd, err_fd, args);
    } else {
        test_context_execute_test(tc, t, in_fd, out_fd, err_fd, args);
    }

    for (i = 0; args[i] != NULL; i++)
        free(args[i]);
    free(args);

    wait(&status);

    test_context_analyze_test_run(tc, t, out_file, err_file, status);
    if (tc->use_valgrind) {
        test_context_analyze_memory(tc, t, file);
    }
out:
    unlink(out_file);
    unlink(err_file);
}

unsigned int
test_context_run_tests(TestContext *tc, List *tests, VerbosityMode verbose)
{
    struct timeval ta, tb, res;
    tc->verbose = verbose;

    gettimeofday(&ta, NULL);
    list_foreach(tests, CBFUNC(test_context_run_test), tc);
    gettimeofday(&tb, NULL);
    timersub(&tb, &ta, &res);

    if (!TC_IS_QUIET(tc)) {
        printf("\n\n");
        oqueue_flush(tc->logs, stdout);
        printf("%u tests, %d crashes, %d skipped, %u checks, %u failed"
               " (%ld.%03ld seconds)\n",
                tc->test_num, tc->crashed, tc->skipped, tc->check_num,
                tc->check_failed, res.tv_sec, res.tv_usec / 1000);
    }
    return tc->check_failed;
}
