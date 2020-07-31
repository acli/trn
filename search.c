/* search.c
 */

/* string search routines */
 
/*		Copyright (c) 1981,1980 James Gosling		*/
 
/* Modified Aug. 12, 1981 by Tom London to include regular expressions
   as in ed.  RE stuff hacked over by jag to correct a few major problems,
   mainly dealing with searching within the buffer rather than copying
   each line to a separate array.  Newlines can now appear in RE's */

/* Ripped to shreds and glued back together to make a search package,
 * July 6, 1984, by Larry Wall. (If it doesn't work, it's probably my fault.)
 * Changes include:
 *	Buffer, window, and mlisp stuff gone.
 *	Translation tables reduced to 1 table.
 *	Expression buffer is now dynamically allocated.
 *	Character classes now implemented with a bitmap.
 * Modified by David Canzi, Apr 1997:
 *	Check bounds on alternatives array.
 *	Correct spurious matching, eg. /: re: .*\bfoo/ matched ": re: bar".
 */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "util2.h"
#include "INTERN.h"
#include "search.h"

#ifndef BITSPERBYTE
#define BITSPERBYTE 8
#endif

#define BMAPSIZ (127 / BITSPERBYTE + 1)

#define MNULL	64		/* Bit is set in a meta-character defn to
				   indicate that the metacharacter can match
				   a null string.  advance() uses this. */
 
/* meta characters in the "compiled" form of a regular expression */
#define CBRA	(2|MNULL)	/* \( -- begin bracket */
#define	CCHR	4		/* a vanilla character */
#define	CDOT	6		/* . -- match anything except a newline */
#define	CCL	8		/* [...] -- character class */
#define	NCCL	10		/* [^...] -- negated character class */
#define CDOL	(12|MNULL)	/* $ -- matches the end of a line */
#define CEND	(14|MNULL)	/* The end of the pattern */
#define CKET	(16|MNULL)	/* \) -- close bracket */
#define CBACK	(18|MNULL)	/* \N -- backreference to the Nth bracketed
				   string */
#define CIRC	(20|MNULL)	/* ^ matches the beginning of a line */

#define WORD	32		/* matches word character \w */
#define NWORD	34		/* matches non-word characer \W */
#define WBOUND	(36|MNULL)	/* matches word boundary \b */
#define NWBOUND	(38|MNULL)	/* matches non-(word boundary) \B */
 
#define	STAR	01		/* * -- Kleene star, repeats the previous
				   REas many times as possible; the value
				   ORs with the other operator types */
 
#define ASCSIZ 256
typedef Uchar	TRANSTABLE[ASCSIZ];

static TRANSTABLE trans;
static bool folding = FALSE;

static int err;
static char* FirstCharacter;

void
search_init()
{
    register int    i;
    
    for (i = 0; i < ASCSIZ; i++)
	trans[i] = i;
}

void
init_compex(compex)
register COMPEX* compex;
{
    /* the following must start off zeroed */

    compex->eblen = 0;
    compex->brastr = NULL;
}

void
free_compex(compex)
register COMPEX* compex;
{
    if (compex->eblen) {
	free(compex->expbuf);
	compex->eblen = 0;
    }
    if (compex->brastr) {
	free(compex->brastr);
	compex->brastr = NULL;
    }
}

static char* gbr_str = NULL;
static int gbr_siz = 0;

char*
getbracket(compex,n)
register COMPEX* compex;
int n;
{
    int length = compex->braelist[n] - compex->braslist[n];

    if (!compex->nbra)
	return NULL;
    if (n > compex->nbra || !compex->braelist[n] || length < 0)
	return nullstr;
    growstr(&gbr_str, &gbr_siz, length+1);
    safecpy(gbr_str, compex->braslist[n], length+1);
    return gbr_str;
}

void
case_fold(which)
int which;
{
    register int i;

    if (which != folding) {
	if (which) {
	    for (i = 'A'; i <= 'Z'; i++)
		trans[i] = tolower(i);
	}
	else {
	    for (i = 'A'; i <= 'Z'; i++)
		trans[i] = i;
	}
	folding = which;
    }
}

/* Compile the given regular expression into a [secret] internal format */

