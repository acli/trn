/*
 * mempool.h
 */

/* memory pool numbers */
/* scoring rule text */
#define MP_SCORE1 0

/* scorefile cache */
#define MP_SCORE2 1

/* sathread.c storage */
#define MP_SATHREAD 2

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void mp_init _((void));
char* mp_savestr _((char*,int));
char* mp_malloc _((int,int));
void mp_free _((int));
