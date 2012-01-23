#include <config.h>

#include "genutils.h"
#include "utils.h"
#include "test.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static const char *exts[] = { EXT_INPUT, EXT_OUTPUT, EXT_ERRORS };

/**
 * Print information about a test.
 */
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

/**
 * List all tests in a directory.
 *
 * @param dir   directory to list
 */
static void
list_tests(const char *dir)
{
    List *tests = test_load_from_dir(dir);
    list_foreach(tests, print_test, NULL);
    list_destroy(tests, DESTROYFUNC(test_free));
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

/**
 * Create a file and save a string into it.
 *
 * @param dir   directory for the file
 * @param name  base name of the file
 * @param ext   extension of the file
 * @param value string to be stored
 * @param msg   information about file, for error message
 */
static void
print_to_file(const char *dir, const char *name, const char *ext,
              const char *value, const char *msg)
{
    char *path = get_filepath(dir, name, ext);
    FILE *fh = fopen(path, "w");
    if (fh == NULL) {
        fprintf(stderr, "Can not open %s file '%s'", msg, path);
        perror("");
        exit(EXIT_FAILURE);
    }
    fprintf(fh, "%s\n", value);
    free(path);
    fclose(fh);
}

/**
 * Check if arguments are consistent and print error message for each
 * inconsistency.
 *
 * @param from_user array of 3 ints indicating whether to load from console
 * @param files     array of 3 filenames to load
 * @return number of failed checks
 */
static int
check_consistency(int *from_user, char **files)
{
    int i, count = 0;
    for (i = 0; i < 3; i++) {
        if (files[i] != NULL && from_user[i]) {
            fprintf(stderr,
                    "You can not load std%s from console and file at the same time.\n",
                    exts[i]);
            count++;
        }
    }
    return count;
}

static void
create_file(const char *dir, const char *name, const char *ext,
        const char *file, int from_user)
{
    char *path;
    FILE *fh, *from;

    if (!file && !from_user) return;

    path = get_filepath(dir, name, ext);
    if ((fh = fopen(path, "w")) == NULL) {
        perror("Can not open test file");
        exit(EXIT_FAILURE);
    }
    if (file) {
        if ((from = fopen(file, "r")) == NULL) {
            perror("Can not open input file");
            exit(EXIT_FAILURE);
        }
    } else if (from_user) {
        printf("Enter std%s:\n", ext);
        from = stdin;
    }

    copy_from_to(from, fh);

    if (file)
        fclose(from);
    fclose(fh);
    free(path);
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

int main(int argc, char *argv[])
{
    char c;
    int i, opt_list_tests = 0;
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
            opt_list_tests = 1;
            break;
        case 'h':
            help();
            return EXIT_SUCCESS;
        case 'V':
            printf("%s %s\n", PACKAGE_NAME, VERSION);
            return EXIT_SUCCESS;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (optind < argc) {
        dir = argv[optind];
    }

    if (opt_list_tests) {
        list_tests(dir);
        return EXIT_SUCCESS;
    }

    if (check_consistency(from_user, files) > 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!(args || retval ||
                files[0] || files[1] || files[2] ||
                from_user[0] || from_user[1] || from_user[2])) {
        printf("No action requested... exiting\n");
        return EXIT_SUCCESS;
    }

    int test_no = get_test_num(dir) + 1;
    printf("Test name: ");
    fflush(stdout);
    fgets(buffer, 128, stdin);
    sprintf(test_name, "%03d_%s", test_no, sanitize_test_name(buffer));

    printf("Creating test %s in %s\n", test_name, dir);

    for (i = 0; i < 3; i++) {
        create_file(dir, test_name, exts[i], files[i], from_user[i]);
    }
    if (args)   print_to_file(dir, test_name, EXT_ARGS, args, "arguments");
    if (retval) print_to_file(dir, test_name, EXT_RETVAL, retval, "exit code");

    return EXIT_SUCCESS;
}
