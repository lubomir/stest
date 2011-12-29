#define _POSIX_C_SOURCE 200809L

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
