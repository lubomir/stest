#include <stdio.h>
#include <ctype.h>

#include "utils.h"

int number_sort(const struct dirent **entry1, const struct dirent **entry2)
{
    unsigned int num1, num2;

    sscanf((*entry1)->d_name, "%u", &num1);
    sscanf((*entry2)->d_name, "%u", &num2);

    return num1 - num2;
}

int filter_tests(const struct dirent *entry)
{
    int digits = 0;
    const char *name = entry->d_name;

    while (*name && isdigit(*name)) {
        digits++;
        name++;
    }

    return digits > 0;
}

unsigned int count_lines(const char *buffer, int len)
{
    unsigned int num = 0;
    while ((len--) > 0) if (*(buffer++) == '\n') num++;
    return num;
}