char*
compile(compex, strp, RE, fold)
register COMPEX* compex;
register char* strp;
int RE;
int fold;
{
    register int c;
    register char* ep;
    char* lastep;
    char  bracket[NBRA];
    char* bracketp;
    char** alt = compex->alternatives;
    char* retmes = "Badly formed search string";
 
    case_fold(compex->do_folding = fold);
    if (!compex->eblen) {
	compex->expbuf = safemalloc(84);
	compex->eblen = 80;
    }
    ep = compex->expbuf;		/* point at expression buffer */
    *alt++ = ep;			/* first alternative starts here */
    bracketp = bracket;			/* first bracket goes here */
    if (*strp == 0) {			/* nothing to compile? */
	if (*ep == 0)			/* nothing there yet? */
	    return "Null search string";
	return NULL;			/* just keep old expression */
    }
    compex->nbra = 0;			/* no brackets yet */
    lastep = 0;
    for (;;) {
	if (ep + 4 - compex->expbuf >= compex->eblen)
	    ep = grow_eb(compex, ep, alt);
	c = *strp++;			/* fetch next char of pattern */
	if (c == 0) {			/* end of pattern? */
	    if (bracketp != bracket) {	/* balanced brackets? */
#ifdef VERBOSE
		retmes = "Unbalanced parens";
#endif
		goto cerror;
	    }
	    *ep++ = CEND;		/* terminate expression */
	    *alt++ = 0;			/* terminal alternative list */
	    return NULL;		/* return success */
	}
	if (c != '*')
	    lastep = ep;
	if (!RE) {			/* just a normal search string? */
	    *ep++ = CCHR;		/* everything is a normal char */
	    *ep++ = c;
	}
	else				/* it is a regular expression */
	    switch (c) {
 
		case '\\':		/* meta something */
		    switch (c = *strp++) {
		    case '(':
			if (compex->nbra >= NBRA) {
#ifdef VERBOSE
			    retmes = "Too many parens";
#endif
			    goto cerror;
			}
			*bracketp++ = ++compex->nbra;
			*ep++ = CBRA;
			*ep++ = compex->nbra;
			break;
		    case '|':
			if (bracketp>bracket) {
#ifdef VERBOSE
			    retmes = "No \\| in parens";	/* Alas! */
#endif
			    goto cerror;
			}
			*ep++ = CEND;
			*alt++ = ep;
			if (alt > compex->alternatives + NALTS) {
#ifdef VERBOSE
				retmes = "Too many alternatives in reg ex";
#endif
				goto cerror;
			}
			break;
		    case ')':
			if (bracketp <= bracket) {
#ifdef VERBOSE
			    retmes = "Unmatched right paren";
#endif
			    goto cerror;
			}
			*ep++ = CKET;
			*ep++ = *--bracketp;
			break;
		    case 'w':
			*ep++ = WORD;
			break;
		    case 'W':
			*ep++ = NWORD;
			break;
		    case 'b':
			*ep++ = WBOUND;
			break;
		    case 'B':
			*ep++ = NWBOUND;
			break;
		    case '0': case '1': case '2': case '3': case '4':
		    case '5': case '6': case '7': case '8': case '9':
			*ep++ = CBACK;
			*ep++ = c - '0';
			break;
		    default:
			*ep++ = CCHR;
			if (c == '\0')
			    goto cerror;
			*ep++ = c;
			break;
		    }
		    break;
		case '.':
		    *ep++ = CDOT;
		    continue;
 
		case '*':
		    if (lastep == 0 || *lastep == CBRA || *lastep == CKET
			|| *lastep == CIRC
			|| (*lastep&STAR)|| *lastep>NWORD)
			goto defchar;
		    *lastep |= STAR;
		    continue;
 
		case '^':
		    if (ep != compex->expbuf && ep[-1] != CEND)
			goto defchar;
		    *ep++ = CIRC;
		    continue;
 
		case '$':
		    if (*strp != 0 && (*strp != '\\' || strp[1] != '|'))
			goto defchar;
		    *ep++ = CDOL;
		    continue;
 
		case '[': {		/* character class */
		    register int i;
		    
		    if (ep - compex->expbuf >= compex->eblen - BMAPSIZ)
			ep = grow_eb(compex, ep, alt); /* reserve bitmap */

		    for (i = BMAPSIZ; i; --i)
			ep[i] = 0;
		    
		    if ((c = *strp++) == '^') {
			c = *strp++;
			*ep++ = NCCL;	/* negated */
		    }
		    else
			*ep++ = CCL;	/* normal */
		    
		    i = 0;		/* remember oldchar */
		    do {
			if (c == '\0') {
#ifdef VERBOSE
			    retmes = "Missing ]";
#endif
			    goto cerror;
			}
			if (*strp == '-' && *(++strp) != ']' && *strp)
			    i = *strp++;
			else
			    i = c;
			while (c <= i) {
			    ep[c / BITSPERBYTE] |= 1 << (c % BITSPERBYTE);
			    if (fold && isalpha(c))
				ep[(c ^ 32) / BITSPERBYTE] |=
				    1 << ((c ^ 32) % BITSPERBYTE);
					/* set the other bit too */
			    c++;
			}
		    } while ((c = *strp++) != ']');
		    ep += BMAPSIZ;
		    continue;
		}
 
	    defchar:
		default:
		    *ep++ = CCHR;
		    *ep++ = c;
	    }
    }
cerror:
    compex->expbuf[0] = 0;
    compex->nbra = 0;
    return retmes;
}

