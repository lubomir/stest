#include <config.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "outputqueue.h"
#include "test.h"
#include "testcontext.h"
#include "utils.h"

struct test_context_t {
    char *cmd;
    char *diff_opts;
    int use_valgrind;
    OQueue *logs;
    unsigned int test_num;
    unsigned int check_num;
    unsigned int check_failed;
    unsigned int crashed;
    unsigned int skipped;
    VerbosityMode verbose;
};

TestContext * test_context_new(void)
{
    TestContext *tc = calloc(sizeof(TestContext), 1);
    tc->verbose = MODE_NORMAL;
    tc->logs = oqueue_new();
    return tc;
}

int test_context_set_command(TestContext *tc, const char *cmd)
{
    struct stat info;

    if (stat(cmd, &info) == -1) {
        perror("stat");
        return 0;
    }
    if (!FLAG_SET(info.st_mode, S_IXUSR)) {
        fprintf(stderr, "Can not execute '%s'\n", cmd);
        return 0;
    }

    free(tc->cmd);
    tc->cmd = strdup(cmd);
    return 1;
}

void test_context_set_use_valgrind(TestContext *tc)
{
    tc->use_valgrind = 1;
}

void test_context_set_diff_opts(TestContext *tc, const char *opts)
{
    free(tc->diff_opts);
    tc->diff_opts = strdup(opts);
}

void test_context_set_verbosity(TestContext *tc, VerbosityMode verbose)
{
    tc->verbose = verbose;
}

void test_context_free(TestContext *tc)
{
    if (tc) {
        free(tc->cmd);
        oqueue_free(tc->logs);
    }
    free(tc);
}

/**
 * Return 1 if test context is set to be totally quiet.
 */
#define TC_IS_QUIET(tc) (tc->verbose == MODE_QUIET)

/**
 * Wrapper around print_color() utility function that only prints if the test
 * context is not set to be quiet.
 *
 * @param tc    test context
 * @param color requested color
 * @param str   string to be printed
 */
static void test_context_print_color(TestContext *tc, char *color, char *str)
{
    if (!TC_IS_QUIET(tc)) {
        print_color(color, str);
    }
}

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
    va_list args;
    if (cond) {
        test_context_print_color(tc, GREEN, ".");
        return 0;
    }
    va_start(args, fmt);
    test_context_print_color(tc, RED, "F");
    oqueue_pushf(tc->logs, "Test %s failed:\n", str_to_bold(test->name));
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
    int expected, actual;

    expected = test_get_exit_code(test);
    if (expected < 0) exit(EXIT_FAILURE);

    actual = WEXITSTATUS(status);
    return test_context_handle_result(tc, test, expected == actual,
            "expected exit code %d, got %d\n\n", expected, actual);
}

/**
 * Get string array to pass to execvp(3) to invoke diff.
 *
 * @param tc    test context
 * @param orig  first file to pass to diff
 * @param real  second file to pass to diff
 * @return      NULL-terminated array of strings
 */
static char **
test_context_get_diff_cmd(TestContext *tc, const char *orig, const char *real)
{
    const char * const template = "diff -u -d --label=expected --label=actual";
    size_t len;
    char *cmd, **args;

    len = strlen(template) + strlen(orig) + strlen(real) + 10;
    len += tc->diff_opts != NULL ? strlen(tc->diff_opts) : 0;
    cmd = calloc(1, len);
    sprintf(cmd, "%s %s %s %s",
            template,
            tc->diff_opts ? tc->diff_opts : "",
            orig, real);

    args = parse_args(cmd, NULL);

    free(cmd);

    return args;
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

    if (pipe(mypipe) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    expected = test_get_file_for_ext(t, ext);

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* child */
        char **args = test_context_get_diff_cmd(tc, expected, fname);
        close(mypipe[PIPE_READ]);
        dup2(mypipe[PIPE_WRITE], STDOUT_FILENO);
        execvp("diff", args);
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
            size_t line_num = count_lines_on_fd(mypipe[PIPE_READ]);
            oqueue_pushf(tc->logs, " - diff has %zu lines\n\n", line_num - 2);
        }
    }
    close(mypipe[PIPE_READ]);
    return res;
}

