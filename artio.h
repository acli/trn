/* artio.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT ART_POS artpos INIT(0);	/* byte position in article file */

EXT ART_LINE artline INIT(0);	/* current line number in article file */
EXT FILE* artfp INIT(NULL);	/* current article file pointer */
EXT ART_NUM openart INIT(0);	/* the article number we have open */

EXT char* artbuf;
EXT long artbuf_size;
EXT long artbuf_pos;
EXT long artbuf_seek;
EXT long artbuf_len;

#define WRAPPED_NL  '\003'
#define AT_NL(c) ((c) == '\n' || (c) == WRAPPED_NL)

EXT char wrapped_nl INIT(WRAPPED_NL);

#ifdef LINKART
EXT char* linkartname INIT(nullstr);/* real name of article for Eunice */
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void artio_init _((void));
FILE* artopen _((ART_NUM,ART_POS));
void artclose _((void));
int seekart _((ART_POS));
ART_POS tellart _((void));
char* readart _((char*,int));
void clear_artbuf _((void));
int seekartbuf _((ART_POS));
char* readartbuf _((bool_int));