char*
grow_eb(compex, epp, alt)
register COMPEX* compex;
char* epp;
char** alt;
{
    register char* oldbuf = compex->expbuf;
    register char** altlist = compex->alternatives;

    compex->eblen += 80;
    compex->expbuf = saferealloc(compex->expbuf, (MEM_SIZE)compex->eblen + 4);
    if (compex->expbuf != oldbuf) {	/* realloc can change expbuf! */
	epp += compex->expbuf - oldbuf;
	while (altlist != alt)
	    *altlist++ += compex->expbuf - oldbuf; 
    }
    return epp;
}

char*
execute(compex, addr)
register COMPEX* compex;
char* addr;
{
    register char* p1 = addr;
    register Uchar* trt = trans;
    register int c;
 
    if (addr == NULL || compex->expbuf == NULL)
	return NULL;
    if (compex->nbra) {			/* any brackets? */
	for (c = 0; c <= compex->nbra; c++)
	    compex->braslist[c] = compex->braelist[c] = NULL;
	if (compex->brastr)
	    free(compex->brastr);
	compex->brastr = savestr(p1);	/* in case p1 is not static */
	p1 = compex->brastr;		/* ! */
    }
    case_fold(compex->do_folding);	/* make sure table is correct */
    FirstCharacter = p1;		/* for ^ tests */
    if (compex->expbuf[0] == CCHR && !compex->alternatives[1]) {
	c = trt[*(Uchar*)(compex->expbuf+1)]; /* fast check for first char */
	do {
	    if (trt[*(Uchar*)p1] == c && advance(compex, p1, compex->expbuf))
		return p1;
	    p1++;
	} while (*p1 && !err);
	if (err) err = 0;
	return NULL;
    }
    else {			/* regular algorithm */
	do {
	    register char** alt = compex->alternatives;
	    while (*alt) {
		if (advance(compex, p1, *alt++))
		    return p1;
	    }
	    p1++;
	} while (*p1 && !err);
	if (err) err = 0;
	return NULL;
    }
   /*NOTREACHED*/
}
 
/* advance the match of the regular expression starting at ep along the
   string lp, simulates an NDFSA */