static void
test_context_analyze_memory(TestContext *tc, Test *t, char *file)
{
    int errors, contexts;
    FILE *fh = fopen(file, "r");
    if (!get_num_errors(fh, &errors, &contexts)) {
        /* log failure to parse */
        goto out;
    }

    tc->check_num++;
    if (errors == 0) {
        test_context_print_color(tc, GREEN, ".");
    } else {
        tc->check_failed++;
        test_context_print_color(tc, YELLOW, "M");
        if (TC_IS_QUIET(tc)) goto out;
        oqueue_pushf(tc->logs, "Test %s failed:\ndetected %d memory %s in %d contexts\n",
                str_to_bold(t->name), errors,
                errors > 1 ? "error" : "errors",
                contexts);
        if (tc->verbose == MODE_VERBOSE) {
            oqueue_copy_from_valgrind(tc->logs, fh, contexts);
        } else {
            oqueue_push(tc->logs, "\n");
        }
    }

out:
    if (fh) fclose(fh);
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
 * @param mem_file  where memory output was stored
 * @param status    status received from wait()
 */
static void
test_context_analyze_test_run(TestContext *tc,
                              Test *test,
                              const char *out_file,
                              const char *err_file,
                              char *mem_file,
                              int status)
{
    tc->test_num++;
    if (!WIFEXITED(status)) {
        tc->crashed++;
        test_context_print_color(tc, RED, "C");
        if (tc->verbose == MODE_VERBOSE) {
            oqueue_pushf(tc->logs, "Crash in %s: %s (%d)\n\n",
                    str_to_bold(test->name), strsignal(WTERMSIG(status)),
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
    if (tc->use_valgrind) {
        test_context_analyze_memory(tc, test, mem_file);
    }
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
    char **args;
    size_t count = 0;

    args = test_get_args(t, &count);
    if (args == NULL) {
        return NULL;
    }
    args = realloc(args, (count + 1) * sizeof(char *));
    memmove(args+1, args, count * sizeof(char *));
    args[0] = strdup(tc->cmd);
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
    test_context_print_color(tc, YELLOW, "S");
    if (tc->verbose == MODE_VERBOSE) {
        oqueue_pushf(tc->logs, "Skipping test %s: %s.\n\n",
                str_to_bold(t->name), msg);
    }
    tc->skipped++;
}

/**
 * Given a file descriptor with standard input and descriptors for both
 * standard and error output, execute the program as specified by args. The
 * actual command to be executed is extracted from the command-line arguments
 * parameter.
 *
 * @param in_fd     where to read standard input from
 * @param out_fd    where to store standard output
 * @param err_fd    where to store error output
 * @param args      arguments of the program
 */
static void
execute_test(int in_fd, int out_fd, int err_fd, char **args)
{
    static char * const env[] = { "MALLOC_CHECK_=2", NULL };
    int child;

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* Child */
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        dup2(err_fd, STDERR_FILENO);
        execve(args[0], args, env);
        perror("exec");
        exit(EXIT_FAILURE);
    }                           /* Parent */
    close(in_fd);
    close(out_fd);
    close(err_fd);
}

/**
 * Modify array of arguments so as to be passed to valgrind. The returned path
 * points to a file that should be unlinked when it is no longer needed. Caller
 * is responsible for freeing the returned pointer.
 *
 * @param _args pointer to a NULL-terminated array of strings @return filename
 * path to a file where valgrind output will be stored
 */
static char *
prepare_for_valgrind(char ***_args)
{
    char **args = *_args;
    int len = 1, i, fd;
    char *file = strdup("/tmp/stest-memory-XXXXXX");

    for (i = 0; args[i] != NULL; i++) {
        len++;
    }

    args = realloc(args, (4 + len) * sizeof(char *));
    memmove(args + 4, args, len * sizeof(char *));
    args[0] = strdup("/usr/bin/valgrind");
    args[1] = strdup("--tool=memcheck");
    args[2] = strdup("--leak-check=full");
    args[3] = calloc(sizeof(char), 128);

    fd = mkstemp(file);
    if (fd < 0) {
        free(file);
        file = NULL;
        goto out;
    }
    close(fd);

    snprintf(args[3], 128, "--log-file=%s", file);
out:
    *_args = args;
    return file;
}

/**
 * Run a test in given context and analyze the results.
 *
 * @param t     test to be run
 * @param tc    test context
 */
static void test_context_run_test(Test *t, TestContext *tc)
{
    int in_fd, out_fd, err_fd;
    char out_file[] = "/tmp/stest-stdout-XXXXXX";
    char err_file[] = "/tmp/stest-stderr-XXXXXX";
    int status = 0;
    char **args = NULL, *mem_file = NULL;

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
        mem_file = prepare_for_valgrind(&args);
        if (!mem_file) {
            test_context_skip(tc, t, "can not open memory output file");
            goto out;
        }
    }
    execute_test(in_fd, out_fd, err_fd, args);

    wait(&status);

    test_context_analyze_test_run(tc, t, out_file, err_file, mem_file, status);
out:
    str_array_free(args);

    unlink(out_file);
    unlink(err_file);
    if (mem_file) {
        unlink(mem_file);
        free(mem_file);
    }
}

unsigned int
test_context_run_tests(TestContext *tc, List *tests)
{
    struct timeval ta, tb, res;

    gettimeofday(&ta, NULL);
    list_foreach(tests, CBFUNC(test_context_run_test), tc);
    gettimeofday(&tb, NULL);
    time_difference(&tb, &ta, &res);

    if (!TC_IS_QUIET(tc)) {
        printf("\n\n");
        oqueue_flush(tc->logs, stdout);
        printf("%u tests, %u crashes, %u skipped, %u checks, %u failed"
               " (%ld.%03ld seconds)\n",
                tc->test_num, tc->crashed, tc->skipped, tc->check_num,
                tc->check_failed, res.tv_sec, res.tv_usec / 1000);
    }
    return tc->check_failed;
}
