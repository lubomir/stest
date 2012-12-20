#include <config.h>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/stat.h>

#include "list.h"
#include "test.h"
#include "testcontext.h"
#include "utils.h"

#define OPTSTRING "hmvqV"

static void usage(const char *progname)
{
    printf("Usage: %s [%s] COMMAND [TESTDIR]\n", progname, OPTSTRING);
}

static void help(void)
{
    puts("SYNOPSIS");
    puts("\tstest ["OPTSTRING"] COMMAND [TESTDIR]");

    puts("\nOPTIONS");
    puts("\t    --diff=OPTIONS\n\t\tpass OPTIONS to diff; see man diff(1)"
           " for available options\n");
    puts("\t-h, --help\n\t\tdisplay this help\n");
    puts("\t-m, --memory\n\t\trun Valgrind memory checking tool\n");
    puts("\t-v, --verbose\n\t\tdisplay output diff of failed tests\n");
    puts("\t-q, --quiet\n\t\tsuppress all output\n");
    puts("\t-V, --version\n\t\tdisplay version info\n");
    puts("\tIf TESTDIR is not specified, use ./tests directory.");

    puts("\nRETURN VALUE");
    puts("\tOn success of all tests return 0, otherwise exit with number of");
    puts("\tfailed tests. If options are bad, other return values are possible:");
    puts("\t253\tbad command name");
    puts("\t254\tbad directory with tests");
    puts("\t255\tunrecognized options");
}

int main(int argc, char *argv[])
{
    struct stat info;
    char *cmd, *dir = "tests";
    unsigned int failed_checks;

    int use_valgrind = 0;
    int verbosity_level = MODE_NORMAL;
    char *diff_opts = NULL;
    char c;

    static const struct option long_options[] = {
        { "diff",    required_argument, NULL, 'd' },
        { "memory",  no_argument,       NULL, 'm' },
        { "verbose", no_argument,       NULL, 'v' },
        { "quiet",   no_argument,       NULL, 'q' },
        { "help",    no_argument,       NULL, 'h' },
        { "version", no_argument,       NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, OPTSTRING, long_options, &option_index);
        if (c == -1) break;

        switch (c) {
        case 'h':
            help();
            return 0;
        case 'm':
            use_valgrind = 1;
            break;
        case 'v':
            verbosity_level = MODE_VERBOSE;
            break;
        case 'q':
            verbosity_level = MODE_QUIET;
            break;
        case 'V':
            printf("%s %s\n", PACKAGE_NAME, VERSION);
            return 0;
        case 'd':
            diff_opts = optarg;
            break;
        default:
            usage(argv[0]);
            return 255;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Missing command name\n");
        return 253;
    }
    cmd = argv[optind];
    if (++optind < argc) {
        dir = argv[optind];
    }

    List *tests = test_load_from_dir(dir);
    if (tests == NULL) {
        fprintf(stderr, "No tests loaded from directory '%s'", dir);
        if (errno == 0) {
            fprintf(stderr, "\n");
        } else {
            perror("");
        }
        return 254;
    }

    if (stat(cmd, &info) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    if (!FLAG_SET(info.st_mode, S_IXUSR)) {
        fprintf(stderr, "Can not execute '%s'\n", cmd);
        exit(EXIT_FAILURE);
    }

    TestContext *tc = test_context_new(cmd, dir, use_valgrind, diff_opts);
    failed_checks = test_context_run_tests(tc, tests, verbosity_level);
    list_destroy(tests, DESTROYFUNC(test_free));
    test_context_free(tc);

    return failed_checks;
}
