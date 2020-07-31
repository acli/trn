/* hash.c
*/
/* This file is an altered version of a set of hash routines by
** Geoffrey Collyer.  See the end of the file for his copyright.
*/

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "final.h"
#include "INTERN.h"
#include "hash.h"
#include "hash.ih"

/* CAA: In the future it might be a good idea to preallocate a region
 *      of hashents, since allocation overhead on some systems will be as
 *      large as the structure itself.
 */
/* grab this many hashents at a time (under 1024 for malloc overhead) */
#define HEBLOCKSIZE 1000

/* define the following if you actually want to free the hashents
 * You probably don't want to do this with the usual malloc...
 */
/* #define HASH_FREE_ENTRIES */

/* CAA: increased from 600.  Each threaded article requires at least
 *      one hashent, and various newsgroup hashes can easily get large.
 */
/* tunable parameters */
#define RETAIN 1000		/* retain & recycle this many HASHENTs */

static HASHENT* hereuse = NULL;
static int reusables = 0;

HASHTABLE*
hashcreate(size, cmpfunc)
unsigned size;			/* a crude guide to size */
int (*cmpfunc) _((char*,int,HASHDATUM));
{
    register HASHTABLE* tbl;
    /* allocate HASHTABLE and (HASHENT*) array together to reduce the
    ** number of malloc calls. */
    register struct alignalloc {
	HASHTABLE ht;
	HASHENT* hepa[1];	/* longer than it looks */
    }* aap;

    if (size < 1)		/* size < 1 is nonsense */
	size = 1;
    aap = (struct alignalloc*)
	safemalloc(sizeof *aap + (size-1)*sizeof (HASHENT*));
    bzero((char*)aap, sizeof *aap + (size-1)*sizeof (HASHENT*));
    tbl = &aap->ht;
    tbl->ht_size = size;
    tbl->ht_magic = HASHMAG;
    tbl->ht_cmp = (cmpfunc == NULL? default_cmp: cmpfunc);
    tbl->ht_addr = aap->hepa;
    return tbl;
}

/* Free all the memory associated with tbl, erase the pointers to it, and
** invalidate tbl to prevent further use via other pointers to it.
*/
void
hashdestroy(tbl)
register HASHTABLE* tbl;
{
    register unsigned idx;
    register HASHENT* hp;
    register HASHENT* next;
    register HASHENT** hepp;
    register int tblsize;

    if (BADTBL(tbl))
	return;
    tblsize = tbl->ht_size;
    hepp = tbl->ht_addr;
    for (idx = 0; idx < tblsize; idx++) {
	for (hp = hepp[idx]; hp != NULL; hp = next) {
	    next = hp->he_next;
	    hp->he_next = NULL;
	    hefree(hp);
	}
	hepp[idx] = NULL;
    }
    tbl->ht_magic = 0;			/* de-certify this table */
    tbl->ht_addr = NULL;
    free((char*)tbl);
}

void
hashstore(tbl, key, keylen, data)
register HASHTABLE* tbl;
char* key;
int keylen;
HASHDATUM data;
{
    register HASHENT* hp;
    register HASHENT** nextp;

    nextp = hashfind(tbl, key, keylen);
    hp = *nextp;
    if (hp == NULL) {			/* absent; allocate an entry */
	hp = healloc();
	hp->he_next = NULL;
	hp->he_keylen = keylen;
	*nextp = hp;			/* append to hash chain */
    }
    hp->he_data = data;		/* supersede any old data for this key */
}

void
hashdelete(tbl, key, keylen)
register HASHTABLE* tbl;
char* key;
int keylen;
{
    register HASHENT* hp;
    register HASHENT** nextp;

    nextp = hashfind(tbl, key, keylen);
    hp = *nextp;
    if (hp == NULL)			/* absent */
	return;
    *nextp = hp->he_next;		/* skip this entry */
    hp->he_next = NULL;
    hp->he_data.dat_ptr = NULL;
    hefree(hp);
}

HASHENT** slast_nextp;
int slast_keylen;

HASHDATUM				/* data corresponding to key */
hashfetch(tbl, key, keylen)
register HASHTABLE* tbl;
char* key;
int keylen;
{
    register HASHENT* hp;
    register HASHENT** nextp;
    static HASHDATUM errdatum = { NULL, 0 };

    nextp = hashfind(tbl, key, keylen);
    slast_nextp = nextp;
    slast_keylen = keylen;
    hp = *nextp;
    if (hp == NULL)			/* absent */
	return errdatum;
    return hp->he_data;
}

