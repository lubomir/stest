#ifndef UTILS_H
#define UTILS_H

#include <config.h>

#include <dirent.h>
#include <stdio.h>
#include <sys/time.h>

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
 * If condition evalueates to false, return from current function.
 *
 * @param cond  condition to test
 */
#define return_if_fail(cond) do { if (!(cond)) return; } while (0)

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
#define YELLOW  "\033[1;33m"
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
 * Test if a certain bit is on in a bit mask
 */
#define FLAG_SET(mask,bit) (((mask) & (bit)) == (bit))

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

/**
 * Count how many lines are available on file descriptor source. This function
 * is destructive, therefore there is no way to read the input again after
 * counting lines.
 *
 * @param source    source file descriptor
 * @return number of lines
 */
size_t count_lines_on_fd(int source);

/**
 * Parse string as command line arguments.
 * This function tokenizes string into an array of strings in a similar way
 * a shell does. It does not do:
 *
 *  * any wildcard expansion
 *  * variable expansion
 *  * command substitution
 *  * arithmetic expansion
 *  * handle comments
 *
 * and many more things.
 *
 * If string is invalid (e.g. unterminated quote), NULL is returned. Caller is
 * responsible for freeing the memory.
 *
 * If second argument is not NULL, the size (in elements) of the returned
 * array is stored there.
 *
 * @param str   string to be parsed
 * @param len   pointer size of the returned array (out) (allow-none)
 * @return NULL-terminated array of strings or NULL on failure
 */
char ** parse_args(const char *str, size_t *len);

/**
 * Parse Valgrind output and find out how many errors in how many context were
 * found.
 *
 * @param fh    file handle to output
 * @param errors    where to store number of errors
 * @param contexts  where to store number of contexts
 * @return 0 if parsing failed, non-negative integer on success.
 */
int get_num_errors(FILE *fh, int *errors, int *contexts);

/**
 * Make the string bold. The returned pointer points to static memory and
 * should no be freed.
 *
 * @param str   string to become bold
 * @return modified string (transfer none)
 */
const char * str_to_bold(const char *str);

/**
 * Free a NULL-terminated array of strings. If array is NULL, no action
 * is performed.
 *
 * @param array     NULL-terminated array to be freed (allow-none)
 */
void str_array_free(char **array);

/**
 * Subtract time value in b from the time value in a, and place result
 * to the place pointed to by res. The result is normalized such that
 * res->tv_usec is in the range 0 to 999,999.
 *
 * This function is equivalent to timersub(3).
 *
 * @param a     first time value
 * @param b     second time value
 * @param res   place where to store result (out)
 */
void time_difference(struct timeval *a, struct timeval *b, struct timeval *res);

#endif /* end of include guard: UTILS_H */
