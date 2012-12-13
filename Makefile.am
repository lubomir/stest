ACLOCAL_AMFLAGS = $$ACLOCAL_ARGS -I m4

bin_PROGRAMS = stest gen-test

stest_SOURCES = src/stest.c \
		src/list.c \
		src/outputqueue.c \
		src/test.c  \
		src/testcontext.c  \
		src/utils.c
stest_CFLAGS = ${AM_CFLAGS}

gen_test_SOURCES = src/gen-test.c \
		   src/genutils.c \
		   src/list.c \
		   src/test.c \
		   src/utils.c
gen_test_CFLAGS = ${AM_CFLAGS}

if ENABLE_TESTS
noinst_LTLIBRARIES = test/test_list.la \
		     test/test_utils.la \
		     test/test_oqueue.la \
		     test/test_genutils.la

TESTS = test/run-test.sh
TESTS_ENVIRONMENT = NO_MAKE="yes" CUTTER="$(CUTTER)"

echo-cutter:
	@echo $(CUTTER)

AM_CPPLAGS = $(CUTTER_CFLAGS) -I$(top_builddir)/src
LIBS += $(CUTTER_LIBS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_test_list_la_SOURCES = test/test-list.c \
			    src/list.c

test_test_utils_la_SOURCES = test/test-utils.c \
			     src/list.c \
			     src/utils.c

test_test_oqueue_la_SOURCES = test/test-oqueue.c \
			      src/list.c \
			      src/utils.c \
			      src/outputqueue.c

test_test_genutils_la_SOURCES = test/test-genutils.c \
				src/genutils.c
endif
