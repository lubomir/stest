#include <config.h>

#include "genutils.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

char * sanitize_test_name(char *name)
{
    char *res = name;
    int i = 0;

    while (isspace(*name)) name++;
    while (*name) {
        res[i] = isspace(*name) ? '_' : tolower(*name);
        if (res[i] != '_' || !i || res[i-1] != '_') i++;
        name++;
    }
    res[i] = 0;
    while (res[--i] == '_') res[i] = 0;

    return res;
}

int get_test_num(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    int max = 0;
    struct dirent *entry;
    if (dir == NULL) {
        perror("Can not open test directory");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (atoi(entry->d_name) > max) {
            max = atoi(entry->d_name);
        }
    }

    closedir(dir);
    return max;
}
