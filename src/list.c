#include "list.h"

struct _list {
    void *data;
    List *next;
};

List * list_prepend(List *list, void *data)
{
    List *new = malloc(sizeof(List));
    new->data = data;
    new->next = list;
    return new;
}

List * list_append(List *list, void *data)
{
    List *new, *tmp;

    if (list == NULL) {
        return list_prepend(list, data);
    }

    new = malloc(sizeof(List));
    new->data = data;
    new->next = NULL;

    while (list != NULL) {
        tmp = list;
        list = list->next;
    }

    tmp->next = new;
}

List * list_reverse(List *list)
{
    List *new = NULL;

    while (list != NULL) {
        new = list_prepend(new, list->data);
        list = list->next;
    }

    return new;
}

void * list_get_data(List *list)
{
    return list == NULL ? NULL : list->data;
}
