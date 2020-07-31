/* charsubst.h
 */
/*
 * Permission is hereby granted to copy, reproduce, redistribute or otherwise
 * use this software as long as: there is no monetary profit gained
 * specifically from the use or reproduction of this software, it is not
 * sold, rented, traded or otherwise marketed, and this copyright notice is
 * included prominently in any copy made. 
 *
 * The authors make no claims as to the fitness or correctness of this software
 * for any use whatsoever, and it is provided as is. Any use of this software
 * is at the user's own risk. 
 */

#ifdef CHARSUBST

/* Conversions are: plain, ISO->USascii, TeX->ISO, ISO->USascii monospaced */
EXT char* charsets INIT("patm");
EXT char* charsubst;

#define HEADER_CONV() (*charsubst=='a' || *charsubst=='m'? *charsubst : '\0')

#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int putsubstchar _((int,int,bool_int));
char* current_charsubst _((void));
int strcharsubst _((char*,char*,int,char_int));
