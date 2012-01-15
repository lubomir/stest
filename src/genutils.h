#ifndef GENUTILS_H
#define GENUTILS_H

/**
 * Strip leading and trailing whitespace. All other white space in the middle
 * is replaced by underscores. At most one successive underscore will be found
 * in the resulting string. This function modifies the passed string.
 *
 * @param name  name to sanitize
 * @return string with
 */
char * sanitize_test_name(char *name);

/**
 */
int get_test_num(const char *dirpath);

#endif /* end of include guard: GENUTILS_H */
