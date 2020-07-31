/* decode.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "term.h"
#include "hash.h"
#include "cache.h"
#include "head.h"
#include "art.h"
#include "artio.h"
#include "artstate.h"
#include "intrp.h"
#include "final.h"
#include "respond.h"
#include "mime.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "uudecode.h"
#include "INTERN.h"
#include "decode.h"
#include "decode.ih"

void
decode_init()
{
}

char*
decode_fix_fname(s)
char* s;
{
    char* t;
#ifdef MSDOS
    int dotcount = 0;
#endif

    if (!s)
	s = "unknown";

    safefree(decode_filename);
    decode_filename = safemalloc(strlen(s) + 2);

/*$$ we need to eliminate any "../"s from the string */
    while (*s == '/' || *s == '~') s++;
    for (t = decode_filename; *s; s++) {
#ifdef MSDOS
/*$$ we should also handle backslashes here */
	if (*s == '.' && (t == decode_filename || dotcount++))
	    continue;
#endif
	if (isprint(*s)
#ifdef GOODCHARS
	 && index(GOODCHARS, *s)
#else
	 && !index(BADCHARS, *s)
#endif
	)
	    *t++ = *s;
    }
    *t = '\0';
    if (t == decode_filename || bad_filename(decode_filename)) {
	*t++ = 'x';
	*t = '\0';
    }
    return decode_filename;
}

/* Returns nonzero if "filename" is a bad choice */
static bool
bad_filename(filename)
char* filename;
{
    int len = strlen(filename);
#ifdef MSDOS
    if (len == 3) {
	if (strcaseEQ(filename, "aux") || strcaseEQ(filename, "con")
	 || strcaseEQ(filename, "nul") || strcaseEQ(filename, "prn"))
	    return TRUE;
    }
    else if (len == 4) {
	if (strcaseEQ(filename, "com1") || strcaseEQ(filename, "com2")
	 || strcaseEQ(filename, "com3") || strcaseEQ(filename, "com4")
	 || strcaseEQ(filename, "lpt1") || strcaseEQ(filename, "lpt2")
	 || strcaseEQ(filename, "lpt3"))
	    return TRUE;
    }
#else
    if (len <= 2) {
	if (*filename == '.' && (*filename == '\0' || *filename == '.'))
	    return TRUE;
    }
#endif
    return 0;
}

/* Parse the subject looking for filename and part number information. */

