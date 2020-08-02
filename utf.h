/* utf.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef UTF_H_INCLUDED
#define UTF_H_INCLUDED

#define USE_UTF_HACK

bool at_norm_char(const char *);

int byte_length_at(const char *);
int visual_width_at(const char *);
int visual_length_of(const char *);
int insert_utf8_at(char *, unsigned long);

#define INVALID_CODE_POINT ((unsigned long)(~0))
unsigned long code_point_at(const char *);

int put_char_adv(char **);

#endif
