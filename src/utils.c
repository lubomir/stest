#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"

int number_sort_wrap(const struct dirent **entry1, const struct dirent **entry2)
{
    return number_sort((*entry1)->d_name, (*entry2)->d_name);
}

int number_sort(const char *name1, const char *name2)
{
    unsigned int num1, num2;

    sscanf(name1, "%u", &num1);
    sscanf(name2, "%u", &num2);

    return num1 - num2;
}

int filter_tests_wrap(const struct dirent *entry)
{
    return filter_tests(entry->d_name);
}

int filter_tests(const char *name)
{
    int digits = 0;

    while (*name && isdigit(*name)) {
        digits++;
        name++;
    }

    return digits;
}

unsigned int count_lines(const char *buffer, int len)
{
    unsigned int num = 0;
    while ((len--) > 0) if (*(buffer++) == '\n') num++;
    return num;
}

void print_color(const char *color, const char *str)
{
    if (isatty(STDOUT_FILENO)) {
        printf("%s%s%s", color, str, NORMAL);
    } else {
        printf("%s", str);
    }
}

TestPart get_test_part(const char *filename)
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