char*
decode_subject(artnum, partp, totalp)
ART_NUM artnum;
int* partp;
int* totalp;
{
    static char* subject = NULL;
    char* filename;
    char* s;
    char* t;
    char* end;
    int part = -1, total = 0, hasdot = 0;

    *partp = part;
    *totalp = total;
    safefree(subject);
    subject = fetchsubj(artnum,TRUE);
    if (!*subject)
	return NULL;

    /* Skip leading whitespace and other garbage */
    s = subject;
    while (*s == ' ' || *s == '\t' || *s == '-') s++;
    if (strncaseEQ(s, "repost", 6)) {
	for (s += 6; *s == ' ' || *s == '\t'
	     || *s == ':' || *s == '-'; s++);
    }

    while (strncaseEQ(s, "re:", 3)) {
	s += 3;
	while (isspace(*s)) s++;
    }

    /* Get filename */

    /* Grab the first filename-like string.  Explicitly ignore strings with
     * prefix "v<digit>" ending in ":", since that is a popular volume/issue
     * representation syntax
     */
    end = s + strlen(s);
    do {
	while (*s && !isalnum(*s) && *s != '_') s++;
	filename = t = s;
	while (isalnum(*s) || *s == '-' || *s == '+' || *s == '&'
	      || *s == '_' || *s == '.') {
	    if (*s++ == '.')
		hasdot = 1;
	}
	if (!*s || *s == '\n')
	    return NULL;
    } while (t == s || (t[0] == 'v' && isdigit(t[1]) && *s == ':'));
    *s++ = '\0';
    
    /* Try looking for a filename with a "." in it later in the subject line.
     * Exclude <digit>.<digit>, since that is usually a version number.
     */
    if (!hasdot) {
    	while (*(t = s) != '\0' && *s != '\n') {
	    while (isspace(*t)) t++;
	    for (s = t; isalnum(*s) || *s == '-' || *s == '+'
		 || *s == '&' || *s == '_' || *s == '.'; s++) {
		if (*s == '.' && 
		    (!isdigit(s[-1]) || !isdigit(s[1]))) {
		    hasdot = 1;
		}
	    }
	    if (hasdot && s > t) {
		filename = t;
		*s++ = '\0';
		break;
	    }
	    while (*s && *s != '\n' && !isalnum(*s)) s++;
    	}
    	s = filename + strlen(filename) + 1;
    }

    if (s >= end)
	return NULL;

    /* Get part number */
    while (*s && *s != '\n') {
	/* skip over versioning */
	if (*s == 'v' && isdigit(s[1])) {
	    s++;
	    while (isdigit(*s)) s++;
	}
	/* look for "1/6" or "1 / 6" or "1 of 6" or "1-of-6" or "1o6" */
	if (isdigit(*s)
	 && (s[1] == '/'
	  || (s[1] == ' ' && s[2] == '/')
	  || (s[1] == ' ' && s[2] == 'o' && s[3] == 'f')
	  || (s[1] == '-' && s[2] == 'o' && s[3] == 'f')
	  || (s[1] == 'o' && isdigit(s[2])))) {
	    for (t = s; isdigit(t[-1]); t--) ;
	    part = atoi(t);
	    while (*++s != '\0' && *s != '\n' && !isdigit(*s)) ;
	    total = isdigit(*s)? atoi(s) : 0;
	    while (isdigit(*s)) s++;
	    /* We don't break here because we want the last item on the line */
	}

	/* look for "6 parts" or "part 1" */
	if (strncaseEQ("part", s, 4)) {
	    if (s[4] == 's') {
		for (t = s; t >= subject && !isdigit(*t); t--);
		if (t > subject) {
		    while (t > subject && isdigit(t[-1])) t--;
		    total = atoi(t);
		}
	    }
	    else {
		while (*s && *s != '\n' && !isdigit(*s)) s++;
		if (isdigit(*s))
		    part = atoi(s);
		s--;
	    }
	}
	if (*s)
	    s++;
    }

    if (total == 0 || part == -1 || part > total)
	return NULL;
    *partp = part;
    *totalp = total;
    return filename;
}

/*
 * Handle a piece of a split file.
 */
