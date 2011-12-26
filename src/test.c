#include "test.h"

#define return_val_if_fail(x,v) do { if (!(x)) return v; } while (0)

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
    if (strcmp(ext, "errors") == 0)
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

    files_num = scandir(dir, &entries, filter_tests, number_sort);

    for (i = 0; i < files_num; i++) {
        test = malloc(sizeof(Test));
        test->parts = 0;
        dot = strchr(entries[i]->d_name, '.');
        len = dot - entries[i]->d_name;
        test->name = calloc(len + 1, sizeof(char));
        strncpy(test->name, entries[i]->d_name, len);

        while (i < files_num && strncmp(test->name, entries[i]->d_name, len) == 0) {
            test->parts |= get_test_part(entries[i]->d_name);
            free(entries[i]);
            i++;
        }
        i--;

        tests = list_prepend(tests, test);
    }

    free(entries);
    tests = list_reverse(tests);
    return tests;
}

void test_free(Test *test)
{
    if (test) {
        free(test->name);
        free(test);
    }
}
