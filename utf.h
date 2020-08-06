/* utf.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef UTF_H_INCLUDED
#define UTF_H_INCLUDED

#define USE_UTF_HACK

#define CHARSET_ASCII		0x0000
#define CHARSET_UTF8		0x8000
#define CHARSET_ISO8859_1	0x4010
#define CHARSET_ISO8859_15	0x4011
#define CHARSET_WINDOWS_1252	0x4020
#define CHARSET_UNKNOWN		0x0FFF

#define TAG_ASCII		"US"
#define TAG_UTF8		"UTF"
#define TAG_ISO8859_1		"Latin1"
#define TAG_ISO8859_15		"Latin9"
#define TAG_WINDOWS_1252	"CP1252"

typedef unsigned long CODE_POINT;

int utf_init(const char *, const char *);
const char *input_charset_name();
const char *output_charset_name();

bool at_norm_char(const char *);

int byte_length_at(const char *);
int visual_width_at(const char *);
int visual_length_of(const char *);
int visual_length_between(const char *, const char *);
int insert_unicode_at(char *, CODE_POINT);

#define INVALID_CODE_POINT ((CODE_POINT) ~0L)
CODE_POINT code_point_at(const char *);

int put_char_adv(char **, bool_int);

char *create_utf8_copy(char *);

#endif
