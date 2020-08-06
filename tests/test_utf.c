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

static char *test_utf_init__in () {
    mu_assert("error, utf_init(NULL, ...) != CHARSET_UNKNOWN", utf_init(NULL, "utf8") == CHARSET_UNKNOWN);
    mu_assert("error, utf_init(\"ascii\", ...) != CHARSET_ASCII", utf_init("ascii", "utf8") == CHARSET_ASCII);
    mu_assert("error, utf_init(\"utf8\", ...) != CHARSET_UTF8", utf_init("utf8", "utf8") == CHARSET_UTF8);
    mu_assert("error, utf_init(\"US-ASCII\", ...) != CHARSET_ASCII", utf_init("US-ASCII", "utf8") == CHARSET_ASCII);
    mu_assert("error, utf_init(\"UTF-8\", ...) != CHARSET_UTF8", utf_init("UTF-8", "utf8") == CHARSET_UTF8);
    return 0;
}

static char *test_input_charset_name () {
    const char* charsets[] = {
	"us-ascii", TAG_ASCII,
	"ascii", TAG_ASCII,
	"iso8859-1", TAG_ISO8859_1,
	"WiNdOwS-1252", TAG_WINDOWS_1252,
	"utf8", TAG_UTF8,
	"UTF-8", TAG_UTF8,
	NULL, NULL };
    int i;
    for (i = 0; charsets[i] != NULL; i += 2) {
	const char* before = charsets[i];
	const char* after;
	const char* expected = charsets[i+1];
	utf_init(before, "utf8");
	after = input_charset_name();
	printf("Set: \"%s\"\n", before);
	printf("Got: \"%s\"\n", after);
	printf("Expected: \"%s\"\n", expected);
	mu_assert("error, after utf_init(before, \"utf8\"), input_charset_name() is not before", strcmp(expected, after) == 0);
    }
    return 0;
}

static char *test_output_charset_name () {
    const char* charsets[] = {
	"us-ascii", TAG_ASCII,
	"ascii", TAG_ASCII,
	"iso8859-1", TAG_ISO8859_1,
	"WiNdOwS-1252", TAG_WINDOWS_1252,
	"utf8", TAG_UTF8,
	"UTF-8", TAG_UTF8,
	NULL, NULL };
    int i;
    for (i = 0; charsets[i] != NULL; i += 2) {
	const char* before = charsets[i];
	const char* after;
	const char* expected = charsets[i+1];
	utf_init("utf8", before);
	after = output_charset_name();
	printf("Set: \"%s\"\n", before);
	printf("Got: \"%s\"\n", after);
	printf("Expected: \"%s\"\n", expected);
	mu_assert("error, after utf_init(before, \"utf8\"), input_charset_name() is not before", strcmp(expected, after) == 0);
    }
    return 0;
}

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
    mu_assert("error, byte_length_at(\"î\") != 2", byte_length_at("î") == 2);
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
    mu_assert("error, put_char_adv(NULL, FALSE) != 0", put_char_adv(NULL, FALSE) == 0);
    mu_assert("error, put_char_adv(NULL, TRUE) != 0",  put_char_adv(NULL, TRUE) == 0);
    return 0;
}

static char *test_put_char_adv__ascii () {
    char *sp0 = "a";
    char *sp = sp0;
    int retval = put_char_adv(&sp, TRUE);
    mu_assert("error, put_char_adv(&\"a\") -> retval != 1", retval == 1);
    mu_assert("error, put_char_adv(&\"a\") -> sp - sp0 != 1", sp - sp0 == 1);
    return 0;
}

static char *test_put_char_adv__iso8859_1 () {
    char *sp0 = "á";
    char *sp = sp0;
    int retval = put_char_adv(&sp, TRUE);
    mu_assert("error, put_char_adv(&\"á\") -> retval != 1", retval == 1);
    mu_assert("error, put_char_adv(&\"á\") -> sp - sp0 != 2", sp - sp0 == 2);
    return 0;
}

