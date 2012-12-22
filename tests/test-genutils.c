#include <cutter.h>
#include <string.h>

#include <genutils.h>

void
test_sanitize_test_name(void)
{
    char test[128];
    strcpy(test, "test name");
    cut_assert_equal_string("test_name", sanitize_test_name(test),
                cut_message("single space"));

    strcpy(test, "test\tname");
    cut_assert_equal_string("test_name", sanitize_test_name(test),
        cut_message("single tab"));

    strcpy(test, "TEST");
    cut_assert_equal_string("test", sanitize_test_name(test),
        cut_message("uppercase"));

    strcpy(test, "  test name  ");
    cut_assert_equal_string("test_name", sanitize_test_name(test),
            cut_message("leading and trailing whitespace"));

    strcpy(test, "test  name");
    cut_assert_equal_string("test_name", sanitize_test_name(test),
            cut_message("multiple spaces"));
}
