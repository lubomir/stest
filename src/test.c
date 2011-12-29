#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "test.h"

#define return_val_if_fail(x,v) do { if (!(x)) return v; } while (0)

struct test_context_t {
    char *cmd;
    char *dir;
    int logfd[2];
};

enum {
    PIPE_READ,
    PIPE_WRITE
};

/**
 * Compare two filenames based on leading digits.
 */
int number_sort(const struct dirent **entry1, const struct dirent **entry2);

/**
 * Test whether particular filename is a test definition - that is, whether
 * it starts with at least one digit.
 */
int filter_tests(const struct dirent *entry);

/**
 * Find out which test part is stored in given file.
 */
enum TestPart get_test_part(const char *filename);

int number_sort(const struct dirent **entry1, const struct dirent **entry2)
{
    unsigned int num1, num2;

    sscanf((*entry1)->d_name, "%u", &num1);
    sscanf((*entry2)->d_name, "%u", &num2);

    return num1 - num2;
}

int filter_tests(const struct dirent *entry)
{
    int digits = 0;
    const char *name = entry->d_name;

    while (*name && isdigit(*name)) {
        digits++;
        name++;
    }

    return digits > 0;
}

enum TestPart get_test_part(const char *filename)
{
    return_val_if_fail(filename != NULL, TEST_UNKNOWN);
    char *ext = strrchr(filename, '.');
    return_val_if_fail(ext != NULL, TEST_UNKNOWN);
    ext++;

    if (strcmp(ext, "in") == 0)
        return TEST_INPUT;
    if (strcmp(ext, "out") == 0)
        return TEST_OUTPUT;
    if (strcmp(ext, "args") == 0)
        return TEST_ARGS;
    if (strcmp(ext, "err") == 0)
        return TEST_ERRORS;
    if (strcmp(ext, "ret") == 0)
        return TEST_RETVAL;

    return TEST_UNKNOWN;
}

List * test_load_from_dir(const char *dir)
{
    struct dirent **entries;
    int i, files_num;
    List *tests = NULL;
    Test *test;
    char *dot;
    size_t len;
    enum TestPart tp;

    files_num = scandir(dir, &entries, filter_tests, number_sort);

    for (i = 0; i < files_num;) {
        test = malloc(sizeof(Test));
        test->parts = 0;
        dot = strchr(entries[i]->d_name, '.');
        if (dot == NULL) {
            free(entries[i]);
            continue;
        }
        len = dot - entries[i]->d_name;
        test->name = calloc(len + 1, sizeof(char));
        strncpy(test->name, entries[i]->d_name, len);

        while (i < files_num && strncmp(test->name, entries[i]->d_name, len) == 0) {
            tp = get_test_part(entries[i]->d_name);
            if (tp != TEST_UNKNOWN) {
                test->parts |= tp;
            }
            free(entries[i]);
            i++;
        }

        tests = list_prepend(tests, test);
    }

    free(entries);
    return list_reverse(tests);
}

void test_free(Test *test)
{
    if (test) {
        free(test->name);
        free(test);
    }
}

TestContext * test_context_new(const char *cmd, const char *dir)
{
    TestContext *tc = malloc(sizeof(TestContext));
    tc->cmd = strdup(cmd);
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

int test_context_get_fd(TestContext *tc, Test *t)
{
    int fd;
    char input_file[strlen(tc->dir) + strlen(t->name) + 5];
    if ((t->parts & TEST_INPUT) != TEST_INPUT) {
        fd = open("/dev/null", O_RDONLY);
    } else {
        sprintf(input_file, "%s/%s.in", tc->dir, t->name);
        fd = open(input_file, O_RDONLY);
    }
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void check_return_code(TestContext *tc, Test *test, int status)
{
    char filename[strlen(tc->dir) + strlen(test->name) + 5];
    sprintf(filename, "%s/%s.ret", tc->dir, test->name);
    FILE *fh = fopen(filename, "r");
    if (fh == NULL) {
        return;
    }
    int expected, actual;
    fscanf(fh, " %d", &expected);
    fclose(fh);

    actual = WEXITSTATUS(status);
    if (expected == actual) {
        printf(".");
    } else {
        printf("F");
        dprintf(tc->logfd[PIPE_WRITE],
                "Test %s failed:\nexpected exit code %d, got %d\n\n",
                test->name, expected, actual);
    }
}

void analyze_test_run(TestContext *tc, Test *test,
        const char *out_file, const char *err_file, int status)
{
    if (!WIFEXITED(status)) {
        printf("Program crashed\n");
        return;
    }

    if ((test->parts & TEST_RETVAL) == TEST_RETVAL) {
        check_return_code(tc, test, status);
    }
    unlink(out_file);
    unlink(err_file);
}

void run_test(Test *t, TestContext *tc)
{
    int stdin_fd, stdout_fd, stderr_fd;
    stdin_fd = test_context_get_fd(tc, t);
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

    analyze_test_run(tc, t, stdout_file, stderr_file, status);
}

void test_context_run_tests(TestContext *tc, List *tests)
{
    list_foreach(tests, CBFUNC(run_test), tc);
    putchar('\n');
    close(tc->logfd[PIPE_WRITE]);
}

void test_context_flush_messages(TestContext *tc, FILE *fh)
{
    char buffer[1024];
    ssize_t len;
    while ((len = read(tc->logfd[PIPE_READ], buffer, 1024)) > 0) {
        fwrite(buffer, sizeof(char), len, fh);
    }
}
