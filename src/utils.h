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
 * If stdout is not a terminal, skip all escape sequences.
 *
 * @param color     what color to print
 * @param str       string to be printed
 */
void print_color(const char *color, const char *str);

/**
 * Each test has at least two parts.
 */
typedef enum {
    /** Gets passed to stdin */
    TEST_INPUT  = 2 << 0,
    /** Is expected on stdout */
    TEST_OUTPUT = 2 << 1,
    /** Gets passed as command line arguments */
    TEST_ARGS   = 2 << 2,
    /** Is expected on stderr */
    TEST_ERRORS = 2 << 3,
    /** Expected exit value of the program */
    TEST_RETVAL = 2 << 4,
    /** Falback value for unknown test type */
    TEST_UNKNOWN
} TestPart;

/**
 * String representations of test parts.
 */
#define EXT_INPUT   "in"
#define EXT_OUTPUT  "out"
#define EXT_ARGS    "args"
#define EXT_ERRORS  "err"
#define EXT_RETVAL  "ret"

/**
 * Find out which test part is stored in given file.
 *
 * @param filename  file for which the type is queried
 * @return test type
 */
TestPart get_test_part(const char *filename);

/**
 * Build a filename. Caller must free the memory once it is not needed anymore.
 *
 * @param dir   directory with files
 * @param fname file basename
 * @param ext   extension
 * @return zero-terminated string with path
 */
char * get_filepath(const char *dir, const char *fname, const char *ext);

#endif /* end of include guard: UTILS_H */
