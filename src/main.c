#include <config.h>

#include <stdio.h>
#include <sys/stat.h>

#include "list.h"
#include "test.h"
#include "testcontext.h"

int main(int argc, char *argv[])
{
    struct stat info;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s PROGRAM\n", argv[0]);
        return 1;
    }

    List *tests = test_load_from_dir("tests");

    if (stat(argv[1], &info) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    if ((info.st_mode & S_IXUSR) != S_IXUSR) {
        fprintf(stderr, "Can not execute '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    TestContext *tc = test_context_new(argv[1], "tests");
    test_context_run_tests(tc, tests, MODE_VERBOSE);
    list_destroy(tests, DESTROYFUNC(test_free));
    test_context_free(tc);

    return 0;
}
