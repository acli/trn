/* test_utf.c - unit tests for rt-util.c
 * vi: set sw=4 ts=8 ai sm noet :
 */

#include <stdio.h>

#include "minunit.h"

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "INTERN.h"

int tests_run = 0;

#define NAME_MAX 29


static char *test_compress_name () {
    char *before = "School of the Art Institute of Chicago";
    char *after = strdup(before);
    char *expected = "School o t A I o C";
    compress_name(after, NAME_MAX);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, compress_name(\"School of the Art Institute of Chicago\") != \"School o t A I o C\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}


/* main loop */

static char *all_tests() {
    mu_run_test(test_compress_name);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
	printf("%s\n", result);
    } else {
	printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);
    return result != 0;
}
