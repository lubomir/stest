#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "list.h"
#include "test.h"

int run_program(const char *cmd)
{
    pid_t child;
    int status;

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* Child */
        execl(cmd, cmd, NULL);
        perror("exec");
        exit(EXIT_FAILURE);
    }                           /* Parent */
    wait(&status);
    return WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status);
}

void free_cb(void * t)
{
    test_free((Test *) t);
}

void test_cb(void *_test, void *data)
{
    Test *test = (Test *) _test;
    printf("Test %s:\n", test->name);
}

int main(int argc, char *argv[])
{
    struct stat info;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s PROGRAM\n", argv[0]);
        return 1;
    }

    List *tests = test_load_from_dir("tests");
    list_foreach(tests, test_cb, NULL);
    list_destroy(tests, free_cb);
    return 0;

    if (stat(argv[1], &info) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    if (info.st_mode & S_IXUSR != S_IXUSR) {
        fprintf(stderr, "Can not execute '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (run_program(argv[1]) == 0) {
        printf("OK\n");
    } else {
        printf("Bad\n");
    }

    return 0;
}
