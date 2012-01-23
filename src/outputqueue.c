#include <unistd.h>
#include <string.h>

#include "outputqueue.h"
#include "utils.h"

struct oqueue_t {
    List *data;
};

OQueue * oqueue_new()
{
    OQueue *q = malloc(sizeof(OQueue));
    q->data = NULL;
    return q;
}

void oqueue_free(OQueue *q)
{
    if (q && q->data) {
        list_destroy(q->data, DESTROYFUNC(free));
    }
    free(q);
}

void oqueue_push(OQueue *q, char *str)
{
    q->data = list_prepend(q->data, strdup(str));
}

void oqueue_flush(OQueue *q, FILE *fh)
{
    q->data = list_reverse(q->data);
    list_foreach(q->data, CBFUNC(fputs), fh);
    list_destroy(q->data, DESTROYFUNC(free));
    q->data = NULL;
}

void oqueue_pushf(OQueue *q, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    oqueue_pushvf(q, fmt, ap);
    va_end(ap);
}

void oqueue_pushvf(OQueue *q, const char *fmt, va_list ap)
{
    char *str = NULL;
    size_t len = 100;
    int n;
    va_list ap_copy;

    str = calloc(len, sizeof(char));

    while (1) {
        va_copy(ap_copy, ap);
        n = vsnprintf(str, len, fmt, ap_copy);

        if (n > -1 && n < len)
            break;

        len *= 2;
        str = realloc(str, len * sizeof(char));
    }

    oqueue_push(q, str);
    free(str);
}

void oqueue_copy_from_fd(OQueue *dest, int source)
{
    char buffer[1024];
    ssize_t len;

    while ((len = read(source, buffer, 1024)) > 0) {
        buffer[len] = '\0';
        oqueue_push(dest, buffer);
    }
}

void oqueue_copy_from_valgrind(OQueue *queue, FILE *fh, int contexts)
{
    char *line = NULL;
    ssize_t size;
    size_t len = 0, strip;
    int i;

    fseek(fh, 0, SEEK_SET);
    for (i = 0; i < 5; i++) {   /* Strip leading header (5 lines) */
        getline(&line, &len, fh);
    }
    strip = getline(&line, &len, fh);
    for (i = 0; i < contexts; i++) {
        do {
            size = getline(&line, &len, fh);
            if (size <= strip) break;
            oqueue_push(queue, line + strip - 1);
        } while (1);
        oqueue_push(queue, "\n");
    }

    free(line);
}
