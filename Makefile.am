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

include doc/Makefile.include

if ENABLE_TESTS
include tests/Makefile.include
endif
