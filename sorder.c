/* This file Copyright 1993 by Clifford A. Adams */
/* sorder.c
 *
 * scan ordering
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN
#include "util.h"
#include "scan.h"
#include "smisc.h"
#ifdef SCAN_ART
#include "scanart.h"
#include "samisc.h"
#endif
#include "INTERN.h"
#include "sorder.h"

#ifdef UNDEF
int
s_compare(a,b)
long* a;
long* b;		/* pointers to the two entries to be compared */
 {
    switch(s_cur_type) {
#ifdef SCAN_ART
      case S_ART:
	return sa_compare(*a,*b);
#endif
      default:
	return *a - *b;
    }
}
#endif

int
s_compare(a,b)
long a;
long b;		/* the two entry numbers to be compared */
{
    switch(s_cur_type) {
#ifdef SCAN_ART
      case S_ART:
	return sa_compare(a,b);
#endif
      default:
	return a - b;
    }
}

/* sort offset--used so that the 1-offset algorithm is clear even
 * though the array is 0-offset.
 */
#define SOFF(a) ((a)-1)

/* Uses a heapsort algorithm with the heap readjustment inlined. */
void
s_sort_basic()
{
    int i,n;
    int t1;
    int j;

    n = s_ent_sort_max + 1;
    if (n < 1)
	return;		/* nothing to sort */

    for (i = n/2; i >= 1; i--) {
	/* begin heap readjust */
	t1 = s_ent_sort[SOFF(i)];
	j = 2*i;
	while (j <= n) {
	    if (j < n
	     &&	s_compare(s_ent_sort[SOFF(j)],s_ent_sort[SOFF(j+1)]) < 0)
		j++;
	    if (s_compare(t1,s_ent_sort[SOFF(j)]) > 0)
		break;		/* out of while loop */
	    else {
		s_ent_sort[SOFF(j/2)] = s_ent_sort[SOFF(j)];
		j = j*2;
	    }
	} /* while */
	s_ent_sort[SOFF(j/2)] = t1;
	/* end heap readjust */
    } /* for */

    for (i = n-1; i >= 1; i--) {
	t1 = s_ent_sort[SOFF(i+1)];
	s_ent_sort[SOFF(i+1)] = s_ent_sort[SOFF(1)];
	s_ent_sort[SOFF(1)] = t1;
	/* begin heap readjust */
	j = 2;
	while (j <= i) {
	    if (j < i
	     && s_compare(s_ent_sort[SOFF(j)],s_ent_sort[SOFF(j+1)]) < 0)
		j++;
	    if (s_compare(t1,s_ent_sort[SOFF(j)]) > 0)
		break;	/* out of while */
	    else {
		s_ent_sort[SOFF(j/2)] = s_ent_sort[SOFF(j)];
		j = j*2;
	    }
	} /* while */
	s_ent_sort[SOFF(j/2)] = t1;
	/* end heap readjust */
    } /* for */
    /* end of heapsort */
}

void
s_sort()
{
    long i;

#ifdef UNDEF
    qsort((void*)s_ent_sort,(s_ent_sort_max)+1,sizeof(long),s_compare);
#endif
    s_sort_basic();
    s_ent_sorted_max = s_ent_sort_max;  /* whole array is now sorted */
    s_order_changed = FALSE;
    /* rebuild the indexes */
    for (i = 0; i <= s_ent_sort_max; i++)
	s_ent_index[s_ent_sort[i]] = i;
}

void
s_order_clean()
{
    if (s_ent_sort)
	free(s_ent_sort);
    if (s_ent_index)
	free(s_ent_index);

    s_ent_sort = NULL;
    s_contexts[s_cur_context].ent_sort = s_ent_sort;

    s_ent_index = (long*)0;
    s_contexts[s_cur_context].ent_index = s_ent_index;

    s_ent_sort_max = -1;
    s_ent_sorted_max = -1;
    s_ent_index_max = -1;
}

/* adds the entry number to the current context */
void
s_order_add(ent)
long ent;
{
    long size;

    if (ent < s_ent_index_max && s_ent_index[ent] >= 0)
	return;		/* entry is already in the list */

    /* add entry to end of sorted list */
    s_ent_sort_max += 1;
    if (s_ent_sort_max % 100 == 0) {	/* be nice to realloc */
	size = (s_ent_sort_max+100) * sizeof (long);
	s_ent_sort = (long*)saferealloc((char*)s_ent_sort,size);
	/* change the context too */
	s_contexts[s_cur_context].ent_sort = s_ent_sort;
    }
    s_ent_sort[s_ent_sort_max] = ent;

    /* grow index list if needed */
    if (ent > s_ent_index_max) {
	long old,i;
	old = s_ent_index_max;
	if (s_ent_index_max == -1)
	    s_ent_index_max += 1;
	s_ent_index_max = (ent/100+1) * 100;	/* round up */
	size = (s_ent_index_max + 1) * sizeof (long);
	s_ent_index = (long*)saferealloc((char*)s_ent_index,size);
	/* change the context too */
	s_contexts[s_cur_context].ent_index = s_ent_index;
	/* initialize new indexes */
	for (i = old+1; i < s_ent_index_max; i++)
	    s_ent_index[i] = -1;	/* -1 == not a legal entry */
    }
    s_ent_index[ent] = s_ent_sort_max;
    s_order_changed = TRUE;
}

long
s_prev(ent)
long ent;
{
    long tmp;

    if (ent < 0 || ent > s_ent_index_max || s_ent_sorted_max < 0)
	return 0;
    if (s_order_changed)
	s_sort();
    tmp = s_ent_index[ent];
    if (tmp <= 0)
	return 0;
    return s_ent_sort[tmp-1];
}

long
s_next(ent)
long ent;
{
    long tmp;

    if (ent < 0 || ent > s_ent_index_max || s_ent_sorted_max < 0)
	return 0;
    if (s_order_changed)
	s_sort();
    tmp = s_ent_index[ent];
    if (tmp < 0 || tmp == s_ent_sorted_max)
	return 0;
    return s_ent_sort[tmp+1];
}

/* given an entry, returns previous eligible entry */
/* returns 0 if no previous eligible entry */
long
s_prev_elig(a)
long a;
{
    while ((a = s_prev(a)) != 0)
	if (s_eligible(a))
	    return a;
    return 0L;
}

/* given an entry, returns next eligible entry */
/* returns 0 if no next eligible entry */
long
s_next_elig(a)
long a;
{
    while ((a = s_next(a)) != 0)
	if (s_eligible(a))
	    return a;
    return 0L;
}

long
s_first()
{
    if (s_order_changed)
	s_sort();
    if (s_ent_sorted_max < 0)
	return 0;
    return s_ent_sort[0];
}

long
s_last()
{
    if (s_order_changed)
	s_sort();
    if (s_ent_sorted_max < 0)
	return 0;
    return s_ent_sort[s_ent_sorted_max];
}
#endif /* SCAN */