void
hashstorelast(data)
HASHDATUM data;
{
    register HASHENT* hp;

    hp = *slast_nextp;
    if (hp == NULL) {			/* absent; allocate an entry */
	hp = healloc();
	hp->he_next = NULL;
	hp->he_keylen = slast_keylen;
	*slast_nextp = hp;		/* append to hash chain */
    }
    hp->he_data = data;		/* supersede any old data for this key */
}

/* Visit each entry by calling nodefunc at each, with keylen, data,
** and extra as arguments.
*/
void
hashwalk(tbl, nodefunc, extra)
HASHTABLE* tbl;
register int (*nodefunc) _((int,HASHDATUM*,int));
register int extra;
{
    register unsigned idx;
    register HASHENT* hp;
    register HASHENT* next;
    register HASHENT** hepp;
    register int tblsize;

    if (BADTBL(tbl))
	return;
    hepp = tbl->ht_addr;
    tblsize = tbl->ht_size;
    for (idx = 0; idx < tblsize; idx++) {
	slast_nextp = &hepp[idx];
	for (hp = *slast_nextp; hp != NULL; hp = next) {
	    next = hp->he_next;
	    if ((*nodefunc)(hp->he_keylen, &hp->he_data, extra) < 0) {
		*slast_nextp = next;
		hp->he_next = NULL;
		hefree(hp);
	    }
	    else
		slast_nextp = &hp->he_next;
	}
    }
}

/* The returned value is the address of the pointer that refers to the
** found object.  Said pointer may be NULL if the object was not found;
** if so, this pointer should be updated with the address of the object
** to be inserted, if insertion is desired.
*/
static HASHENT**
hashfind(tbl, key, keylen)
register HASHTABLE* tbl;
char* key;
register int keylen;
{
    register HASHENT* hp;
    register HASHENT* prevhp = NULL;
    register HASHENT** hepp;
    register unsigned size; 

    if (BADTBL(tbl)) {
	fputs("Hash table is invalid.",stderr);
	finalize(1);
    }
    size = tbl->ht_size;
    hepp = &tbl->ht_addr[hash(key,keylen) % size];
    for (hp = *hepp; hp != NULL; prevhp = hp, hp = hp->he_next) {
	if (hp->he_keylen == keylen && !(*tbl->ht_cmp)(key, keylen, hp->he_data))
	    break;
    }
    /* assert: *(returned value) == hp */
    return (prevhp == NULL? hepp: &prevhp->he_next);
}

static unsigned				/* not yet taken modulus table size */
hash(key, keylen)
register char* key;
register int keylen;
{
    register unsigned hash = 0;

    while (keylen--)
	hash += *key++;
    return hash;
}

static int
default_cmp(key, keylen, data)
char* key;
int keylen;
HASHDATUM data;
{
    /* We already know that the lengths are equal, just compare the strings */
    return bcmp(key, data.dat_ptr, keylen);
}

static HASHENT*
healloc()				/* allocate a hash entry */
{
    register HASHENT* hp;

    if (hereuse == NULL) {
	int i;

	/* make a nice big block of hashents to play with */
	hp = (HASHENT*)safemalloc(HEBLOCKSIZE * sizeof (HASHENT));
	/* set up the pointers within the block */
	for (i = 0; i < HEBLOCKSIZE-1; i++)
	    (hp+i)->he_next = hp + i + 1;
	/* The last block is the end of the list */
	(hp+i)->he_next = NULL;
	hereuse = hp;		/* start of list is the first item */
	reusables += HEBLOCKSIZE;
    }

    /* pull the first reusable one off the pile */
    hp = hereuse;
    hereuse = hereuse->he_next;
    hp->he_next = NULL;			/* prevent accidents */
    reusables--;
    return hp;
}

static void
hefree(hp)				/* free a hash entry */
register HASHENT* hp;
{
#ifdef HASH_FREE_ENTRIES
    if (reusables >= RETAIN)		/* compost heap is full? */
	free((char*)hp);		/* yup, just pitch this one */
    else {				/* no, just stash for reuse */
	++reusables;
	hp->he_next = hereuse;
	hereuse = hp;
    }
#else
    /* always add to list */
    ++reusables;
    hp->he_next = hereuse;
    hereuse = hp;
#endif
}

/*
 * Copyright (c) 1992 Geoffrey Collyer
 * All rights reserved.
 * Written by Geoffrey Collyer.
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company, the Regents of the University of California, or
 * the Free Software Foundation.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits must appear in the documentation.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits must appear in the documentation.
 *
 * 4. This notice may not be removed or altered.
 */
