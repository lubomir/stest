#include <cutter.h>
#include <utils.h>

#define call_count_lines(str) count_lines(str, strlen(str))

void
test_count_lines()
{
    cut_assert_equal_uint(0, call_count_lines("foo bar"));
    cut_assert_equal_uint(1, call_count_lines("foo\nbar"));
    cut_assert_equal_uint(2, call_count_lines("foo\nbar\n"));
    cut_assert_equal_uint(3, call_count_lines("foo\nbar\nbaz\n"));
    cut_assert_equal_uint(7, call_count_lines("\nfoo\n\nbar\n\nbaz\n\n"));
    cut_assert_equal_uint(0, call_count_lines(""));
    cut_assert_equal_uint(3, call_count_lines("\n\n\n"));
}

void
test_number_sort()
{
    cut_assert_equal_int(0, number_sort("001_file1", "001_file2"));
    cut_assert_equal_int(-1, number_sort("001_file1", "002_file2"));
    cut_assert_equal_int(1, number_sort("002_file1", "001_file2"));
    cut_assert_equal_int(-2, number_sort("002_file1", "004_file2"));
    cut_assert_equal_int(2, number_sort("006_file1", "004_file2"));
    cut_assert_equal_int(-10, number_sort("006_file1", "016_file2"));
    cut_assert_equal_int(99, number_sort("100_file1", "001_file2"));
    cut_assert_equal_int(-99, number_sort("001_file1", "100_file2"));

    cut_assert_equal_int(99, number_sort("00100_file1", "001_file2"));
    cut_assert_equal_int(0, number_sort("00100_file1", "100_file2"));
}

void
test_filter_tests()
{
    cut_assert_equal_int(1, filter_tests("1_file1"));
    cut_assert_equal_int(3, filter_tests("001_file1"));
    cut_assert_equal_int(5, filter_tests("00100_file1"));
    cut_assert_equal_int(0, filter_tests("bad_file1"));
    cut_assert_equal_int(1, filter_tests("0-almost_bad_file1"));
    cut_assert_equal_int(10, filter_tests("1234567890_file1"));
}

void
test_get_test_part_ok()
{
    cut_assert_equal_uint(TEST_INPUT, get_test_part("001_file.in"));
    cut_assert_equal_uint(TEST_OUTPUT, get_test_part("001_file.out"));
    cut_assert_equal_uint(TEST_ERRORS, get_test_part("001_file.err"));
    cut_assert_equal_uint(TEST_ARGS, get_test_part("001_file.args"));
    cut_assert_equal_uint(TEST_RETVAL, get_test_part("001_file.ret"));
}

void
test_get_test_part_multiple_exts()
{
    cut_assert_equal_uint(TEST_INPUT, get_test_part("001_file.out.in"));
    cut_assert_equal_uint(TEST_OUTPUT, get_test_part("001_file.in.out"));
    cut_assert_equal_uint(TEST_ERRORS, get_test_part("001_file.ret.err"));
    cut_assert_equal_uint(TEST_ARGS, get_test_part("001_file.in.args"));
    cut_assert_equal_uint(TEST_RETVAL, get_test_part("001_file.out.ret"));
}

void
test_get_test_part_bad()
{
    cut_assert_equal_uint(TEST_UNKNOWN, get_test_part(NULL));

    static char *bad_ones[] = { "001_file.bad", "002_file.", "003_no_ext",
        "", NULL};

    for (int i = 0; bad_ones[i] != NULL; i++) {
        cut_assert_equal_uint(TEST_UNKNOWN, get_test_part(bad_ones[i]));
    }
}

void
test_get_filepath_ok()
{
    char *path;
    path = get_filepath("dir", "file", "txt");
    cut_assert_equal_string("dir/file.txt", path);
    free(path);

    path = get_filepath(".", "file", "in");
    cut_assert_equal_string("./file.in", path);
    free(path);
}

void
test_get_filepath_failure()
{
    cut_assert_null(get_filepath(NULL, "file", "txt"));
    cut_assert_null(get_filepath("dir", NULL, "txt"));
    cut_assert_null(get_filepath("dir", "file", NULL));
}
