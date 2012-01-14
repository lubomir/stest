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

#endif /* end of include guard: TEST_H */
