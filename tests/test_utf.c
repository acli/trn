/* test_utf.c - unit tests for utf.c
 * vi: set sw=4 ts=8 ai sm noet :
 */

#include <stdio.h>

#include "minunit.h"

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "INTERN.h"

int tests_run = 0;

/* byte length */

static char *test_byte_length_at__null () {
    mu_assert("error, byte_length_at(NULL) != 0", byte_length_at(NULL) == 0);
    return 0;
}

static char *test_byte_length_at__ascii () {
    mu_assert("error, byte_length_at(\"a\") != 1", byte_length_at("a") == 1);
    return 0;
}

static char *test_byte_length_at__iso8859_1 () {
    mu_assert("error, byte_length_at(\"á\") != 2", byte_length_at("á") == 2);
    return 0;
}

static char *test_byte_length_at__cjk_basic () {
    mu_assert("error, byte_length_at(\"天\") != 3", byte_length_at("天") == 3);
    return 0;
}

/* at "normal" character */

static char *test_at_norm_char__null () {
    mu_assert("error, at_norm_char(NULL) != 0", at_norm_char(NULL) == 0);
    return 0;
}

static char *test_at_norm_char__soh () {
    mu_assert("error, at_norm_char(\"\\001\") != 0", at_norm_char("\001") == 0);
    return 0;
}

static char *test_at_norm_char__sp () {
    mu_assert("error, at_norm_char(\" \") != 1", at_norm_char(" ") == 1);
    return 0;
}

static char *test_at_norm_char__tilde () {
    mu_assert("error, at_norm_char(\"~\") != 1", at_norm_char("~") == 1);
    return 0;
}

static char *test_at_norm_char__del () {
    mu_assert("error, at_norm_char(\"\\177\") != 0", at_norm_char("\177") == 0);
    return 0;
}

static char *test_at_norm_char__iso8859_1 () {
    mu_assert("error, at_norm_char(\"á\") != 1", at_norm_char("á") == 1);
    return 0;
}

static char *test_at_norm_char__cjk_basic () {
    mu_assert("error, at_norm_char(\"地\") != 1", at_norm_char("地") == 1);
    return 0;
}

/* visual advance width */

static char *test_put_char_adv__null () {
    mu_assert("error, put_char_adv(NULL) != 0", put_char_adv(NULL) == 0);
    return 0;
}

static char *test_put_char_adv__ascii__retval () {
    char *sp0 = "a";
    char *sp = sp0;
    int retval = put_char_adv(&sp);
    mu_assert("error, put_char_adv(&\"a\") -> retval != 1", retval == 1);
    return 0;
}

static char *test_put_char_adv__iso8859_1__retval () {
    char *sp0 = "á";
    char *sp = sp0;
    int retval = put_char_adv(&sp);
    mu_assert("error, put_char_adv(&\"á\") -> retval != 1", retval == 1);
    return 0;
}

static char *test_put_char_adv__cjk_basic__retval () {
    char *sp0 = "玄";
    char *sp = sp0;
    int retval = put_char_adv(&sp);
    mu_assert("error, put_char_adv(&\"á\") -> retval != 2", retval == 2);
    return 0;
}

/* byte advance width */

static char *test_put_char_adv__ascii__sp () {
    char *sp0 = "a";
    char *sp = sp0;
    int retval = put_char_adv(&sp);
    mu_assert("error, put_char_adv(&\"a\") -> sp - sp0 != 1", sp - sp0 == 1);
    return 0;
}

static char *test_put_char_adv__iso8859_1__sp () {
    char *sp0 = "á";
    char *sp = sp0;
    int retval = put_char_adv(&sp);
    mu_assert("error, put_char_adv(&\"á\") -> sp - sp0 != 2", sp - sp0 == 2);
    return 0;
}

static char *test_put_char_adv__cjk_basic__sp () {
    char *sp0 = "玄";
    char *sp = sp0;
    int retval = put_char_adv(&sp);
    mu_assert("error, put_char_adv(&\"á\") -> sp - sp0 != 3", sp - sp0 == 3);
    return 0;
}

/* code point decoding */

static char *test_code_point_at__null () {
    mu_assert("error, code_point_at(NULL) != INVALID_CODE_POINT", code_point_at(NULL) == INVALID_CODE_POINT);
    return 0;
}

static char *test_code_point_at__sp () {
    mu_assert("error, code_point_at(\" \") != 0x20", code_point_at(" ") == 0x20);
    return 0;
}

static char *test_code_point_at__5 () {
    mu_assert("error, code_point_at(\"5\") != 0x35", code_point_at("5") == 0x35);
    return 0;
}

static char *test_code_point_at__eth () {
    mu_assert("error, code_point_at(\"ð\") != 0xF0", code_point_at("ð") == 0xF0);
    return 0;
}

static char *test_code_point_at__shin () {
    mu_assert("error, code_point_at(\"ש\") != 0x05E9", code_point_at("ש") == 0x05E9);
    return 0;
}

static char *test_code_point_at__oy () {
    mu_assert("error, code_point_at(\"ᢰ\") != 0x18B0", code_point_at("ᢰ") == 0x18B0);
    return 0;
}

static char *test_code_point_at__kissing_face_with_closed_eyes () {
    mu_assert("error, code_point_at(\"😚\") != 0x1F61A", code_point_at("😚") == 0x1F61A);
    return 0;
}


static char *all_tests() {
    /* Number of bytes taken by the character at the beginning of the string */
    mu_run_test(test_byte_length_at__null);
    mu_run_test(test_byte_length_at__ascii);
    mu_run_test(test_byte_length_at__iso8859_1);
    mu_run_test(test_byte_length_at__cjk_basic);

    /* Whether the character at the beginning of the string is "normal" */
    mu_run_test(test_at_norm_char__null);
    mu_run_test(test_at_norm_char__soh);
    mu_run_test(test_at_norm_char__sp);
    mu_run_test(test_at_norm_char__del);
    mu_run_test(test_at_norm_char__iso8859_1);
    mu_run_test(test_at_norm_char__cjk_basic);

    /* Advance widths in character cell units */
    mu_run_test(test_put_char_adv__null);
    mu_run_test(test_put_char_adv__ascii__sp);
    mu_run_test(test_put_char_adv__iso8859_1__sp);
    mu_run_test(test_put_char_adv__cjk_basic__sp);

    /* Byte offsets in strings */
    mu_run_test(test_put_char_adv__ascii__retval);
    mu_run_test(test_put_char_adv__iso8859_1__retval);
    mu_run_test(test_put_char_adv__cjk_basic__retval);

    /* Code point decoding */
    mu_run_test(test_code_point_at__null);
    mu_run_test(test_code_point_at__sp);
    mu_run_test(test_code_point_at__5);
    mu_run_test(test_code_point_at__eth);
    mu_run_test(test_code_point_at__shin);
    mu_run_test(test_code_point_at__oy);
    mu_run_test(test_code_point_at__kissing_face_with_closed_eyes);
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
