#ifndef LIST_H
#define LIST_H

#include <config.h>

#include <stdlib.h>

typedef struct _list List;
struct _list {
    void *data;
    List *next;
};

/**
 * Function to call for each stored piece of data.
 */
typedef void (*CbFunc) (void *, void *);
/**
 * Cast a function to CbFunc.
 */
#define CBFUNC(x) ((void(*)(void*,void*))(x))

/**
 * Function to free stored data.
 */
typedef void (*DestroyFunc) (void *);
/**
 * Cast a function to DestroyFunc.
 */
#define DESTROYFUNC(x) ((void(*)(void*))x)

/**
 * Prepend item to the start of the list.
 *
 * @param list  list to prepend to (allow-none)
 * @param data  data to store
 * @return      new start of the list
 */
List * list_prepend(List *list, void *data);

/**
 * Add item to end of list. This function runs in O(n). If you intend to
 * create list by repeatedly calling list_append(), you should prepend
 * to the list and then call list_reverse().
 *
 * @param list  list to append to (allow-none)
 * @param data  data to store
 * @return      new start of the list
 */
List * list_append(List *list, void *data);

/**
 * Reverse items in the list.
 *
 * @param list  list to reverse (allow-none)
 * @return      new start of the list
 */
List * list_reverse(List *list);

/**
 * Call function for each stored piece of data. Callback function should
 * accept two parameters - the first is the data from the list, the second
 * user specified data. Please note that calling list_destroy() from callback
 * is definitely not a good idea and may result in crash.
 *
 * @param list  list to process (allow-none)
 * @param cb    function to call
 * @param data  user data for callback function
 */
void   list_foreach(List *list, CbFunc cb, void *data);

/**
 * Destroy list. If destructor function is not NULL, then it is called for each
 * stored pointer. Unspecified destructor may result in memory leaks.
 *
 * @param list  start of the list to be destroyed (allow-none)
 * @param destr destructor function for data
 */
void   list_destroy(List *list, DestroyFunc destr);


#endif /* end of include guard: LIST_H */
