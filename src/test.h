#ifndef TEST_H
#define TEST_H

#include <config.h>

#include <stdint.h>

#include "list.h"

typedef struct test_t Test;
struct test_t {
    char *name;
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

#endif /* end of include guard: TEST_H */
