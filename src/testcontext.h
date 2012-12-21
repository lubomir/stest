#ifndef TESTCONTEXT_H
#define TESTCONTEXT_H

#include <config.h>

#include "list.h"

typedef enum {
    MODE_QUIET,
    MODE_NORMAL,
    MODE_VERBOSE
} VerbosityMode;

typedef struct test_context_t TestContext;

/**
 * Create new test context to run tests in.
 *
 * @return          new TestContext
 */
TestContext * test_context_new(void);

/**
 * Set command to be tested. The passed string is tested to be path to
 * executable file. If it fails, the function prints error message and
 * returns zero.
 *
 * @param tc        test context to modify
 * @param cmd       command to be tested
 * @return 1 if ok, 0 if command is not executable
 */
int test_context_set_command(TestContext *tc, const char *cmd);

/**
 * Set directory with tests.
 *
 * @param tc        test context to modify
 * @param dir       directory where tests are stored
 */
void test_context_set_dir(TestContext *tc, const char *cmd);

/**
 * Use Valgrind tool in the context.
 *
 * @param tc        test context to modify
 */
void test_context_set_use_valgrind(TestContext *tc);

/**
 * Set custom options to underlying diff.
 *
 * @param tc        test context to modify
 * @param diff_opts option string passed to diff (allow-none)
 */
void test_context_set_diff_opts(TestContext *tc, const char *opts);

/**
 * Set verbosity level.
 *
 * @param tc        test context to modify
 * @param verbose   level to be set
 */
void test_context_set_verbosity(TestContext *tc, VerbosityMode verbose);

/**
 * Free memory used by TestContext.
 *
 * @param tc    TestContext to be freed (allow-none)
 */
void test_context_free(TestContext *tc);

/**
 * Run list of tests in given context.
 *
 * @param tc        context to run in
 * @param tests     tests to be executed (list of Test objects)
 * @return number of failed checks
 */
unsigned int
test_context_run_tests(TestContext *tc, List *tests);

#endif /* end of include guard: TESTCONTEXT_H */
