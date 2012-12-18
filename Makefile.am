ACLOCAL_AMFLAGS = $$ACLOCAL_ARGS -I m4

bin_PROGRAMS = stest gen-test

noinst_HEADERS = src/genutils.h \
		 src/list.h \
		 src/outputqueue.h \
		 src/test.h \
		 src/testcontext.h \
		 src/utils.h

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
check_LTLIBRARIES = test/test_list.la \
		    test/test_utils.la \
		    test/test_oqueue.la \
		    test/test_genutils.la
dist_check_SCRIPTS = test/run-test.sh

TESTS = test/run-test.sh
TESTS_ENVIRONMENT = NO_MAKE="yes" CUTTER="$(CUTTER)" TOP_DIR="$(top_builddir)"

echo-cutter:
	@echo $(CUTTER)

MY_CFLAGS = $(CUTTER_CFLAGS) -I$(top_srcdir)/src
MY_LIBS = $(CUTTER_LIBS)
MY_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_test_list_la_SOURCES = test/test-list.c \
			    src/list.c
test_test_list_la_CFLAGS = $(MY_CFLAGS)
test_test_list_la_LIBS = $(MY_LIBS)
test_test_list_la_LDFLAGS = $(MY_LDFLAGS)

test_test_utils_la_SOURCES = test/test-utils.c \
			     src/list.c \
			     src/utils.c
test_test_utils_la_CFLAGS = $(MY_CFLAGS)
test_test_utils_la_LIBS = $(MY_LIBS)
test_test_utils_la_LDFLAGS = $(MY_LDFLAGS)

test_test_oqueue_la_SOURCES = test/test-oqueue.c \
			      src/list.c \
			      src/utils.c \
			      src/outputqueue.c
test_test_oqueue_la_CFLAGS = $(MY_CFLAGS)
test_test_oqueue_la_LIBS = $(MY_LIBS)
test_test_oqueue_la_LDFLAGS = $(MY_LDFLAGS)

test_test_genutils_la_SOURCES = test/test-genutils.c \
				src/genutils.c
test_test_genutils_la_CFLAGS = $(MY_CFLAGS)
test_test_genutils_la_LIBS = $(MY_LIBS)
test_test_genutils_la_LDFLAGS = $(MY_LDFLAGS)
endif
