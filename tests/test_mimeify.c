/* test_mimeify.c - unit tests for mimeify.c
 * vi: set sw=4 ts=8 ai sm noet :
 */

#include <stdio.h>

#include "minunit.h"

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "INTERN.h"

int tests_run = 0;

#define MIMEIFY_BIN "../mimeify"

static char *test_mimeify__empty_argv () {
    int st = system(MIMEIFY_BIN);
    mu_assert("error, system(\"" MIMEIFY_BIN "\") == 0", st != 0);
    return 0;
}

static char *test_mimeify__one_argv () {
    int st = system(MIMEIFY_BIN " /dev/null");
    mu_assert("error, system(\"" MIMEIFY_BIN " /dev/null\") != 0", st == 0);
    return 0;
}

static char *test_mimeify__too_many_argv () {
    int st = system(MIMEIFY_BIN " /dev/null /dev/null");
    mu_assert("error, system(\"" MIMEIFY_BIN " /dev/null /dev/null\") == 0", st != 0);
    return 0;
}


/* main loop */

static char *all_tests() {
    mu_run_test(test_mimeify__empty_argv);
    mu_run_test(test_mimeify__one_argv);
    mu_run_test(test_mimeify__too_many_argv);
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
