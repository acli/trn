/* test_mimeify.c - unit tests for mimeify.c
 * vi: set sw=4 ts=8 ai sm noet :
 */

#include <stdio.h>

#include "minunit.h"

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "mimeify-int.h"
#include "INTERN.h"

int tests_run = 0;

#define MIMEIFY_BIN "../mimeify"

/* mimeify executable */

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

/* mimeify_scan_input() */

static char *test_mimeify_scan_input__null_null_null () {
    mu_assert("error, mimeify_scan_input(NULL, NULL, NULL) == 0", mimeify_scan_input(NULL, NULL, NULL) != 0);
    return 0;
}

static char *test_mimeify_scan_input__devnull_null_null () {
    FILE *input = fopen("/dev/null", "r");
    mu_assert("internal error, fopen(\"/dev/null\") failed", input != NULL);
    mu_assert("error, mimeify_scan_input(\"/dev/null\", NULL, NULL) != 0", mimeify_scan_input(input, NULL, NULL) == 0);
    fclose(input);
    return 0;
}

static char *test_mimeify_scan_input__devnull_null_buf () {
    FILE *input = fopen("/dev/null", "r");
    mimeify_status_t buf;
    memset(&buf, 0xff, sizeof buf);
    mu_assert("internal error, fopen(\"/dev/null\") failed", input != NULL);
    mu_assert("internal error, memset failed", buf.head.has8bit && buf.head.maxbytelen && buf.body.has8bit && buf.body.maxbytelen);
    mu_assert("error, mimeify_scan_input(\"/dev/null\", &buf) != 0", mimeify_scan_input(input, NULL, &buf) == 0);
    mu_assert("error, after mimeify_scan_input(\"/dev/null\", &buf), buf.head.has8bit != 0", buf.head.has8bit == 0);
    mu_assert("error, after mimeify_scan_input(\"/dev/null\", &buf), buf.body.has8bit != 0", buf.body.has8bit == 0);
    mu_assert("error, after mimeify_scan_input(\"/dev/null\", &buf), buf.maxbytelen != 0", buf.head.maxbytelen == 0 && buf.body.maxbytelen == 0);
    fclose(input);
    return 0;
}

static char *test_mimeify_scan_input__one_short_header () {
    int fd[2];
    pid_t childpid;
    mimeify_status_t buf;
    pipe(fd);
    if ((childpid = fork()) == -1) {
	perror("fork");
	exit(1);
    }
    if (childpid == 0) {
	FILE *output = fdopen(fd[1], "w");
	close(fd[0]);
	fprintf(output, "From: a\n");
	fflush(output);
	exit(0);
    } else {
	FILE *input = fdopen(fd[0], "r");;
	close(fd[1]);
	mu_assert("internal error, fdopen failed", input != NULL);
	mu_assert("error, mimeify_scan_input(..., NULL, &buf) != 0", mimeify_scan_input(input, NULL, &buf) == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.head.has8bit != 0", buf.head.has8bit == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.body.has8bit != 0", buf.body.has8bit == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.head.maxbytelen != 7", buf.head.maxbytelen == 7);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.body.maxbytelen != 0", buf.body.maxbytelen == 0);
	fclose(input);
    }
    return 0;
}

static char *test_mimeify_scan_input__one_short_header_plus_one_8bit_char_in_head () {
    int fd[2];
    pid_t childpid;
    mimeify_status_t buf;
    pipe(fd);
    if ((childpid = fork()) == -1) {
	perror("fork");
	exit(1);
    }
    if (childpid == 0) {
	FILE *output = fdopen(fd[1], "w");
	close(fd[0]);
	fprintf(output, "From: á\n");
	fflush(output);
	exit(0);
    } else {
	FILE *input = fdopen(fd[0], "r");;
	close(fd[1]);
	mu_assert("internal error, fdopen failed", input != NULL);
	mu_assert("error, mimeify_scan_input(..., NULL, &buf) != 0", mimeify_scan_input(input, NULL, &buf) == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.head.has8bit != TRUE", buf.head.has8bit == TRUE);
	fclose(input);
    }
    return 0;
}

static char *test_mimeify_scan_input__one_short_header_plus_one_8bit_char_in_body () {
    int fd[2];
    pid_t childpid;
    mimeify_status_t buf;
    pipe(fd);
    if ((childpid = fork()) == -1) {
	perror("fork");
	exit(1);
    }
    if (childpid == 0) {
	FILE *output = fdopen(fd[1], "w");
	close(fd[0]);
	fprintf(output, "From: a\n");
	fprintf(output, "\né\n");
	fflush(output);
	exit(0);
    } else {
	FILE *input = fdopen(fd[0], "r");;
	close(fd[1]);
	mu_assert("internal error, fdopen failed", input != NULL);
	mu_assert("error, mimeify_scan_input(..., NULL, &buf) != 0", mimeify_scan_input(input, NULL, &buf) == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.body.has8bit != TRUE", buf.body.has8bit == TRUE);
	fclose(input);
    }
    return 0;
}

static char *test_mimeify_scan_input__one_short_header_with_continuation () {
    int fd[2];
    pid_t childpid;
    mimeify_status_t buf;
    pipe(fd);
    if ((childpid = fork()) == -1) {
	perror("fork");
	exit(1);
    }
    if (childpid == 0) {
	FILE *output = fdopen(fd[1], "w");
	close(fd[0]);
	fprintf(output, "From: a\n\tb");
	fflush(output);
	exit(0);
    } else {
	FILE *input = fdopen(fd[0], "r");;
	close(fd[1]);
	mu_assert("internal error, fdopen failed", input != NULL);
	mu_assert("error, mimeify_scan_input(..., NULL, &buf) != 0", mimeify_scan_input(input, NULL, &buf) == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.head.has8bit != 0", buf.head.has8bit == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.body.has8bit != 0", buf.body.has8bit == 0);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.head.maxbytelen != 9", buf.head.maxbytelen == 9);
	mu_assert("error, after mimeify_scan_input(..., NULL, &buf), buf.body.maxbytelen != 0", buf.body.maxbytelen == 0);
	fclose(input);
    }
    return 0;
}

/* main loop */

static char *all_tests() {
    mu_run_test(test_mimeify__empty_argv);
    mu_run_test(test_mimeify__one_argv);
    mu_run_test(test_mimeify__too_many_argv);

    mu_run_test(test_mimeify_scan_input__null_null_null);
    mu_run_test(test_mimeify_scan_input__devnull_null_null);
    mu_run_test(test_mimeify_scan_input__devnull_null_buf);
    mu_run_test(test_mimeify_scan_input__one_short_header);
    mu_run_test(test_mimeify_scan_input__one_short_header_plus_one_8bit_char_in_head);
    mu_run_test(test_mimeify_scan_input__one_short_header_plus_one_8bit_char_in_body);
    mu_run_test(test_mimeify_scan_input__one_short_header_with_continuation);
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
