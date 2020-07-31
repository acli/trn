/*
 * general-purpose in-core hashing
 */
/* This file is an altered version of a set of hash routines by
 * Geoffrey Collyer.  See hash.c for his copyright.
 */

struct hashdatum {
    char* dat_ptr;
    unsigned dat_len;
};

#define HASH_DEFCMPFUNC (int(*)_((char*,int,HASHDATUM)))NULL

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

HASHTABLE* hashcreate _((unsigned,int(*) _((char*,int,HASHDATUM))));
void hashdestroy _((HASHTABLE*));
void hashstore _((HASHTABLE*,char*,int,HASHDATUM));
void hashdelete _((HASHTABLE*,char*,int));
HASHDATUM hashfetch _((HASHTABLE*,char*,int));
void hashstorelast _((HASHDATUM));
void hashwalk _((HASHTABLE*,int(*) _((int,HASHDATUM*,int)),int));
