/* uudecode.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */

#include "EXTERN.h"
#include "common.h"
#include "respond.h"
#include "artio.h"
#include "mime.h"
#include "term.h"
#include "util2.h"
#include "decode.h"
#include "INTERN.h"
#include "uudecode.h"
#include "uudecode.ih"

int
uue_prescan(bp, filenamep, partp, totalp)
char* bp;
char** filenamep;
int* partp;
int* totalp;
{
    char* s;
    char* tmpfilename;
    int tmppart, tmptotal;

    if (strnEQ(bp, "begin ", 6)
     && isdigit(bp[6]) && isdigit(bp[7])
     && isdigit(bp[8]) && (bp[9] == ' ' ||
	(bp[6] == '0' && isdigit(bp[9]) && bp[10] == ' '))) {
	if (*partp == -1) {
	    *filenamep = NULL;
	    *partp = 1;
	    *totalp = 0;
	}
	return 1;
    }
    if (strnEQ(bp,"section ",8) && isdigit(bp[8])) {
	s = bp + 8;
	tmppart = atoi(s);
	if (tmppart == 0)
	    return 0;
	while (isdigit(*s)) s++;
	if (strnEQ(s, " of ", 4)) {
	    /* "section N of ... of file F ..." */
	    for (s += 4; *s && strnNE(s," of file ",9); s++) ;
	    if (!*s)
		return 0;
	    s += 9;
	    tmpfilename = s;
	    s = index(s, ' ');
	    if (!s)
		return 0;
	    *s = '\0';
	    *filenamep = tmpfilename;
	    *partp = tmppart;
	    *totalp = 0;
	    return 1;
	}
	if (*s == '/' && isdigit(s[1])) {
	    /* "section N/M   file F ..." */
	    tmptotal = atoi(s);
	    while (isdigit(*s)) s++;
	    while (*s && isspace(*s)) s++;
	    if (tmppart > tmptotal || strnNE(s,"file ",5))
		return 0;
	    tmpfilename = s+5;
	    s = index(tmpfilename, ' ');
	    if (!s)
		return 0;
	    *s = '\0';
	    *filenamep = tmpfilename;
	    *partp = tmppart;
	    *totalp = tmptotal;
	    return 1;
	}
    }
    if (strnEQ(bp, "POST V", 6)) {
	/* "POST Vd.d.d F (Part N/M)" */
	s = index(bp+6, ' ');
	if (!s)
	    return 0;
	tmpfilename = s+1;
	s = index(tmpfilename, ' ');
	if (!s || strnNE(s, " (Part ", 7))
	    return 0;
	*s = '\0';
	s += 7;
	tmppart = atoi(s);
	while (isdigit(*s)) s++;
	if (tmppart == 0 || *s++ != '/')
	    return 0;
	tmptotal = atoi(s);
	while (isdigit(*s)) s++;
	if (tmppart > tmptotal || *s != ')')
	    return 0;
	*filenamep = tmpfilename;
	*partp = tmppart;
	*totalp = tmptotal;
	return 1;
    }
    if (strnEQ(bp, "File: ", 6)) {
	/* "File: F -- part N of M -- ... */
	tmpfilename = bp+6;
	s = index(tmpfilename, ' ');
	if (!s || strnNE(s, " -- part ", 9))
	    return 0;
	*s = '\0';
	s += 9;
	tmppart = atoi(s);
	while (isdigit(*s)) s++;
	if (tmppart == 0 || strnNE(s, " of ", 4))
	    return 0;
	s += 4;
	tmptotal = atoi(s);
	while (isdigit(*s)) s++;
	if (tmppart > tmptotal || strnNE(s, " -- ", 4))
	    return 0;
	*filenamep = tmpfilename;
	*partp = tmppart;
	*totalp = tmptotal;
	return 1;
    }
    if (strnEQ(bp, "[Section: ", 10)) {
	/* "[Section: N/M  File: F ..." */
	s = bp + 10;
	tmppart = atoi(s);
	if (tmppart == 0)
	    return 0;
	while (isdigit(*s)) s++;
	tmptotal = atoi(++s);
	while (isdigit(*s)) s++;
	while (*s && isspace(*s)) s++;
	if (tmppart > tmptotal || strnNE(s, "File: ", 6))
	    return 0;
	tmpfilename = s+6;
	s = index(tmpfilename, ' ');
	if (!s)
	    return 0;
	*s = '\0';
	*filenamep = tmpfilename;
	*partp = tmppart;
	*totalp = tmptotal;
	return 1;
    }
    if (*filenamep && *partp > 0 && *totalp > 0 && *partp <= *totalp
     && (strnEQ(bp,"BEGIN",5) || strnEQ(bp,"--- BEGIN ---",12)
      || (bp[0] == 'M' && strlen(bp) == UULENGTH))) {
	/* Found the start of a section of uuencoded data
	 * and have the part N of M information.
	 */
	return 1;
    }
    if (strncaseEQ(bp, "x-file-name: ", 13)) {
	for (s = bp + 13; *s && !isspace(*s); s++) ;
	*s = '\0';
	safecpy(msg, bp+13, sizeof msg);
	*filenamep = msg;
	return 0;
    }
    if (strncaseEQ(bp, "x-part: ", 8)) {
	tmppart = atoi(bp+8);
	if (tmppart > 0)
	    *partp = tmppart;
	return 0;
    }
    if (strncaseEQ(bp, "x-part-total: ", 14)) {
	tmptotal = atoi(bp+14);
	if (tmptotal > 0)
	    *totalp = tmptotal;
	return 0;
    }
    return 0;
}

