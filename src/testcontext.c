#include <fcntl.h>
#include <stdio.h>
#include <string.h>
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
    OQueue *logs;
    unsigned int test_num;
    unsigned int check_num;
    unsigned int check_failed;
    VerbosityMode verbose;
};

TestContext * test_context_new(const char *cmd, const char *dir)
{
    TestContext *tc = malloc(sizeof(TestContext));
    tc->cmd = strdup(cmd);
    tc->test_num = 0;
    tc->check_num = 0;
    tc->check_failed = 0;
    tc->verbose = MODE_QUIET;
    tc->dir = strdup(dir);
    tc->logs = oqueue_new();
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
        print_color(GREEN, ".");
        return 0;
    }
    va_list args;
    va_start(args, fmt);
    print_color(RED, "F");
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

    filename = get_filepath(tc->dir, test->name, EXT_RETVAL);
    fh = fopen(filename, "r");
    free(filename);
    if (fh == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fscanf(fh, " %d", &expected);
    fclose(fh);

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
        if (tc->verbose) {      /* Verbose means copying diff */
            oqueue_push(tc->logs, " - diff follows:\n");
            oqueue_copy_from_fd(tc->logs, mypipe[PIPE_READ]);
        } else {                /* Otherwise only print how big the diff is */
            line_num = count_lines_on_fd(mypipe[PIPE_READ]);
            oqueue_pushf(tc->logs, " - diff has %zu lines\n", line_num - 2);
        }
    }
    close(mypipe[PIPE_READ]);
    return res;
}

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
        printf("Program crashed\n");
        return;
    }

    if (FLAG_SET(test->parts, TEST_RETVAL)) {
        tc->check_num++;
        tc->check_failed += test_context_check_return_code(tc, test, status);
    }
    if (FLAG_SET(test->parts, TEST_OUTPUT)) {
        tc->check_num++;
        tc->check_failed += test_context_check_output_file(tc, test,
                out_file, EXT_OUTPUT);
    }
    if (FLAG_SET(test->parts, TEST_ERRORS)) {
        tc->check_num++;
        tc->check_failed += test_context_check_output_file(tc, test,
                err_file, EXT_ERRORS);
    }
    putchar(' ');
    unlink(out_file);
    unlink(err_file);
}

/**
 * Create temporary files for stdout and stderr, open them and then return
 * their file descriptors. The (char *) arguments will be modified and
 * therefore can not point to string constant.
 */
static void
test_context_prepare_outfiles(char *out_file,
                              int *out_fd,
                              char *err_file,
                              int *err_fd)
{
    *out_fd = mkstemp(out_file);
    *err_fd = mkstemp(err_file);
    if (*err_fd < 0 || *out_fd < 0) {
        perror("Can not create temporary file");
        exit(EXIT_FAILURE);
    }
}

/**
 * Build argument vector to be passed to tested program. First element will be
 * the executed command. This function always returns vector with at least one
 * element.
 *
 * @param tc    test context
 * @param t     test from which to retrieve arguments
 * @return NULL terminated array of strings
 */
static char **
test_context_get_args(TestContext *tc, Test *t)
{
    char **args = NULL;
    char *line = NULL, *fname;
    size_t len = 0, count;
    FILE *fh;

    if ((t->parts & TEST_ARGS) == TEST_ARGS) {
        fname = get_filepath(tc->dir, t->name, EXT_ARGS);
        fh = fopen(fname, "r");
        if (fh == NULL) {
            perror("Can not open args file");
            exit(EXIT_FAILURE);
        }
        if (getline(&line, &len, fh) < 0) {
            fprintf(stderr, "Can not read arguments from %s\n", fname);
            exit(EXIT_FAILURE);
        }
        fclose(fh);
        free(fname);
        args = parse_args(line, &count);
        if (args == NULL) {
            fprintf(stderr, "Can not parse arguments for test %s\n", t->name);
            exit(EXIT_FAILURE);
        }

        args = realloc(args, (count + 1) * sizeof(char *));
        memmove(args+1, args, count * sizeof(char *));
    } else {
        args = calloc(2, sizeof(char **));
        args[1] = NULL;
    }
    free(line);
    args[0] = strdup(tc->cmd);
    return args;
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
    char **args;

    test_context_prepare_outfiles(out_file, &out_fd, err_file, &err_fd);
    in_fd = test_get_input_fd(t);
    args = test_context_get_args(tc, t);

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* Child */
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        dup2(err_fd, STDERR_FILENO);
        execv(tc->cmd, args);
        perror("exec");
        exit(EXIT_FAILURE);
    }                           /* Parent */
    close(in_fd);
    close(out_fd);
    close(err_fd);

    for (i = 0; args[i] != NULL; i++)
        free(args[i]);
    free(args);

    wait(&status);

    test_context_analyze_test_run(tc, t, out_file, err_file, status);
}

void test_context_run_tests(TestContext *tc, List *tests, VerbosityMode verbose)
{
    tc->verbose = verbose;

    list_foreach(tests, CBFUNC(test_context_run_test), tc);
    printf("\n\n");

    oqueue_flush(tc->logs, stdout);

    printf("\nRun %u tests, %u checks, %u failed\n",
            tc->test_num, tc->check_num, tc->check_failed);
}
