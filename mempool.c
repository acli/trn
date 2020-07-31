/*
 * mempool.c
 */

#include "EXTERN.h"
#include "common.h"
#include "final.h"
#include "util.h"
#include "util2.h"
#include "INTERN.h"
#include "mempool.h"
#include "mempool.ih"

/* any of these defines can be increased arbitrarily */
#define MAX_MEM_POOLS 16
#define MAX_MEM_FRAGS 4000
/* number of bytes in a fragment */
#define FRAG_SIZE ((1024*64)-32)

typedef struct mp_frag {
    char* data;
    char* lastfree;
    long bytesfree;
    int next;
} MP_FRAG;

/* structure for extensibility */
typedef struct mp_head {
    int current;	/* index into mp_frag of most recent alloc */
} MP_HEAD;

static MP_FRAG mpfrags[MAX_MEM_FRAGS]; /* zero is unused */
static int mp_first_free_frag;

static MP_HEAD mpheads[MAX_MEM_POOLS];

void
mp_init()
{
    int i;

    for (i = 1; i < MAX_MEM_FRAGS; i++) {
	mpfrags[i].data = NULL;
	mpfrags[i].lastfree = NULL;
	mpfrags[i].bytesfree = 0;
	mpfrags[i].next = i+1;
    }
    mpfrags[i-1].next = 0;
    mp_first_free_frag = 1;	/* first free fragment */

    for (i = 0; i < MAX_MEM_POOLS; i++)
	mpheads[i].current = 0;
}

/* returns the fragment number */
static int
mp_alloc_frag()
{
    int f;

    f = mp_first_free_frag;
    if (f == 0) {
	printf("trn: out of memory pool fragments!\n");
	sig_catcher(0);		/* die. */
    }
    mp_first_free_frag = mpfrags[f].next;
    if (mpfrags[f].bytesfree)
	return f;	/* already allocated */
    mpfrags[f].data = (char*)safemalloc(FRAG_SIZE);
    mpfrags[f].lastfree = mpfrags[f].data;
    mpfrags[f].bytesfree = FRAG_SIZE;

    return f;
}

/* frees a fragment number */
static void
mp_free_frag(f)
int f;
{
#if 0
    /* old code to actually free the blocks */
    if (mpfrags[f].data)
	free(mpfrags[f].data);
    mpfrags[f].lastfree = NULL;
    mpfrags[f].bytesfree = 0;
#else
    /* instead of freeing it, reset it for later use */
    mpfrags[f].lastfree = mpfrags[f].data;
    mpfrags[f].bytesfree = FRAG_SIZE;
#endif
    mpfrags[f].next = mp_first_free_frag;
    mp_first_free_frag = f;
}

char*
mp_savestr(str,pool)
char* str;
int pool;
{
    int f, oldf;
    int len;
    char* s;

    if (!str) {
#if 1
	printf("\ntrn: mp_savestr(NULL,%d) error.\n",pool);
	assert(FALSE);
#else
	return NULL;		/* only a flesh wound... (;-) */
#endif
    }
    len = strlen(str);
    if (len >= FRAG_SIZE) {
	printf("trn: string too big (len = %d) for memory pool!\n",len);
	printf("trn: (maximum length allowed is %d)\n",FRAG_SIZE);
	sig_catcher(0);		/* die. */
    }
    f = mpheads[pool].current;
    /* just to be extra safe, keep 2 bytes unused at end of block */
    if (f == 0 || len >= mpfrags[f].bytesfree-2) {
	oldf = f;
	f = mp_alloc_frag();
	mpfrags[f].next = oldf;
	mpheads[pool].current = f;
    }
    s = mpfrags[f].lastfree;
    safecpy(s,str,len+1);
    mpfrags[f].lastfree += len+1;
    mpfrags[f].bytesfree -= len+1;
    return s;
}

/* returns a pool-allocated string */
char*
mp_malloc(len,pool)
int len;
int pool;
{
    int f,oldf;
    char* s;

    if (len == 0)
	len = 1;
    if (len >= FRAG_SIZE) {
	printf("trn: malloc size too big (len = %d) for memory pool!\n",len);
	printf("trn: (maximum length allowed is %d)\n",FRAG_SIZE);
	sig_catcher(0);		/* die. */
    }
    f = mpheads[pool].current;
    if (f == 0 || len >= mpfrags[f].bytesfree) {
	oldf = f;
	f = mp_alloc_frag();
	mpfrags[f].next = oldf;
	mpheads[pool].current = f;
    }
    s = mpfrags[f].lastfree;
    mpfrags[f].lastfree += len+1;
    mpfrags[f].bytesfree -= len+1;
    return s;
}

/* free a whole memory pool */
void
mp_free(pool)
int pool;
{
    int oldnext;
    int f;

    f = mpheads[pool].current;
    while (f) {
	oldnext = mpfrags[f].next;
	mp_free_frag(f);
	f = oldnext;
    }
    mpheads[pool].current = 0;
}
