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
    char *dir = "tests";
    unsigned int failed_checks;

    char c;

    TestContext *tc;
    List *tests;

    static const struct option long_options[] = {
        { "diff",    required_argument, NULL, 'd' },
        { "memory",  no_argument,       NULL, 'm' },
        { "verbose", no_argument,       NULL, 'v' },
        { "quiet",   no_argument,       NULL, 'q' },
        { "help",    no_argument,       NULL, 'h' },
        { "version", no_argument,       NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };

    tc = test_context_new ();

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, OPTSTRING, long_options, &option_index);
        if (c == -1) break;

        switch (c) {
        case 'h':
            help();
            return 0;
        case 'm':
            test_context_set_use_valgrind(tc);
            break;
        case 'v':
            test_context_set_verbosity(tc, MODE_VERBOSE);
            break;
        case 'q':
            test_context_set_verbosity(tc, MODE_QUIET);
            break;
        case 'V':
            printf("%s %s\n", PACKAGE_NAME, VERSION);
            return 0;
        case 'd':
            test_context_set_diff_opts(tc, optarg);
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

    if (!test_context_set_command(tc, argv[optind])) {
        exit(EXIT_FAILURE);
    }

    if (++optind < argc) {
        dir = argv[optind];
    }

    tests = test_load_from_dir(dir);
    if (tests == NULL) {
        fprintf(stderr, "No tests loaded from directory '%s'", dir);
        if (errno == 0) {
            fprintf(stderr, "\n");
        } else {
            perror("");
        }
        return 254;
    }

    failed_checks = test_context_run_tests(tc, tests);
    list_destroy(tests, DESTROYFUNC(test_free));
    test_context_free(tc);

    return failed_checks;
}
