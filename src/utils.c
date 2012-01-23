#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "utils.h"

int number_sort_wrap(const struct dirent **entry1, const struct dirent **entry2)
{
    return number_sort((*entry1)->d_name, (*entry2)->d_name);
}

int number_sort(const char *name1, const char *name2)
{
    unsigned int num1, num2;

    sscanf(name1, "%u", &num1);
    sscanf(name2, "%u", &num2);

    return num1 - num2;
}

int filter_tests_wrap(const struct dirent *entry)
{
    return filter_tests(entry->d_name);
}

int filter_tests(const char *name)
{
    int digits = 0;

    while (*name && isdigit(*name)) {
        digits++;
        name++;
    }

    return digits;
}

unsigned int count_lines(const char *buffer, int len)
{
    unsigned int num = 0;
    while ((len--) > 0) if (*(buffer++) == '\n') num++;
    return num;
}

void print_color(const char *color, const char *str)
{
    if (isatty(STDOUT_FILENO)) {
        printf("%s%s%s", color, str, NORMAL);
        fflush(stdout);
    } else {
        printf("%s", str);
    }
}

TestPart get_test_part(const char *filename)
{
    static const char *extensions[] = { EXT_INPUT, EXT_OUTPUT,
        EXT_ARGS, EXT_ERRORS, EXT_RETVAL, NULL };
    char *ext;
    int i;

    return_val_if_fail(filename != NULL, TEST_UNKNOWN);
    ext = strrchr(filename, '.');
    return_val_if_fail(ext != NULL, TEST_UNKNOWN);
    ext++;

    for (i = 0; extensions[i] != NULL; i++) {
        if (strcmp(ext, extensions[i]) == 0)
            return 2 << i;
    }

    return TEST_UNKNOWN;
}

char * get_filepath(const char *dir, const char *fname, const char *ext)
{
    char *path;

    return_val_if_fail(dir != NULL, NULL);
    return_val_if_fail(fname != NULL, NULL);
    return_val_if_fail(ext != NULL, NULL);

    path = calloc(strlen(dir) + strlen(fname) + strlen(ext) + 3, sizeof(char));
    sprintf(path, "%s/%s.%s", dir, fname, ext);
    return path;
}

size_t count_lines_on_fd(int source)
{
    char buffer[1024];
    ssize_t len;
    size_t lines = 0;

    while ((len = read(source, buffer, 1024)) > 0) {
        lines += count_lines(buffer, len);
    }

    return lines;
}

/**
 * Make s point to next character. If that character is terminating '\0',
 * go to out label.
 */
#define try_inc_string(s) do { s++; if (*s == '\0') goto out; } while (0)

char ** parse_args(const char *str, size_t *len)
{
    char **array;
    List *list = NULL, *tmp;
    char *string = NULL;
    size_t idx = 0, i = 0;
    char escape;

    return_val_if_fail(str != NULL, NULL);

    while (*str) {
        if (string == NULL && *str != '\0') {
            string = calloc(strlen(str) + 1, sizeof(char));
            i = 0;
        }
        if (isspace(*str)) {
            if (*string != '\0') {
                list = list_prepend(list, string);
                idx++;
            } else {
                free(string);
            }
            string = NULL;
            i = 0;
        } else if (*str == '\\') {
            try_inc_string(str);
            string[i++] = *str;
        } else if (*str == '"' || *str == '\'') {
            escape = *str;
            do {
                try_inc_string(str);
                if (escape == '"' && *str == '\\') {
                    try_inc_string(str);
                    string[i++] = *str++;
                }
                if (*str != escape) string[i++] = *str;
            } while (*str != escape);
        } else {
            string[i++] = *str;
        }
        str++;
    }
    if (string != NULL && *string != '\0') {
        list = list_prepend(list, string);
        idx++;
    }

    array = calloc(idx + 1, sizeof(char *));
    if (len) *len = idx + 1;
    if (idx != 0) {
        idx--;
        for (tmp = list; tmp != NULL; tmp = tmp->next) {
            array[idx--] = tmp->data;
        }
        list_destroy(list, NULL);
    }

    return array;

out:
    if (list) list_destroy(list, DESTROYFUNC(free));
    if (len) *len = 0;
    return NULL;
}

int get_num_errors(FILE *fh, int *errors, int *contexts)
{
    char c;

    fseek(fh, -40, SEEK_END);
    do {
        c = fgetc(fh);
        fseek(fh, -2, SEEK_CUR);
    } while (c != 'Y');
    fseek(fh, 3, SEEK_CUR);
    return fscanf(fh, " %d errors from %d", errors, contexts) == 2;
}

#define MAXLEN 512
char * str_to_bold(const char *str)
{
    static char array[MAXLEN];
    snprintf(array, MAXLEN, "%s%s%s", BOLD, str, NORMAL);
    return array;
}
