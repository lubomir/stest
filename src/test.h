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

typedef struct test_result_t TestResult;
struct test_result_t {
    char *output_diff;
    char *errors_diff;
    char *retval;
};
#define TEST_RESULT(x) ((TestResult *)(x))

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
 * Returns a list of test results.
 *
 * @param tc    context to run in
 * @param tests tests to be executed (list of Test objects)
 * @return list of TestResult objects
 */
List * test_context_run_tests(TestContext *tc, List *tests);

/**
 * Print details of failed tests to specified stream.
 *
 * @param tc    test context
 * @param fh    handle of stream to print to
 */
void test_context_flush_messages(TestContext *tc, FILE *fh);

#endif /* end of include guard: TEST_H */
