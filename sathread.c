/* This file Copyright 1992 by Clifford A. Adams */
/* sathread.c
 *
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN_ART
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"	/* for absfirst */
#include "head.h"	/* hc_setspin() */
#include "ngdata.h"
#include "mempool.h"
#include "scanart.h"
#include "sadesc.h"	/* sa_desc_subject() */
#include "samain.h"
#include "samisc.h"
#include "sorder.h"
#include "util.h"
#include "INTERN.h"
#include "sathread.h"

static long sa_num_threads = 0;
static HASHTABLE* sa_thread_hash = 0;

void
sa_init_threads()
{
    mp_free(MP_SATHREAD);
    sa_num_threads = 0;
    if (sa_thread_hash) {
	hashdestroy(sa_thread_hash);
	sa_thread_hash = 0;
    }
}

/* called only if the macro didn't find a value */
/* XXX: dependent on hash feature that data.dat_len is not used in
 * the default comparison function, so it can be used for a number.
 * later: write a custom comparison function.
 */
long
sa_get_subj_thread(e)
long e;			/* entry number */
{
    HASHDATUM data;
    char* s;
    bool old_untrim;
    char* p;

    old_untrim = untrim_cache;
    untrim_cache = TRUE;
    s = sa_desc_subject(e);
    untrim_cache = old_untrim;

    if (!s || !*s)
      return -2;
    if ((*s == '>') && (s[1] == ' '))
	s += 2;

    if (!sa_thread_hash) {
	sa_thread_hash = hashcreate(401, HASH_DEFCMPFUNC);
    }
    data = hashfetch(sa_thread_hash,s,strlen(s));
    if (data.dat_ptr) {
	return (long)(data.dat_len);
    }
    p = mp_savestr(s,MP_SATHREAD);
    data = hashfetch(sa_thread_hash,p,strlen(s));
    data.dat_ptr = p;
    data.dat_len = (unsigned)(sa_num_threads+1);
    hashstorelast(data);
    sa_num_threads++;
    sa_ents[e].subj_thread_num = sa_num_threads;
    return sa_num_threads;
}

int
sa_subj_thread_count(a)
long a;
{
    int count;
    long b;

    count = 1;
    b = a;

    while ((b = sa_subj_thread_next(b)) != 0)
	if (sa_basic_elig(b))
	    count++;
    return count;
}

/* returns basic_elig previous subject thread */
long
sa_subj_thread_prev(a)
long a;
{
    int i,j;

    i = sa_subj_thread(a);
    while ((a = s_prev(a)) != 0) {
	if (!sa_basic_elig(a))
	    continue;
	if (!(j = sa_ents[a].subj_thread_num))
	    j = sa_subj_thread(a);
	if (i == j)
	    return a;
    }
    return 0L;
}

long
sa_subj_thread_next(a)
long a;
{
    int i,j;

    i = sa_subj_thread(a);
    while ((a = s_next(a)) != 0) {
	if (!sa_basic_elig(a))
	    continue;
	if (!(j = sa_ents[a].subj_thread_num))
	    j = sa_subj_thread(a);
	if (i == j)
	    return a;
    }
    return 0L;
}

#endif /* SCAN */