int
uudecode(ifp, state)
FILE* ifp;
int state;
{
    static FILE* ofp = NULL;
    static int line_length;
    char lastline[UULENGTH+1];
    char* filename;
    char* p;

    if (state == DECODE_DONE) {
      all_done:
	if (ofp) {
	    fclose(ofp);
	    ofp = NULL;
	}
	return state;
    }

    while (ifp? fgets(buf, sizeof buf, ifp) : readart(buf, sizeof buf)) {
	if (!ifp && mime_EndOfSection(buf))
	    break;
	p = strchr(buf, '\r');
	if (p) {
	    p[0] = '\n';
	    p[1] = '\0';
	}
	switch (state) {
	  case DECODE_START:	/* Looking for start of uuencoded file */
	  case DECODE_MAYBEDONE:
	    if (strnNE(buf, "begin ", 6))
		break;
	    /* skip mode */
	    p = buf + 6;
	    while (*p && !isspace(*p)) p++;
	    while (*p && isspace(*p)) p++;
	    filename = p;
	    while (*p && (!isspace(*p) || *p == ' ')) p++;
	    *p = '\0';
	    if (!*filename)
		return DECODE_ERROR;
	    filename = decode_fix_fname(filename);

	    /* Create output file and start decoding */
	    ofp = fopen(filename, FOPEN_WB);
	    if (!ofp)
		return DECODE_ERROR;
	    printf("Decoding %s\n", filename);
	    termdown(1);
	    state = DECODE_SETLEN;
	    break;
	  case DECODE_INACTIVE:	/* Looking for uuencoded data to resume */
	    if (*buf != 'M' || strlen(buf) != line_length) {
		if (*buf == 'B' && strnEQ(buf, "BEGIN", 5))
		    state = DECODE_ACTIVE;
		break;
	    }
	    state = DECODE_ACTIVE;
	    /* FALL THROUGH */
	  case DECODE_SETLEN:
	    line_length = strlen(buf);
	    state = DECODE_ACTIVE;
	    /* FALL THROUGH */
	  case DECODE_ACTIVE:	/* Decoding data */
	    if (*buf == 'M' && strlen(buf) == line_length) {
		uudecodeline(buf, ofp);
		break;
	    }
	    if ((int)strlen(buf) > line_length) {
		state = DECODE_INACTIVE;
		break;
	    }
	    /* May be nearing end of file, so save this line */
	    strcpy(lastline, buf);
	    /* some encoders put the end line right after the last M line */
	    if (strnEQ(buf, "end", 3))
		goto end;
	    else if (*buf == ' ' || *buf == '`')
		state = DECODE_LAST;
	    else
		state = DECODE_NEXT2LAST;
	    break;
	  case DECODE_NEXT2LAST:/* May be nearing end of file */
	    if (strnEQ(buf, "end", 3))
		goto end;
	    else if (*buf == ' ' || *buf == '`')
		state = DECODE_LAST;
	    else
		state = DECODE_INACTIVE;
	    break;
	  case DECODE_LAST:	/* Should be at end of file */
	    if (strnEQ(buf, "end", 3) && isspace(buf[3])) {
		/* Handle that last line we saved */
		uudecodeline(lastline, ofp);
end:		if (ofp) {
		    fclose(ofp);
		    ofp = NULL;
		}
		state = DECODE_MAYBEDONE;
	    }
	    else
		state = DECODE_INACTIVE;
	    break;
	  case DECODE_DONE:
	    break;
	}
    }

    if (state == DECODE_DONE || state == DECODE_MAYBEDONE)
	goto all_done;

    return DECODE_INACTIVE;
}

#define DEC(c)	(((c) - ' ') & 077)

/* Decode a uuencoded line to 'ofp' */
static void
uudecodeline(line, ofp)
char* line;
FILE* ofp;
{
    int c, len;

    /* Calculate expected length and pad if necessary */
    if ((len = ((DEC(line[0]) + 2) / 3) * 4) > UULENGTH)
	len = UULENGTH;
    for (c = strlen(line) - 1; c <= len; c++)
	line[c] = ' ';

    len = DEC(*line++);
    while (len) {
	c = DEC(*line) << 2 | DEC(line[1]) >> 4;
	putc(c, ofp);
	if (--len) {
	    c = DEC(line[1]) << 4 | DEC(line[2]) >> 2;
	    putc(c, ofp);
	    if (--len) {
		c = DEC(line[2]) << 6 | DEC(line[3]);
		putc(c, ofp);
		len--;
	    }
	}
	line += 4;
    }
}
