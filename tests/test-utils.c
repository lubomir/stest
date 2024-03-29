#include <cutter.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#define call_count_lines(str) count_lines(str, strlen(str))

void
test_count_lines(void)
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
test_number_sort(void)
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
test_filter_tests(void)
{
    cut_assert_equal_int(1, filter_tests("1_file1"));
    cut_assert_equal_int(3, filter_tests("001_file1"));
    cut_assert_equal_int(5, filter_tests("00100_file1"));
    cut_assert_equal_int(0, filter_tests("bad_file1"));
    cut_assert_equal_int(1, filter_tests("0-almost_bad_file1"));
    cut_assert_equal_int(10, filter_tests("1234567890_file1"));
}

void
test_get_test_part_ok(void)
{
    cut_assert_equal_uint(TEST_INPUT, get_test_part("001_file.in"));
    cut_assert_equal_uint(TEST_OUTPUT, get_test_part("001_file.out"));
    cut_assert_equal_uint(TEST_ERRORS, get_test_part("001_file.err"));
    cut_assert_equal_uint(TEST_ARGS, get_test_part("001_file.args"));
    cut_assert_equal_uint(TEST_RETVAL, get_test_part("001_file.ret"));
}

void
test_get_test_part_multiple_exts(void)
{
    cut_assert_equal_uint(TEST_INPUT, get_test_part("001_file.out.in"));
    cut_assert_equal_uint(TEST_OUTPUT, get_test_part("001_file.in.out"));
    cut_assert_equal_uint(TEST_ERRORS, get_test_part("001_file.ret.err"));
    cut_assert_equal_uint(TEST_ARGS, get_test_part("001_file.in.args"));
    cut_assert_equal_uint(TEST_RETVAL, get_test_part("001_file.out.ret"));
}

void
test_get_test_part_bad(void)
{
    cut_assert_equal_uint(TEST_UNKNOWN, get_test_part(NULL));

    int i;
    static char *bad_ones[] = { "001_file.bad", "002_file.", "003_no_ext",
        "", NULL};

    for (i = 0; bad_ones[i] != NULL; i++) {
        cut_assert_equal_uint(TEST_UNKNOWN, get_test_part(bad_ones[i]));
    }
}

void
test_get_filepath_ok(void)
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
test_get_filepath_failure(void)
{
    cut_assert_null(get_filepath(NULL, "file", "txt"));
    cut_assert_null(get_filepath("dir", NULL, "txt"));
    cut_assert_null(get_filepath("dir", "file", NULL));
}

#define write_data(fd, str) write(fd, str, strlen(str) * sizeof(char))
void
test_count_lines_on_fd_no_lines(void)
{
    int from[2];
    pipe(from);

    write_data(from[PIPE_WRITE], "foo bar baz");
    close(from[PIPE_WRITE]);

    cut_assert_equal_uint(0, count_lines_on_fd(from[PIPE_READ]));
    close(from[PIPE_READ]);
}

void
test_count_lines_on_fd_more_lines(void)
{
    int from[2];

    pipe(from);

    write_data(from[PIPE_WRITE], "foo\n");
    write_data(from[PIPE_WRITE], "bar\n");
    write_data(from[PIPE_WRITE], "baz\n");
    close(from[PIPE_WRITE]);

    cut_assert_equal_uint(3, count_lines_on_fd(from[PIPE_READ]));
    close(from[PIPE_READ]);
}

void
test_parse_args_single(void)
{
    char *expected[] = { "foo", NULL };
    char *tests[] = {"foo", "  foo", "foo  ", "  foo  ", NULL};
    int i = 0;

    while (tests[i] != NULL) {
        cut_assert_equal_string_array_with_free(expected,
                parse_args(tests[i], NULL),
                cut_message("Input: >%s<", tests[i]));
        i++;
    }
}

void
test_parse_args_double(void)
{
    char *expected[] = { "foo", "bar", NULL };
    char *tests[] = { "foo bar", "  foo bar", "foo  bar", "foo bar  ",
        "  foo bar  ", "  foo  bar  ", NULL };
    int i = 0;

    while (tests[i] != NULL) {
        cut_assert_equal_string_array_with_free(expected,
                parse_args(tests[i], NULL),
                cut_message("Input: >%s<", tests[i]));
        i++;
    }
}