static char *test_put_char_adv__cjk_basic () {
    char *sp0 = "玄";
    char *sp = sp0;
    int retval = put_char_adv(&sp, TRUE);
    mu_assert("error, put_char_adv(&\"á\") -> retval != 2", retval == 2);
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

static char *test_visual_length_of__null () {
    mu_assert("error, visual_length_of(NULL) != 0", visual_length_of(NULL) == 0);
    return 0;
}

static char *test_visual_length_of__ascii () {
    mu_assert("error, visual_length_of(\"cat\") != 3", visual_length_of("cat") == 3);
    return 0;
}

static char *test_visual_length_of__iso_8859_1 () {
    mu_assert("error, visual_length_of(\"liberté\") != 7", visual_length_of("liberté") == 7);
    mu_assert("error, visual_length_of(\"bébé\") != 4", visual_length_of("bébé") == 4);
    mu_assert("error, visual_length_of(\"be\314\201be\314\201\") /* combining acute */ != 4", visual_length_of("be\314\201be\314\201") == 4);
    mu_assert("error, visual_length_of(\"\314\201\") /* combining acute */ != 0", visual_length_of("\314\201") == 0);
    return 0;
}

static char *test_visual_length_of__cjk () {
    mu_assert("error, visual_length_of(\"良心\") != 4", visual_length_of("良心") == 4);
    return 0;
}

/* insert utf */

static char *test_insert_unicode_at__null () {
    mu_assert("error, insert_unicode_at(NULL,...) != 0", insert_unicode_at(NULL, 0x40) == 0);
    return 0;
}

static char *test_insert_unicode_at__ascii () {
    char buf[8] = "\0\0\0\0\0\0\0";
    mu_assert("error, insert_unicode_at(..., 0x64) != 1", insert_unicode_at(buf, 0x64) == 1);
    mu_assert("error, insert_unicode_at(..., 0x64) did not set buf to \"d\"", strcmp(buf, "d") == 0);
    return 0;
}

static char *test_insert_unicode_at__iso_8859_1 () {
    char buf[8] = "\0\0\0\0\0\0\0";
    mu_assert("error, insert_unicode_at(..., 0xe4) != 2", insert_unicode_at(buf, 0xe4) == 2);
    mu_assert("error, insert_unicode_at(..., 0xe4) did not set buf to \"ä\"", strcmp(buf, "ä") == 0);
    return 0;
}

static char *test_insert_unicode_at__cjk_basic () {
    char buf[8] = "\0\0\0\0\0\0\0";
    mu_assert("error, insert_unicode_at(..., 0x4e00) != 3", insert_unicode_at(buf, 0x4e00) == 3);
    mu_assert("error, insert_unicode_at(..., 0x4e00) did not set buf to \"一\"", strcmp(buf, "一") == 0);
    return 0;
}

static char *test_insert_unicode_at__kissing_face_with_closed_eyes () {
    char buf[8] = "\0\0\0\0\0\0\0";
    mu_assert("error, insert_unicode_at(..., 0x1f61a) != 4", insert_unicode_at(buf, 0x1f61a) == 4);
    mu_assert("error, insert_unicode_at(..., 0x1f61a) did not set buf to \"😚\"", strcmp(buf, "😚") == 0);
    return 0;
}

/* create copy of string converted to utf8 */

static char *test_create_utf8_copy__null () {
    mu_assert("error, create_utf8_copy(NULL) != NULL", create_utf8_copy(NULL) == NULL);
    return 0;
}

static char *test_create_utf8_copy__ascii () {
    char *before = "Lorem ipsum";
    char *after = create_utf8_copy(before);
    mu_assert("error, create_utf8_copy of ASCII string returned NULL", before != NULL);
    mu_assert("error, create_utf8_copy of ASCII string did not alloc a new string", before != after);
    mu_assert("error, create_utf8_copy of ASCII string did not create an identical copy", strcmp(before, after) == 0);
    free(after);
    return 0;
}

static char *test_create_utf8_copy__iso8859_1 () {
    char *before = "Quoi, le biblioth\350que est ferm\351\240!";
    char *after;
    char *expected = "Quoi, le bibliothèque est fermé !";
    utf_init("iso8859-1", "utf8");
    after = create_utf8_copy(before);
    utf_init("utf8", "utf8");
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, create_utf8_copy of ISO-8859-1 string returned NULL", before != NULL);
    mu_assert("error, create_utf8_copy of ISO-8859-1 string did not alloc a new string", before != after);
    mu_assert("error, create_utf8_copy of ISO-8859-1 string did not create a corresponding UTF-8 copy", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

/* terminate string at nth visual column instead of at the nth byte */

static char *test_terminate_string_at_visual_index__null () {
    terminate_string_at_visual_index(NULL, 1);
    mu_assert("error, terminate_at_visual_index(NULL) should not crash", 1);
    return 0;
}

static char *test_terminate_string_at_visual_index__negative_index () {
    char *before = "abcdefg";
    char *after = strdup(before);
    char *expected = "";
    terminate_string_at_visual_index(after, -12345);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, terminate_at_visual_index(\"abcdefg\", -12345) != \"abcdefg\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_terminate_string_at_visual_index__ascii () {
    char *before = "abcdefg";
    char *after = strdup(before);;
    char *expected = "abcde";
    terminate_string_at_visual_index(after, 5);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, terminate_at_visual_index(\"abcdefg\", 5) != \"abcde\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_terminate_string_at_visual_index__iso8859_1 () {
    char *before = "áíúéóäïüëö";
    char *after = strdup(before);;
    char *expected = "áíúéó";
    terminate_string_at_visual_index(after, 5);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, terminate_at_visual_index(\"áíúéóäïüëö\", 5) != \"áíúéó\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_terminate_string_at_visual_index__cjk_basic () {
    char *before = "寧化飛灰不作浮塵";
    char *after = strdup(before);;
    char *expected = "寧化飛灰";
    terminate_string_at_visual_index(after, 8);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, terminate_at_visual_index(\"寧化飛灰不作浮塵\", 8) != \"寧化飛灰\"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

static char *test_terminate_string_at_visual_index__cjk_basic_at_wrong_boundary () {
    char *before = "寧化飛灰不作浮塵";
    char *after = strdup(before);;
    char *expected = "寧化飛灰 ";
    terminate_string_at_visual_index(after, 9);
    printf("Test %d:\n", tests_run);
    printf("Before   : \"%s\"\n", before);
    printf("After    : \"%s\"\n", after);
    printf("Expected : \"%s\"\n", expected);
    mu_assert("error, terminate_at_visual_index(\"寧化飛灰不作浮塵\", 9) != \"寧化飛灰 \"", strcmp(after, expected) == 0);
    free(after);
    return 0;
}

/* main loop */

static char *all_tests() {
    mu_run_test(test_utf_init__in);
    mu_run_test(test_input_charset_name);
    mu_run_test(test_output_charset_name);

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

    /* Put char -- advance pointer + return visual advance widths in character cell units */
    mu_run_test(test_put_char_adv__null);
    mu_run_test(test_put_char_adv__ascii);
    mu_run_test(test_put_char_adv__iso8859_1);
    mu_run_test(test_put_char_adv__cjk_basic);

    /* Code point decoding */
    mu_run_test(test_code_point_at__null);
    mu_run_test(test_code_point_at__sp);
    mu_run_test(test_code_point_at__5);
    mu_run_test(test_code_point_at__eth);
    mu_run_test(test_code_point_at__shin);
    mu_run_test(test_code_point_at__oy);
    mu_run_test(test_code_point_at__kissing_face_with_closed_eyes);

    /* Visual length of string */
    mu_run_test(test_visual_length_of__null);
    mu_run_test(test_visual_length_of__ascii);
    mu_run_test(test_visual_length_of__iso_8859_1);
    mu_run_test(test_visual_length_of__cjk);

    /* Code point to UTF */
    mu_run_test(test_insert_unicode_at__null);
    mu_run_test(test_insert_unicode_at__ascii);
    mu_run_test(test_insert_unicode_at__iso_8859_1);
    mu_run_test(test_insert_unicode_at__cjk_basic);
    mu_run_test(test_insert_unicode_at__kissing_face_with_closed_eyes);

    mu_run_test(test_create_utf8_copy__null);
    mu_run_test(test_create_utf8_copy__ascii);
    mu_run_test(test_create_utf8_copy__iso8859_1);

    mu_run_test(test_terminate_string_at_visual_index__null);
    mu_run_test(test_terminate_string_at_visual_index__negative_index);
    mu_run_test(test_terminate_string_at_visual_index__ascii);
    mu_run_test(test_terminate_string_at_visual_index__iso8859_1);
    mu_run_test(test_terminate_string_at_visual_index__cjk_basic);
    mu_run_test(test_terminate_string_at_visual_index__cjk_basic_at_wrong_boundary);
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
