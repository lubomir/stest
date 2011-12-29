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

typedef enum {
    MODE_QUIET,
    MODE_VERBOSE
} VerbosityMode;

typedef struct test_t Test;
struct test_t {
    char *name;
    uint8_t parts;
};

typedef struct test_context_t TestContext;

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
 * Create new test context to run tests in.
 *
 * @param cmd   command to be tested
 * @param dir   directory where tests are stored
 * @return      new TestContext
 */
TestContext * test_context_new(const char *cmd, const char *dir);

/**
 * Free memory used by TestContext.
 *
 * @param tc    TestContext to be freed
 */
void test_context_free(TestContext *tc);

/**
 * Run list of tests in given context.
 *
 * @param tc        context to run in
 * @param tests     tests to be executed (list of Test objects)
 * @param verbose   whether to print diffs (if any)
 */
void test_context_run_tests(TestContext *tc, List *tests, VerbosityMode verbose);

#endif /* end of include guard: TEST_H */
