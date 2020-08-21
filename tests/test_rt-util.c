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


static char *test_compress_name__SAIC () {
    char *before = "School of the Art Institute of Chicago";
    char *after = strdup(before);
    char *expected = "School o t A I o Chicago";
    compress_name(after, NAME_MAX);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, compress_name(\"School of the Art Institute of Chicago\") != \"School o t A I o Chicago\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_compress_name__PCS () {
    char *before = "IEEE Professional Communication Society";
    char *after = strdup(before);
    char *expected = "IEEE P C Society";
    compress_name(after, NAME_MAX);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, compress_name(\"IEEE Professional Communication Society\") != \"IEEE P C Society\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_subject_has_Re__1 () {
    char *before = "Re: followup";
    char *after;
    char *expected = "followup";
    subject_has_Re(before, &after);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, \"followup\" != \"followup\"", strcmp(after, expected) == 0);
    return 0;
}


/* main loop */

static char *all_tests() {
    mu_run_test(test_subject_has_Re__1);
    mu_run_test(test_compress_name__SAIC);
    mu_run_test(test_compress_name__PCS);
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
