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
 * @param cmd       command to be tested
 * @param dir       directory where tests are stored
 * @param mem       whether to check for memory errors
 * @param diff_opts option string passed to diff [NULL]
 * @return          new TestContext
 */
TestContext * test_context_new(const char *cmd,
                               const char *dir,
                               int mem,
                               char *diff_opts);

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
 * @return number of failed checks
 */
unsigned int
test_context_run_tests(TestContext *tc, List *tests, VerbosityMode verbose);

#endif /* end of include guard: TESTCONTEXT_H */
