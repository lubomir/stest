#include <cutter.h>
#include <list.h>

#define INT_TO_POINTER(n) (void *)(long)n
#define POINTER_TO_INT(p) (int)(long)p

void
test_prepending_into_null()
{
    int x = 0;
    List *l = list_prepend(NULL, &x);
    cut_assert_not_null(l);
    cut_assert_null(l->next);
    cut_assert_equal_pointer(&x, l->data);
}

void
test_append_to_null()
{
    int x = 0;
    List *l = list_append(NULL, &x);
    cut_assert_not_null(l);
    cut_assert_null(l->next);
    cut_assert_equal_pointer(&x, l->data);
}

void
test_two_prepends()
{
    int x, y;
    List *l = list_prepend(NULL, &x);
    l = list_prepend(l, &y);
    cut_assert_not_null(l);
    cut_assert_equal_pointer(&y, l->data);
    cut_assert_not_null(l->next);
    cut_assert_equal_pointer(&x, l->next->data);
    cut_assert_null(l->next->next);
}

void
test_two_appends()
{
    List *l = list_append(NULL, INT_TO_POINTER(1));
    cut_assert_not_null(l);
    cut_assert_null(l->next);
    cut_assert_equal_pointer(INT_TO_POINTER(1), l->data);

    l = list_append(l, INT_TO_POINTER(2));
    cut_assert_equal_pointer(INT_TO_POINTER(1), l->data);
    cut_assert_not_null(l->next);
    cut_assert_equal_pointer(INT_TO_POINTER(2), l->next->data);
    cut_assert_null(l->next->next);
}

void
test_reverse_list()
{
    List *l = list_prepend(NULL, INT_TO_POINTER(1));
    l = list_prepend(l, INT_TO_POINTER(2));
    l = list_prepend(l, INT_TO_POINTER(3));

    l = list_reverse(l);

    cut_assert_not_null(l);
    cut_assert_equal_pointer(INT_TO_POINTER(1), l->data);
    cut_assert_not_null(l->next);
    cut_assert_equal_pointer(INT_TO_POINTER(2), l->next->data);
    cut_assert_not_null(l->next->next);
    cut_assert_equal_pointer(INT_TO_POINTER(3), l->next->next->data);
    cut_assert_null(l->next->next->next);
}

void foreach_cb(void *item, void *data)
{
    int i = POINTER_TO_INT(item);
    int *dest = (int*) data;
    *dest = *dest + i;
}

void test_list_foreach()
{
    List *l = list_prepend(NULL, INT_TO_POINTER(1));
    l = list_prepend(l, INT_TO_POINTER(2));
    l = list_prepend(l, INT_TO_POINTER(3));

    int sum = 0;
    list_foreach(l, foreach_cb, &sum);
    cut_assert_equal_int(6, sum);
}
