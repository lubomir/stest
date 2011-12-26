#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef struct _list List;

typedef void (*CbFunc) (void *, void *);
typedef void (*DestroyFunc) (void *);

List * list_prepend(List *list, void *data);
List * list_append(List *list, void *data);
List * list_reverse(List *list);
void * list_get_data(List *list);
void   list_foreach(List *list, CbFunc cb, void *data);
void   list_destroy(List *list, DestroyFunc destr);


#endif /* end of include guard: LIST_H */
