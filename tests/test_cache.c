/* test_cache.c - unit tests for the changed portions of cache.c
 * vi: set sw=4 ts=8 ai sm noet :
 */

#include <stdio.h>
#include <string.h>

#include "minunit.h"

#include "EXTERN.h"
#include "common.h"
#include "intrp.h"
#include "hash.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "term.h"
#include "cache.h"
#include "util.h"
#include "utf.h"
#include "INTERN.h"

int tests_run = 0;

static char *test_dectrl__null () {
    dectrl(NULL);
    mu_assert("error, dectrl(NULL) should not crash", 1);
    return 0;
}

static char *test_dectrl__ascii_all_printable () {
    char *before = "This is a test.";
    char *after = strdup(before);
    dectrl(after);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("\n");
    mu_assert("error, dectrl changed an ASCII string with all-printable characters", strcmp(before, after) == 0);
    free(after);
    return 0;
}

static char *test_dectrl__ascii_some_nonprintable () {
    char *before = "This\tis\fa\177test.";
    char *after = strdup(before);
    char *expected = "This is a test.";
    dectrl(after);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    printf("\n");
    mu_assert("error, dectrl did not change an ASCII string with some nonprintable characters", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_dectrl__iso_8859_1 () {
    char *before = "« La liberté guide le peuple. »";
    char *after = strdup(before);
    dectrl(after);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("\n");
    mu_assert("error, dectrl changed an ISO8859-1 string with all-printable characters", strcmp(before, after) == 0);
    free(after);
    return 0;
}

static char *test_dectrl__cjk_basic () {
    char *before = "寧化飛灰，不作瓦全";
    char *after = strdup(before);
    dectrl(after);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("\n");
    mu_assert("error, dectrl changed a CJK string with all-printable characters", strcmp(before, after) == 0);
    free(after);
    return 0;
}

static char *all_tests() {
    mu_run_test(test_dectrl__null);
    mu_run_test(test_dectrl__ascii_all_printable);
    mu_run_test(test_dectrl__ascii_some_nonprintable);
    mu_run_test(test_dectrl__iso_8859_1);
    mu_run_test(test_dectrl__cjk_basic);
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
