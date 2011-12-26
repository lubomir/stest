#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef struct _list List;

List * list_prepend(List *list, void *data);
List * list_append(List *list, void *data);
List * list_reverse(List *list);
void * list_get_data(List *list);


#endif /* end of include guard: LIST_H */
