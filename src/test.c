#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "utils.h"

List * test_load_from_dir(const char *dir)
{
    struct dirent **entries;
    int i, files_num;
    List *tests = NULL;
    Test *test;
    char *dot;
    size_t len;
    TestPart tp;

    files_num = scandir(dir, &entries, filter_tests_wrap, number_sort_wrap);

    for (i = 0; i < files_num;) {
        test = malloc(sizeof(Test));
        test->dir = dir;
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

int test_get_input_fd(Test *test)
{
    int fd;
    char *input_file;
    if (!FLAG_SET(test->parts, TEST_INPUT)) {
        fd = open("/dev/null", O_RDONLY);
    } else {
        input_file = get_filepath(test->dir, test->name, EXT_INPUT);
        fd = open(input_file, O_RDONLY);
        free(input_file);
    }
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}
