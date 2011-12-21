#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>

#define return_val_if_fail(x,v) do { if (!(x)) return v; } while (0)

enum TestPart {
    TEST_INPUT  = 2 << 0,
    TEST_OUTPUT = 2 << 1,
    TEST_ARGS   = 2 << 3,
    TEST_ERRORS = 2 << 4,
    TEST_RETVAL = 2 << 5,
    TEST_UNKNOWN
};

typedef struct test_t Test;
struct test_t {
    char *name;
    uint8_t parts;
};

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

enum TestPart get_test_part(const char *filename)
{
    return_val_if_fail(filename != NULL, TEST_UNKNOWN);
    char *ext = strrchr(filename, '.');
    return_val_if_fail(ext != NULL, TEST_UNKNOWN);
    ext++;

    if (strcmp(ext, "in") == 0)
        return TEST_INPUT;
    if (strcmp(ext, "out") == 0)
        return TEST_OUTPUT;
    if (strcmp(ext, "args") == 0)
        return TEST_ARGS;
    if (strcmp(ext, "errors") == 0)
        return TEST_ERRORS;
    if (strcmp(ext, "ret") == 0)
        return TEST_RETVAL;

    return TEST_UNKNOWN;
}

Test * load_tests(const char *dir)
{
    struct dirent **entries;
    int i;

    scandir(dir, &entries, filter_tests, number_sort);

    for (i = 0; entries[i] != NULL; i++) {
        printf("Found test: %s\n", entries[i]->d_name);
        free(entries[i]);
    }

    free(entries);
}

int run_program(const char *cmd)
{
    pid_t child;
    int status;

    child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child == 0) {    /* Child */
        execl(cmd, cmd, NULL);
        perror("exec");
        exit(EXIT_FAILURE);
    }                           /* Parent */
    wait(&status);
    return WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status);
}

int main(int argc, char *argv[])
{
    struct stat info;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s PROGRAM\n", argv[0]);
        return 1;
    }

    load_tests("tests");
    return 0;

    if (stat(argv[1], &info) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    if (info.st_mode & S_IXUSR != S_IXUSR) {
        fprintf(stderr, "Can not execute '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (run_program(argv[1]) == 0) {
        printf("OK\n");
    } else {
        printf("Bad\n");
    }

    return 0;
}
