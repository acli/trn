/* decode.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT char* decode_filename INIT(NULL);

#define DECODE_DONE	 0
#define DECODE_START	 1
#define DECODE_INACTIVE  2
#define DECODE_SETLEN	 3
#define DECODE_ACTIVE	 4
#define DECODE_NEXT2LAST 5
#define DECODE_LAST	 6
#define DECODE_MAYBEDONE 7
#define DECODE_ERROR	 8

#ifdef MSDOS
#define GOODCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" \
                  "0123456789-_^#%"
#else
#define BADCHARS "!$&*()|\'\";<>[]{}?/`\\ \t"
#endif

typedef int (*DECODE_FUNC) _((FILE*,int));

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void decode_init _((void));
char* decode_fix_fname _((char*));
char* decode_subject _((ART_NUM,int*,int*));
int decode_piece _((MIMECAP_ENTRY*,char*));
DECODE_FUNC decode_function _((int));
char* decode_mkdir _((char*));
void decode_rmdir _((char*));