int
decode_piece(mcp, first_line)
MIMECAP_ENTRY* mcp;
char* first_line;
{
    char* dir;
    char* filename;
    FILE* fp;
    int part, total, state;
    DECODE_FUNC decoder;

    filename = decode_fix_fname(mime_section->filename);
    part = mime_section->part;
    total = mime_section->total;
    *msg = '\0';

    if (!total && is_mime)
	total = part = 1;

    if (mcp || total != 1 || part != 1) {
	/* Create directory to store parts and copy this part there. */
	dir = decode_mkdir(filename);
	if (!dir) {
	    strcpy(msg, "Failed.");
	    return 0;
	}
    }
    else
	dir = NULL;

    if (mcp) {
	if (chdir(dir)) {
	    printf(nocd,dir) FLUSH;
	    sig_catcher(0);
	}
    }

    if (total != 1 || part != 1) {
	sprintf(buf, "Saving part %d ", part);
	if (total)
	    sprintf(buf+strlen(buf), "of %d ", total);
	strcat(buf, filename);
	fputs(buf,stdout);
	if (nowait_fork)
	    fflush(stdout);
	else
	    newline();

	sprintf(buf, "%s%d", dir, part);
	fp = fopen(buf, "w");
	if (!fp) {
	    strcpy(msg,"Failed."); /*$$*/
	    return 0;
	}
	while (readart(art_line,sizeof art_line)) {
	    if (mime_EndOfSection(art_line))
		break;
	    fputs(art_line,fp);
	    if (total == 0 && *art_line == 'e' && art_line[1] == 'n'
	     && art_line[2] == 'd' && isspace(art_line[3])) {
		/* This is the last part. Remember the fact */
		total = part;
		sprintf(buf, "%sCT", dir);
		tmpfp = fopen(buf, "w");
		if (!tmpfp)
		    /*os_perror(buf)*/;
		else {
		    fprintf(tmpfp, "%d\n", total);
		    fclose(tmpfp);
		}
	    }
	}
	fclose(fp);

	/* Retrieve any previously saved number of the last part */
	if (total == 0) {
	    sprintf(buf, "%sCT", dir);
	    if ((fp = fopen(buf, "r")) != NULL) {
		if (fgets(buf, sizeof buf, fp)) {
		    total = atoi(buf);
		    if (total < 0)
			total = 0;
		}
		fclose(fp);
	    }
	    if (total == 0)
		return 1;
	}

	/* Check to see if we have all parts.  Start from the highest numbers
	 * as we are more likely not to have them.
	 */
	for (part = total; part; part--) {
	    sprintf(buf, "%s%d", dir, part);
	    fp = fopen(buf, "r");
	    if (!fp)
		return 1;
	    if (part != 1)
		fclose(fp);
	}
    }
    else {
	fp = NULL;
	total = 1;
    }

    if (mime_section->type == MESSAGE_MIME) {
	mime_PushSection();
	mime_ParseSubheader(fp,first_line);
	first_line = NULL;
    }
    mime_getc_line = first_line;
    decoder = decode_function(mime_section->encoding);
    if (!decoder) {
	strcpy(msg,"Unhandled encoding type -- aborting.");
	if (fp)
	    fclose(fp);
	if (dir)
	    decode_rmdir(dir);
	return 0;
    }

    /* Handle each part in order */
    for (state = DECODE_START, part = 1; part <= total; part++) {
	if (part != 1) {
	    sprintf(buf, "%s%d", dir, part);
	    fp = fopen(buf, "r");
	    if (!fp) {
		/*os_perror(buf);*/
		return 1;
	    }
	}

	state = decoder(fp, state);
	if (fp)
	    fclose(fp);
	if (state == DECODE_ERROR) {
	    strcpy(msg,"Failed."); /*$$*/
	    return 0;
	}
    }

    if (state != DECODE_DONE) {
	(void) decoder((FILE*)NULL, DECODE_DONE);
	if (state != DECODE_MAYBEDONE) {
	    strcpy(msg,"Premature EOF.");
	    return 0;
	}
    }

    if (fp) {
	/* Cleanup all the pieces */
	for (part = 0; part <= total; part++) {
	    sprintf(buf, "%s%d", dir, part);
	    UNLINK(buf);
	}
	sprintf(buf, "%sCT", dir);
	UNLINK(buf);
    }

    if (mcp) {
	mime_Exec(mcp->command);
	UNLINK(decode_filename);
	chdir("..");
    }

    if (dir)
	decode_rmdir(dir);

    return 1;
}

DECODE_FUNC
decode_function(encoding)
int encoding;
{
    switch (encoding) {
      case MENCODE_QPRINT:
	return qp_decode;
      case MENCODE_BASE64:
	return b64_decode;
      case MENCODE_UUE:
	return uudecode;
      case MENCODE_NONE:
	return cat_decode;
      default:
	return NULL;
    }
}

/* return a directory to use for unpacking the pieces of a given filename */
char*
decode_mkdir(filename)
char* filename;
{
    static char dir[LBUFLEN];
    char* s;

#ifdef MSDOS
    interp(dir, sizeof dir, "%Y/parts/");
#else
    interp(dir, sizeof dir, "%Y/m-prts-%L/");
#endif
    strcat(dir, filename);
    s = dir + strlen(dir);
    if (s[-1] == '/')
	return NULL;
    *s++ = '/';
    *s = '\0';
    if (makedir(dir, MD_FILE) != 0)
	return NULL;
    return dir;
}

void
decode_rmdir(dir)
char* dir;
{
    char* s;

    /* Remove trailing slash */
    s = dir + strlen(dir) - 1;
    *s = '\0';

    /*$$ conditionalize this */
    rmdir(dir);
}