void
test_parse_args_escaped(void)
{
    char *expected[] = { "foo", "bar", "baz quux", NULL };
    char *tests[] = {
        "foo bar baz\" \"quux",
        "foo bar \"baz quux\"",
        "foo bar baz\\ quux",
        "foo bar 'baz quux'",
        "foo bar baz' 'quux",
        "'foo' 'bar' \"baz quux\"",
        NULL };
    int i = 0;

    while (tests[i] != NULL) {
        cut_assert_equal_string_array_with_free(expected,
                parse_args(tests[i], NULL),
                cut_message("Input: >%s<", tests[i]));
        i++;
    }
}

void
test_parse_args_complicated(void)
{
    char *expected[] = { "\\", "'foo'", "bar\"baz", NULL };
    char *tests[] = {
        "\\\\       \"'foo'\"       bar\\\"baz",
        "'\\'       \\\'foo\\\'     bar'\"'baz",
        "\"\\\\\"   \\'foo\\'       bar'\"'baz",
        "\"\\\\\"   \"'\"foo\"'\"   bar\"\\\"\"baz",
        "'\\'       \"'\"foo\"'\"   bar\"\\\"\"baz",
        NULL };
    int i = 0;

    while (tests[i] != NULL) {
        cut_assert_equal_string_array_with_free(expected,
                parse_args(tests[i], NULL),
                cut_message("Input: >%s<", tests[i]));
        i++;
    }
}

void
test_parse_args_empty(void)
{
    char *tests[] = { "", " ", "''", "\"\"", NULL };
    char *expected[] = { NULL };
    int i = 0;
    size_t len = 0;

    while (tests[i] != NULL) {
        cut_assert_equal_string_array_with_free(expected,
                parse_args(tests[i], &len),
                cut_message("Input: >%s<", tests[i]));
        cut_assert_equal_uint(1, len, cut_message("Input: >%s<", tests[i]));
        i++;
    }
}

void
test_parse_args_malformed(void)
{
    char *tests[] = {
        "\"",
        "'",
        "\\",
        "\"foo",
        "'foo",
        "foo\\",
        "foo \"",
        "foo '",
        "foo \\",
        "foo \" bar",
        "foo ' bar",
        "foo \"\\",
        NULL };
    int i = 0;

    while (tests[i] != NULL) {
        cut_assert_null(parse_args(tests[i], NULL),
                cut_message("Input: >%s<", tests[i]));
        i++;
    }
}

void
test_parse_args_many(void)
{
    char *expected[] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
        "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
        "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
        "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        NULL };
    cut_assert_equal_string_array_with_free(expected,
            parse_args("a b c d e f g h i j k l m n o p q r s t u v w x y z "
                       "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z",
                       NULL));
}

void
test_parse_args_long(void)
{
    char *expected[] = { "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "1234567890987654321", NULL };
    cut_assert_equal_string_array_with_free(expected,
            parse_args("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ "
                       " 1234567890987654321", NULL));
}

void
test_parse_args_len(void)
{
    char *expected1[] = { "foo", NULL };
    char *expected2[] = { "foo", "bar", NULL };
    char *expected3[] = { "foo", "bar", "baz", NULL };
    char *expected4[] = { "foo bar", NULL };
    char **expected[] = { expected1, expected2, expected3, expected4 };
    char *tests[] = { "foo", "foo bar", "foo bar baz", "'foo bar'", NULL };
    size_t lengths[] = { 2, 3, 4, 2 };
    size_t len = 0;
    int i;

    for (i = 0; tests[i] != NULL; i++) {
        cut_assert_equal_string_array_with_free(expected[i],
                parse_args(tests[i], &len),
                cut_message("Input -%s-", tests[i]));
        cut_assert_equal_uint(lengths[i], len,
                cut_message("Input -%s-", tests[i]));
    }
}

void
test_timersub(void)
{
    struct timeval a, b, res;

    a.tv_sec = 1;
    a.tv_usec = 1;
    b.tv_sec = 0;
    b.tv_usec = 0;
    time_difference(&a, &b, &res);
    cut_assert_equal_int(1, res.tv_sec);
    cut_assert_equal_int(1, res.tv_usec);

    a.tv_sec = 10;
    a.tv_usec = 0;
    b.tv_sec = 0;
    b.tv_usec = 100;
    time_difference(&a, &b, &res);
    cut_assert_equal_int(9, res.tv_sec);
    cut_assert_equal_int(999900, res.tv_usec);
}
