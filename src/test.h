#ifndef TEST_H
#define TEST_H

#include <config.h>

#include <ctype.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

enum TestPart {
    TEST_INPUT  = 2 << 0,
    TEST_OUTPUT = 2 << 1,
    TEST_ARGS   = 2 << 3,
    TEST_ERRORS = 2 << 4,
    TEST_RETVAL = 2 << 5,
    TEST_UNKNOWN
};

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

#endif /* end of include guard: TEST_H */
