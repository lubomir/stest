#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test.h"
#include "testcontext.h"
#include "utils.h"

struct test_context_t {
    char *cmd;
    char *dir;
    int logfd[2];
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
    if (pipe(tc->logfd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    return tc;
}

void test_context_free(TestContext *tc)
{
    if (tc) {
        free(tc->cmd);
        free(tc->dir);
        close(tc->logfd[PIPE_READ]);
    }
    free(tc);
}

/**
 * Open file descriptor with input for given test. If test does not specify
 * any input, /dev/null is opened and passed. It is responsibility of caller
 * to close this descriptor.
 *
 * @param tc    test context
 * @param t     test
 * @return file descriptor with new standard input
 */
static int test_context_get_stdin(TestContext *tc, Test *t)
{
    int fd;
    char *input_file;
    if ((t->parts & TEST_INPUT) != TEST_INPUT) {
        fd = open("/dev/null", O_RDONLY);
    } else {
        input_file = get_filepath(tc->dir, t->name, EXT_INPUT);
        fd = open(input_file, O_RDONLY);
        free(input_file);
    }
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
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
    dprintf(tc->logfd[PIPE_WRITE], "Test %s%s%s failed:\n",
            BOLD, test->name, NORMAL);
    vdprintf(tc->logfd[PIPE_WRITE], fmt, args);
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
    char buffer[1024];
    ssize_t len;
    pid_t child;
    int status, res;
    int mypipe[2];
    unsigned int line_num = 0;

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
        if (tc->verbose) {
            dprintf(tc->logfd[PIPE_WRITE], " - diff follows:\n");
        }
        while ((len = read(mypipe[PIPE_READ], buffer, 1024)) > 0) {
            if (tc->verbose) {  /* copy diff to log */
                write(tc->logfd[PIPE_WRITE], buffer, len);
            } else {            /* count number of lines */
                line_num += count_lines(buffer, len);
            }
        }
        if (!tc->verbose) {
            dprintf(tc->logfd[PIPE_WRITE], " - diff has %u lines\n",
                    line_num - 2);
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

    if ((test->parts & TEST_RETVAL) == TEST_RETVAL) {
        tc->check_num++;
        tc->check_failed += test_context_check_return_code(tc, test, status);
    }
    if ((test->parts & TEST_OUTPUT) == TEST_OUTPUT) {
        tc->check_num++;
        tc->check_failed += test_context_check_output_file(tc, test,
                out_file, EXT_OUTPUT);
    }
    if ((test->parts & TEST_ERRORS) == TEST_ERRORS) {
        tc->check_num++;
        tc->check_failed += test_context_check_output_file(tc, test,
                err_file, EXT_ERRORS);
    }
    putchar(' ');
    unlink(out_file);
    unlink(err_file);
}

/**
 * Run a test in given context and analyze the results.
 *
 * @param t     test to be run
 * @param tc    test context
 */
static void test_context_run_test(Test *t, TestContext *tc)
{
    int stdin_fd, stdout_fd, stderr_fd;
    stdin_fd = test_context_get_stdin(tc, t);
    char stdout_file[] = "/tmp/stest-stdout-XXXXXX";
    char stderr_file[] = "/tmp/stest-stderr-XXXXXX";
    stdout_fd = mkstemp(stdout_file);
    if (stdout_fd < 0) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }
    stderr_fd = mkstemp(stderr_file);
    if (stderr_fd < 0) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    pid_t child;
    int status;

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* Child */
        dup2(stdin_fd, STDIN_FILENO);
        dup2(stdout_fd, STDOUT_FILENO);
        dup2(stderr_fd, STDERR_FILENO);
        execl(tc->cmd, tc->cmd, NULL);
        perror("exec");
        exit(EXIT_FAILURE);
    }                           /* Parent */
    close(stdin_fd);
    close(stdout_fd);
    close(stderr_fd);

    wait(&status);

    test_context_analyze_test_run(tc, t, stdout_file, stderr_file, status);
}

void test_context_run_tests(TestContext *tc, List *tests, VerbosityMode verbose)
{
    char buffer[1024];
    ssize_t len;

    tc->verbose = verbose;

    list_foreach(tests, CBFUNC(test_context_run_test), tc);
    close(tc->logfd[PIPE_WRITE]);
    printf("\n\n");

    while ((len = read(tc->logfd[PIPE_READ], buffer, 1024)) > 0) {
        fwrite(buffer, sizeof(char), len, stdout);
    }

    printf("\nRun %u tests, %u checks, %u failed\n",
            tc->test_num, tc->check_num, tc->check_failed);
}
