#ifndef TEST_H
#define TEST_H

#include <config.h>

#include <stdint.h>

#include "list.h"

typedef struct test_t Test;
struct test_t {
    char *name;
    const char *dir;
    uint8_t parts;
};

/**
 * Load all test definitions from a directory.
 *
 * @param dir   path to directory with tests
 * @return      list of tests
 */
List * test_load_from_dir(const char *dir);

/**
 * Free a test.
 *
 * @param test  test to be freed
 */
void test_free(Test *test);

/**
 * Open file descriptor with input for given test. If test does not specify
 * any input, /dev/null is opened and passed. It is responsibility of caller
 * to close this descriptor.
 *
 * @param test  test
 * @return file descriptor with new standard input
 */
int test_get_input_fd(Test *t);

/**
 * Get argument list for this test. Returns NULL-terminated list of strings on
 * success, NULL if file can not be opened or contains malformed arguments.
 *
 * @param test  test
 * @return NULL if arguments are malformed, otherwise return NULL-terminated
 *         array of strings
 */
char ** test_get_args(Test *test, size_t *count);

/**
 * Get expected exit code for this test.
 *
 * @param test  test
 * @return expected exit code or -1 on failure
 */
int test_get_exit_code(Test *test);

#endif /* end of include guard: TEST_H */
