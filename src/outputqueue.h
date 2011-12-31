#ifndef OUTPUTQUEUE_H
#define OUTPUTQUEUE_H

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#include "list.h"

/**
 * Output queue is basically a wrapper around a list of strings. However, there
 * is no direct access to the list.
 */
typedef struct oqueue_t OQueue;

/**
 * Create new Output Queue.
 *
 * @return new output queue
 */
OQueue * oqueue_new();

/**
 * Free memory used by the output queue.
 *
 * @param q     queue to be freed
 */
void oqueue_free(OQueue *q);

/**
 * Add another string to queue. This string will be duplicated.
 *
 * @param q     the output queue
 * @param str   string to be added
 */
void oqueue_push(OQueue *q, char *str);

/**
 * Printf-like version of pushing data to output queue.
 *
 * @param q     the output queue
 * @param fmt   printf-like formatting string
 * @param ...   arguments for format
 */
void oqueue_pushf(OQueue *q, const char *fmt, ...);

/**
 * This function is exactly same as oqueue_pushf() except that is called with
 * a va_list instead of variable number of arguments.
 *
 * @param q     the output queue
 * @param fmt   printf-like formatting string
 * @param ap    arguments for the format
 */
void oqueue_pushvf(OQueue *q, const char *fmt, va_list ap);

/**
 * Print all strings to specified file handler. The strings will be freed
 * and queue emptied. It still has to be freed using oqueue_free().
 *
 * @param q     queue to be flushed
 * @param fh    where to output the text
 */
void oqueue_flush(OQueue *q, FILE *fh);

/**
 * Copy all contents from file descriptor to the queue.
 *
 * @param queue     destination queue
 * @param source    source file descriptor
 */
void oqueue_copy_from_fd(OQueue *queue, int source);

#endif /* end of include guard: OUTPUTQUEUE_H */