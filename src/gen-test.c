#include <config.h>

#include "genutils.h"
#include "utils.h"
#include "test.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void
print_test(void *t, void *data)
{
    Test *test = (Test*) t;
    printf("%-30s (%c%c:%c%c%c)\n", test->name,
            FLAG_SET(test->parts, TEST_INPUT)   ? 'I' : ' ',
            FLAG_SET(test->parts, TEST_ARGS)    ? 'A' : ' ',
            FLAG_SET(test->parts, TEST_OUTPUT)  ? 'O' : ' ',
            FLAG_SET(test->parts, TEST_ERRORS)  ? 'E' : ' ',
            FLAG_SET(test->parts, TEST_RETVAL)  ? 'R' : ' ');
}

static void
copy_from_to(FILE *from, FILE *to)
{
    char buffer[1024];
    size_t len;
    do {
        len = fread(buffer, sizeof(char), 1024, from);
        fwrite(buffer, sizeof(char), len, to);
    } while (len == 1024);
}

#define OPTSTRING "ioer:a:lhV"

static void help()
{
    printf("Help\n");
}

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [OPTIONS] [TESTDIR]\n", progname);
}

static const struct option long_options[] = {
    { "input",      required_argument,  NULL, 0   },
    { "output",     required_argument,  NULL, 0   },
    { "errors",     required_argument,  NULL, 0   },
    { "retval",     required_argument,  NULL, 'r' },
    { "args",       required_argument,  NULL, 'a' },
    { "list",       no_argument,        NULL, 'l' },
    { "help",       no_argument,        NULL, 'h' },
    { "version",    no_argument,        NULL, 'V' },
    { 0, 0, 0, 0 }
};

static const char *stream_name[] = { "input", "output", "errors" };
static const char *exts[] = { EXT_INPUT, EXT_OUTPUT, EXT_ERRORS };

int main(int argc, char *argv[])
{
    char c;
    int i, list_tests = 0;
    char *dir = "tests";
    char buffer[128], test_name[128];
    char *files[] = { NULL, NULL, NULL };
    int  from_user[] = { 0, 0, 0 };
    char *args = NULL, *retval = NULL, *name;

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, OPTSTRING, long_options, &option_index);
        if (c == -1) break;

        switch (c) {
        case 0:
            files[option_index] = optarg;
            break;
        case 'i':
            from_user[0] = 1;
            break;
        case 'o':
            from_user[1] = 1;
            break;
        case 'e':
            from_user[2] = 1;
            break;
        case 'r':
            retval = optarg;
            break;
        case 'a':
            args = optarg;
            break;

        case 'l':
            list_tests = 1;
            break;
        case 'h':
            help();
            return 0;
        case 'V':
            printf("%s %s\n", PACKAGE_NAME, VERSION);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }
    if (optind < argc) {
        dir = argv[optind];
    }

    if (list_tests) {
        List *tests = test_load_from_dir(dir);
        list_foreach(tests, print_test, NULL);
        list_destroy(tests, DESTROYFUNC(test_free));
        return 0;
    }

    for (i = 0; i < 3; i++) {
        if (files[i] != NULL && from_user[i]) {
            fprintf(stderr,
                    "You can not load %s from console and file at the same time.\n",
                    stream_name[i]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    int test_no = get_test_num(dir) + 1;
    printf("Test name: ");
    fflush(stdout);
    fgets(buffer, 128, stdin);
    sprintf(test_name, "%03d_%s", test_no, sanitize_test_name(buffer));

    printf("Creating test %s in %s\n", test_name, dir);

    for (i = 0; i < 3; i++) {
        char *path = get_filepath(dir, test_name, exts[i]);
        FILE *fh = fopen(path, "w");
        if (fh == NULL) {
            perror("Can not open test file");
            exit(EXIT_FAILURE);
        }
        FILE *from;
        if (files[i]) {
            from = fopen(files[i], "r");
            if (from == NULL) {
                perror("Can not open input file");
                exit(EXIT_FAILURE);
            }
        } else if (from_user[i]) {
            printf("Enter %s:\n", stream_name[i]);
            from = stdin;
        } else {
            continue;
        }

        copy_from_to(from, fh);
        if (files[i]) {
            fclose(from);
        }
        fclose(fh);
        free(path);
    }
    if (args) {
        char *path = get_filepath(dir, test_name, EXT_ARGS);
        FILE *fh = fopen(path, "w");
        if (fh == NULL) {
            perror("Can not open arguments file");
            exit(EXIT_FAILURE);
        }
        fprintf(fh, "%s\n", args);
        free(path);
        fclose(fh);
    }
    if (retval) {
        char *path = get_filepath(dir, test_name, EXT_RETVAL);
        FILE *fh = fopen(path, "w");
        if (fh == NULL) {
            perror("Can not open exit code file");
            exit(EXIT_FAILURE);
        }
        fprintf(fh, "%s\n", retval);
        free(path);
        fclose(fh);
    }

    return 0;
}
