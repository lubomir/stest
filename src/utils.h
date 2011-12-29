#ifndef UTILS_H
#define UTILS_H

#include <config.h>

#include <dirent.h>

/**
 * Compare two filenames based on leading digits.
 */
int number_sort(const char *name1, const char *name2);

/**
 * Wrapper around number_sort() that extracts filenames from pointers to
 * pointers to dirent structure.
 */
int number_sort_wrap(const struct dirent **entry1, const struct dirent **entry2);

/**
 * Test whether particular filename is a test definition - that is, whether
 * it starts with at least one digit.
 *
 * @param name  filename to be checked
 * @return number of digits at the start
 */
int filter_tests(const char *name);

/**
 * Wrapper around filter_tests that takes pointer to dirent structure.
 */
int filter_tests_wrap(const struct dirent *entry);

/**
 * Count number of lines in given string.
 *
 * @param buffer    string in a buffer
 * @param len       length of the buffer
 * @return number of ends of line
 */
unsigned int count_lines(const char *buffer, int len);

/**
 * If condition evaluates to false, return value and exit function.
 *
 * @param cond  condition to test
 * @param val   value to be returned
 */
#define return_val_if_fail(cond,val) do { if (!(cond)) return val; } while (0)

/**
 * Helper enum to easily index ends of a pipe.
 */
enum {
    /**
     * Reading end of the pipe.
     */
    PIPE_READ  = 0,
    /**
     * Writing end of the pipe.
     */
    PIPE_WRITE = 1
};

/**
 * Macros defining ANSI escape sequences to alter appearance of printed text.
 */
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define BOLD    "\033[1m"
#define NORMAL  "\033[0m"

/**
 * Print string to stdout with specified color.
 *
 * @param color     what color to print
 * @param str       string to be printed
 */
#define print_color(color, str) printf("%s%s%s", color, str, NORMAL)

#endif /* end of include guard: UTILS_H */
