/* utf.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef UTF_H_INCLUDED
#define UTF_H_INCLUDED

#define USE_UTF_HACK

bool at_norm_char(const char *);

int byte_length_at(const char *);

int put_char_adv(char **);

#endif