bool
advance(compex, lp, ep)
register COMPEX* compex;
register char* ep;
register char* lp;
{
    register char* curlp;
    register Uchar* trt = trans;
    register int i;
 
    while (*lp || (*ep & (STAR|MNULL))) {
	switch (*ep++) {
 
	    case CCHR:
		if (trt[*(Uchar*)ep++] != trt[*(Uchar*)lp])
		    return FALSE;
		lp++;
		continue;
 
	    case CDOT:
		if (*lp == '\n') return FALSE;
		lp++;
		continue;
 
	    case CDOL:
		if (!*lp || *lp == '\n')
		    continue;
		return FALSE;
 
	    case CIRC:
		if (lp == FirstCharacter || lp[-1]=='\n')
		    continue;
		return FALSE;
 
	    case WORD:
		if (isalnum(*lp)) {
		    lp++;
		    continue;
		}
		return FALSE;
 
	    case NWORD:
		if (!isalnum(*lp)) {
		    lp++;
		    continue;
		}
		return FALSE;
 
	    case WBOUND:
		if ((lp == FirstCharacter || !isalnum(lp[-1])) !=
			(!*lp || !isalnum(*lp)) )
		    continue;
		return FALSE;
 
	    case NWBOUND:
		if ((lp == FirstCharacter || !isalnum(lp[-1])) ==
			(!*lp || !isalnum(*lp)))
		    continue;
		return FALSE;
 
	    case CEND:
		return TRUE;
 
	    case CCL:
		if (cclass(ep, *lp, 1)) {
		    ep += BMAPSIZ;
		    lp++;
		    continue;
		}
		return FALSE;
 
	    case NCCL:
		if (cclass(ep, *lp, 0)) {
		    ep += BMAPSIZ;
		    lp++;
		    continue;
		}
		return FALSE;
 
	    case CBRA:
		compex->braslist[(unsigned char)*ep++] = lp;
		continue;
 
	    case CKET:
		i = *ep++;
		compex->braelist[i] = lp;
		compex->braelist[0] = lp;
		compex->braslist[0] = compex->braslist[i];
		continue;
 
	    case CBACK:
		if (compex->braelist[i = *ep++] == 0) {
		    fputs("bad braces\n",stdout) FLUSH;
		    err = TRUE;
		    return FALSE;
		}
		if (backref(compex, i, lp)) {
		    lp += compex->braelist[i] - compex->braslist[i];
		    continue;
		}
		return FALSE;
 
	    case CBACK | STAR:
		if (compex->braelist[i = *ep++] == 0) {
		    fputs("bad braces\n",stdout) FLUSH;
		    err = TRUE;
		    return FALSE;
		}
		curlp = lp;
		while (backref(compex, i, lp)) {
		    lp += compex->braelist[i] - compex->braslist[i];
		}
		while (lp >= curlp) {
		    if (advance(compex, lp, ep))
			return TRUE;
		    lp -= compex->braelist[i] - compex->braslist[i];
		}
		continue;
 
	    case CDOT | STAR:
		curlp = lp;
		while (*lp++ && lp[-1] != '\n') ;
		goto star;
 
	    case WORD | STAR:
		curlp = lp;
		while (*lp++ && isalnum(lp[-1])) ;
		goto star;
 
	    case NWORD | STAR:
		curlp = lp;
		while (*lp++ && !isalnum(lp[-1])) ;
		goto star;
 
	    case CCHR | STAR:
		curlp = lp;
		while (*lp++ && trt[*(Uchar*)(lp-1)] == trt[*(Uchar*)ep]) ;
		ep++;
		goto star;
 
	    case CCL | STAR:
	    case NCCL | STAR:
		curlp = lp;
		while (*lp++ && cclass(ep, lp[-1], ep[-1] == (CCL | STAR))) ;
		ep += BMAPSIZ;
		goto star;
 
	star:
		do {
		    lp--;
		    if (advance(compex, lp, ep))
			return TRUE;
		} while (lp > curlp);
		return FALSE;
 
	    default:
		fputs("Badly compiled pattern\n",stdout) FLUSH;
		err = TRUE;
		return -1;
	}
    }
    return FALSE;
}
 
bool
backref(compex, i, lp)
register COMPEX* compex;
register int i;
register char* lp;
{
    register char* bp;
 
    bp = compex->braslist[i];
    while (*lp && *bp == *lp) {
	bp++;
	lp++;
	if (bp >= compex->braelist[i])
	    return TRUE;
    }
    return FALSE;
}

bool
cclass(set, c, af)
register char* set;
register int c;
int af;
{
    c &= 0177;
#if BITSPERBYTE == 8
    if (set[c >> 3] & 1 << (c & 7))
#else
    if (set[c / BITSPERBYTE] & 1 << (c % BITSPERBYTE))
#endif
	return af;
    return !af;
}
