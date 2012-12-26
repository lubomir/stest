#include <config.h>

#include "list.h"

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

    tmp = list;
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }

    tmp->next = new;
    return list;
}

List * list_reverse(List *list)
{
    List *new = NULL, *tmp;

    while (list != NULL) {
        tmp = list->next;
        list->next = new;
        new = list;
        list = tmp;
    }

    return new;
}

void list_foreach(List *list, CbFunc cb, void *data)
{
    while (list != NULL) {
        cb(list->data, data);
        list = list->next;
    }
}

void list_destroy(List *list, DestroyFunc destr)
{
    List *tmp;
    while (list != NULL) {
        if (destr != NULL) {
            destr(list->data);
        }
        tmp = list;
        list = list->next;
        free(tmp);
    }
}
