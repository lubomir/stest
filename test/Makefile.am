noinst_LTLIBRARIES = test_list.la

TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE="yes" CUTTER="$(CUTTER)"

echo-cutter:
	@echo $(CUTTER)

INCLUDES = $(CUTTER_CFLAGS) -I$(top_srcdir)/src
LIBS = $(CUTTER_LIBS) $(top_srcdir)/src/liblist.la
LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_list_la_SOURCES = test-list.c
