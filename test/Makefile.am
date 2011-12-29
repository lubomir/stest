noinst_LTLIBRARIES = test_list.la test_utils.la

TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE="yes" CUTTER="$(CUTTER)"

echo-cutter:
	@echo $(CUTTER)

INCLUDES = $(CUTTER_CFLAGS) -I$(top_srcdir)/src
LIBS = $(CUTTER_LIBS)
LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_list_la_SOURCES = test-list.c $(top_srcdir)/src/list.c
test_utils_la_SOURCES = test-utils.c $(top_srcdir)/src/utils.c
