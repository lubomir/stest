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
    if (files_num < 0) return NULL;

    for (i = 0; i < files_num;) {
        dot = strchr(entries[i]->d_name, '.');
        if (dot == NULL) {
            free(entries[i]);
            continue;
        }
        test = malloc(sizeof(Test));
        test->dir = strdup(dir);
        test->parts = 0;
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
        free(test->dir);
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
    return fd;
}

char ** test_get_args(Test *test, size_t *count)
{
    char **args = NULL;
    FILE *fh;
    char *path, *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (!FLAG_SET(test->parts, TEST_ARGS)) {
        args = malloc(sizeof(char*));
        args[0] = NULL;
        *count = 1;
        return args;
    }

    path = get_filepath(test->dir, test->name, EXT_ARGS);
    fh = fopen(path, "r");
    if (fh == NULL) {
        perror("Can not open arguments file");
        goto out1;
    }
    read = getline(&line, &len, fh);
    if (read < 0) {
        fprintf(stderr, "Can not read arguments\n");
        goto out2;
    }
    args = parse_args(line, count);

    free(line);
out2:
    fclose(fh);
out1:
    free(path);
    return args;
}

int test_get_exit_code(Test *test)
{
    FILE *fh;
    int code = -1;
    char *path;

    path = get_filepath(test->dir, test->name, EXT_RETVAL);
    fh = fopen(path, "r");
    if (fh == NULL) goto out;
    fscanf(fh, " %d", &code);
    fclose(fh);
out:
    free(path);
    return code;
}
