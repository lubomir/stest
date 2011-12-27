bin_PROGRAMS = stest
lib_LTLIBRARIES = liblist.la

LDFLAGS = -no-undefined

stest_LDADD = $(top_srcdir)/src/liblist.la
stest_SOURCES = main.c test.c utils.c

liblist_la_SOURCES = list.c
