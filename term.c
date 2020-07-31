/* term.c
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "final.h"
#include "help.h"
#include "hash.h"
#include "cache.h"
#include "opt.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "intrp.h"
#include "init.h"
#include "art.h"
#include "rt-select.h"
#ifdef SCORE
#include "score.h"		/* for sc_lookahead */
#endif
#ifdef SCAN
#include "scan.h"
#include "sdisp.h"
#endif
#ifdef SCAN_ART
#include "scanart.h"
#endif
#ifdef USE_TK
#include "tkstuff.h"
#endif
#include "univ.h"
#include "color.h"
#include "INTERN.h"
#include "term.h"
#include "term.ih"
#include "utf.h"

#ifdef u3b2
#undef TIOCGWINSZ
#endif

#undef	USETITE		/* use terminal init/exit seqences (not recommended) */
#undef	USEKSKE		/* use keypad start/end sequences */

char tcarea[TCSIZE];	/* area for "compiled" termcap strings */

static KEYMAP*	topmap INIT(NULL);

static char* lines_export = NULL;
static char* cols_export = NULL;

static int leftcost, upcost;
static bool got_a_char = FALSE;	/* TRUE if we got a char since eating */

/* guarantee capability pointer != NULL */
/* (I believe terminfo will ignore the &tmpaddr argument.) */

char* tgetstr();
#define Tgetstr(key) ((tmpstr = tgetstr(key,&tmpaddr)) ? tmpstr : nullstr)

/* terminal initialization */

void
term_init()
{
    savetty();				/* remember current tty state */

#ifdef I_TERMIO
    outspeed = _tty.c_cflag & CBAUD;	/* for tputs() */
    ERASECH = _tty.c_cc[VERASE];	/* for finish_command() */
    KILLCH = _tty.c_cc[VKILL];		/* for finish_command() */
    if (tc_GT = ((_tty.c_oflag & TABDLY) != TAB3))
	/* we have tabs, so that's OK */;
    else
	_tty.c_oflag &= ~TAB3;	/* turn off kernel tabbing -- done in rn */
#else /* !I_TERMIO */
# ifdef I_TERMIOS
    outspeed = cfgetospeed(&_tty);	/* for tputs() (output) */
    ERASECH = _tty.c_cc[VERASE];	/* for finish_command() */
    KILLCH = _tty.c_cc[VKILL];		/* for finish_command() */
#if 0
    _tty.c_oflag &= ~OXTABS;	/* turn off kernel tabbing-done in rn */
#endif
# else /* !I_TERMIOS */
#  ifdef I_SGTTY
    outspeed = _tty.sg_ospeed;		/* for tputs() */
    ERASECH = _tty.sg_erase;		/* for finish_command() */
    KILLCH = _tty.sg_kill;		/* for finish_command() */
    if (tc_GT = ((_tty.sg_flags & XTABS) != XTABS))
	/* we have tabs, so that's OK */;
    else
	_tty.sg_flags &= ~XTABS;
#  else /* !I_SGTTY */
#   ifdef MSDOS
    outspeed = B19200;
    ERASECH = '\b';
    KILLCH = Ctl('u');
    tc_GT = 1;
#   else
    ..."Don't know how to initialize the terminal!"
#   endif /* !MSDOS */
#  endif /* !I_SGTTY */
# endif /* !I_TERMIOS */
#endif /* !I_TERMIO */

    /* The following could be a table but I can't be sure that there isn't */
    /* some degree of sparsity out there in the world. */

    switch (outspeed) {			/* 1 second of padding */
#ifdef BEXTA
        case BEXTA:  just_a_sec = 1920; break;
#else
#ifdef B19200
        case B19200: just_a_sec = 1920; break;
#endif
#endif
        case B9600:  just_a_sec =  960; break;
        case B4800:  just_a_sec =  480; break;
        case B2400:  just_a_sec =  240; break;
        case B1800:  just_a_sec =  180; break;
        case B1200:  just_a_sec =  120; break;
        case B600:   just_a_sec =   60; break;
	case B300:   just_a_sec =   30; break;
	/* do I really have to type the rest of this??? */
        case B200:   just_a_sec =   20; break;
        case B150:   just_a_sec =   15; break;
        case B134:   just_a_sec =   13; break;
        case B110:   just_a_sec =   11; break;
        case B75:    just_a_sec =    8; break;
        case B50:    just_a_sec =    5; break;
        default:     just_a_sec =  960; break;
					/* if we are running detached I */
    }					/*  don't want to know about it! */
}

#ifdef PENDING
# if !defined(FIONREAD) && !defined(HAS_RDCHK) && !defined(MSDOS)
int devtty;
# endif
#endif

/* set terminal characteristics */

void
term_set(tcbuf)
char* tcbuf;		/* temp area for "uncompiled" termcap entry */
{
    char* tmpaddr;			/* must not be register */
    register char* tmpstr;
    char* s;
    int status;
#ifdef TIOCGWINSZ
    struct winsize winsize;
#endif

#ifdef PENDING
#if !defined (FIONREAD) && !defined (HAS_RDCHK) && !defined(MSDOS)
    /* do no delay reads on something that always gets closed on exit */

    devtty = fileno(stdin);
    if (isatty(devtty)) {
	devtty = open("/dev/tty",0);
	if (devtty < 0) {
	    printf(cantopen,"/dev/tty") FLUSH;
	    finalize(1);
	}
	fcntl(devtty,F_SETFL,O_NDELAY);
    }
#endif
#endif
    
    /* get all that good termcap stuff */

#ifdef HAS_TERMLIB
#ifdef MSDOS
    tc_BC = "\b";
    tc_UP = "\033[A";
    tc_CR = "\r";
    tc_VB = nullstr;
    tc_CL = "\033[H\033[2J";
    tc_CE = "\033[K";
    tc_TI = nullstr;
    tc_TE = nullstr;
    tc_KS = nullstr;
    tc_KE = nullstr;
    tc_CM = "\033[%d;%dH";
    tc_HO = "\033[H";
    tc_IL = nullstr; /*$$*/
    tc_CD = nullstr; /*$$*/
    tc_SO = "\033[7m";
    tc_SE = "\033[m";
    tc_US = "\033[7m";
    tc_UE = "\033[m";
    tc_UC = nullstr;
    set_lines_and_cols();
    tc_AM = TRUE;
#else
    s = getenv("TERM");
    status = tgetent(tcbuf,s? s : "dumb");	/* get termcap entry */
    if (status < 1) {
	printf("No termcap %s found.\n", status ? "file" : "entry") FLUSH;
	finalize(1);
    }
    tmpaddr = tcarea;			/* set up strange tgetstr pointer */
    s = Tgetstr("pc");			/* get pad character */
    tc_PC = *s;				/* get it where tputs wants it */
    if (!tgetflag("bs")) {		/* is backspace not used? */
	tc_BC = Tgetstr("bc");		/* find out what is */
	if (tc_BC == nullstr) {		/* terminfo grok's 'bs' but not 'bc' */
	    tc_BC = Tgetstr("le");
	    if (tc_BC == nullstr)
		tc_BC = "\b";		/* better than nothing... */
	}
    } else
	tc_BC = "\b";			/* make a backspace handy */
    tc_UP = Tgetstr("up");		/* move up a line */
    tc_CL = Tgetstr("cl");		/* get clear string */
    tc_CE = Tgetstr("ce");		/* clear to end of line string */
    tc_TI = Tgetstr("ti");		/* initialize display */
    tc_TE = Tgetstr("te");		/* reset display */
    tc_KS = Tgetstr("ks");		/* enter `keypad transmit' mode */
    tc_KE = Tgetstr("ke");		/* exit `keypad transmit' mode */
    tc_HO = Tgetstr("ho");		/* home cursor */
    tc_IL = Tgetstr("al");		/* insert (add) line */
    tc_CM = Tgetstr("cm");		/* cursor motion */
    tc_CD = Tgetstr("cd");		/* clear to end of display */
    if (!*tc_CE)
	tc_CE = tc_CD;
    tc_SO = Tgetstr("so");		/* begin standout */
    tc_SE = Tgetstr("se");		/* end standout */
    if ((tc_SG = tgetnum("sg"))<0)
	tc_SG = 0;			/* blanks left by SG, SE */
    tc_US = Tgetstr("us");		/* start underline */
    tc_UE = Tgetstr("ue");		/* end underline */
    if ((tc_UG = tgetnum("ug"))<0)
	tc_UG = 0;			/* blanks left by US, UE */
    if (*tc_US)
	tc_UC = nullstr;		/* UC must not be NULL */
    else
	tc_UC = Tgetstr("uc");		/* underline a character */
    if (!*tc_US && !*tc_UC) {		/* no underline mode? */
	tc_US = tc_SO;			/* substitute standout mode */
	tc_UE = tc_SE;
	tc_UG = tc_SG;
    }
    tc_LINES = tgetnum("li");		/* lines per page */
    tc_COLS = tgetnum("co");		/* columns on page */

#ifdef TIOCGWINSZ
    { struct winsize ws;
	if (ioctl(0, TIOCGWINSZ, &ws) >= 0 && ws.ws_row > 0 && ws.ws_col > 0) {
	    tc_LINES = ws.ws_row;
	    tc_COLS = ws.ws_col;
	}
    }
#endif
	
    tc_AM = tgetflag("am");		/* terminal wraps automatically? */
    tc_XN = tgetflag("xn");		/* then eats next newline? */
    tc_VB = Tgetstr("vb");
    if (!*tc_VB)
	tc_VB = "\007";
    tc_CR = Tgetstr("cr");
    if (!*tc_CR) {
	if (tgetflag("nc") && *tc_UP) {
	    tc_CR = safemalloc((MEM_SIZE)strlen(tc_UP)+2);
	    sprintf(tc_CR,"%s\r",tc_UP);
	}
	else
	    tc_CR = "\r";
    }
#ifdef TIOCGWINSZ
	if (ioctl(1, TIOCGWINSZ, &winsize) >= 0) {
		if (winsize.ws_row > 0)
		    tc_LINES = winsize.ws_row;
		if (winsize.ws_col > 0)
		    tc_COLS = winsize.ws_col;
	}
# endif
#endif
    if (!*tc_UP)			/* no UP string? */
	marking = 0;			/* disable any marking */
    if (*tc_CM || *tc_HO)
	can_home = TRUE;
    if (!*tc_CD || !can_home)		/* can we CE, CD, and home? */
	erase_each_line = FALSE;	/*  no, so disable use of clear eol */
    if (muck_up_clear)			/* this is for weird HPs */
	tc_CL = NULL;
    leftcost = strlen(tc_BC);
    upcost = strlen(tc_UP);
#else /* !HAS_TERMLIB */
    ..."Don't know how to set the terminal!"
#endif /* !HAS_TERMLIB */
    termlib_init();
    line_col_calcs();
    noecho();				/* turn off echo */
    crmode();				/* enter cbreak mode */
    sprintf(buf, "%d", tc_LINES);
    lines_export = export("LINES",buf);
    sprintf(buf, "%d", tc_COLS);
    cols_export = export("COLUMNS",buf);

    mac_init(tcbuf);
}

#ifdef MSDOS
static void
set_lines_and_cols()
{
    gotoxy(132,1);
    if (wherex() == 132)
	tc_COLS = 132;
    else
	tc_COLS = 80;
    gotoxy(1,50);
    if (wherey() == 50)
	tc_LINES = 50;
    else {
	gotoxy(1,43);
	if (wherey() == 50)
	    tc_LINES = 50;
	else
	    tc_LINES = 25;
    }
}
#endif

void
set_macro(seq,def)
char* seq;	/* input sequence of keys */
char* def;	/* definition */
{
    mac_line(def,seq,0);
    /* check for common (?) brain damage: ku/kd/etc sequence may be the
     * cursor move sequence instead of the input sequence.
     * (This happens on the local xterm definitions.)
     * Try to recognize and adjust for this case.
     */
    if (seq[0] == '\033' && seq[1] == '[' && seq[2]) {
	char lbuf[LBUFLEN];	/* copy of possibly non-writable string */
	strcpy(lbuf,seq);
	lbuf[1] = 'O';
	mac_line(def,lbuf,0);
    }
    if (seq[0] == '\033' && seq[1] == 'O' && seq[2]) {
	char lbuf[LBUFLEN];	/* copy of possibly non-writable string */
	strcpy(lbuf,seq);
	lbuf[1] = '[';
	mac_line(def,lbuf,0);
    }
}

char* up[] = {
    "^@",
    /* '(' at article or pager, '[' in thread sel, 'p' otherwise */
    "%(%m=[ap]?\\(:%(%m=t?[:p))",
    /* '(' at article or pager, '[' in thread sel, 'p' otherwise */
    "%(%m=[ap]?\\(:%(%m=t?[:p))"
};
char* down[] = {
    "^@",
    /* ')' at article or pager, ']' in thread sel, 'n' otherwise */
    "%(%m=[ap]?\\):%(%m=t?]:n))",
    /* ')' at article or pager, ']' in thread sel, 'n' otherwise */
    "%(%m=[ap]?\\):%(%m=t?]:n))"
};
char* left[] = {
    "^@",
    /* '[' at article or pager, 'Q' otherwise */
    "%(%m=[ap]?\\[:Q)",
    /* '[' at article or pager, '<' otherwise */
    "%(%m=[ap]?\\[:<)"
};
char* right[] = {
    "^@",
    /* ']' at article or pager, CR otherwise */
    "%(%m=[ap]?\\]:^j)",
    /* CR at newsgroups, ']' at article or pager, '>' otherwise */
    "%(%m=n?^j:%(%m=[ap]?\\]:>))"
};

/* Turn the arrow keys into macros that do some basic trn functions.
** Code provided by Clifford Adams.
*/
void
arrow_macros(tmpbuf)
char* tmpbuf;
{
#ifdef HAS_TERMLIB
    char lbuf[256];			/* should be long enough */
#ifndef MSDOS
    char* tmpaddr = tmpbuf;
#endif
    register char* tmpstr;

    /* If arrows are defined as single keys, we probably don't
     * want to redefine them.  (The tvi912c defines kl as ^H)
     */
#ifdef MSDOS
    strcpy(lbuf,"\035\110");
#else
    strcpy(lbuf,Tgetstr("ku"));		/* up */
#endif
    if ((int)strlen(lbuf) > 1)
	set_macro(lbuf,up[auto_arrow_macros]);

#ifdef MSDOS
    strcpy(lbuf,"\035\120");
#else
    strcpy(lbuf,Tgetstr("kd"));		/* down */
#endif
    if ((int)strlen(lbuf) > 1)
	set_macro(lbuf,down[auto_arrow_macros]);

#ifdef MSDOS
    strcpy(lbuf,"\035\113");
#else
    strcpy(lbuf,Tgetstr("kl"));		/* left */
#endif
    if ((int)strlen(lbuf) > 1)
	set_macro(lbuf,left[auto_arrow_macros]);

#ifdef MSDOS
    strcpy(lbuf,"\035\115");
#else
    strcpy(lbuf,Tgetstr("kr"));		/* right */
#endif
    if ((int)strlen(lbuf) > 1)
	set_macro(lbuf,right[auto_arrow_macros]);

    if (*lbuf == '\033')
	set_macro("\033\033", "\033");
#endif
}

static void
mac_init(tcbuf)
char* tcbuf;
{
    char tmpbuf[1024];

    if (auto_arrow_macros)
	arrow_macros(tmpbuf);
    if (!use_threads
     || (tmpfp = fopen(filexp(getval("TRNMACRO",TRNMACRO)),"r")) == NULL)
	tmpfp = fopen(filexp(getval("RNMACRO",RNMACRO)),"r");
    if (tmpfp) {
	while (fgets(tcbuf,TCBUF_SIZE,tmpfp) != NULL)
	    mac_line(tcbuf,tmpbuf,sizeof tmpbuf);
	fclose(tmpfp);
    }
}

void
mac_line(line,tmpbuf,tbsize)
char* line;
char* tmpbuf;
int tbsize;
{
    register char* s;
    register char* m;
    register KEYMAP* curmap;
    register int ch;
    register int garbage = 0;
    static char override[] = "\nkeymap overrides string\n";

    if (topmap == NULL)
	topmap = newkeymap();
    if (*line == '#' || *line == '\n')
	return;
    if (line[ch = strlen(line)-1] == '\n')
	line[ch] = '\0';
    /* A 0 length signifies we already parsed the macro into tmpbuf,
    ** so line is just the definition. */
    if (tbsize)
	m = dointerp(tmpbuf,tbsize,line," \t",(char*)NULL);
    else
	m = line;
    if (!*m)
	return;
    while (*m == ' ' || *m == '\t') m++;
    for (s=tmpbuf,curmap=topmap; *s; s++) {
	ch = *s & 0177;
	if (s[1] == '+' && isdigit(s[2])) {
	    s += 2;
	    garbage = (*s & KM_GMASK) << KM_GSHIFT;
	}
	else
	    garbage = 0;
	if (s[1]) {
	    if ((curmap->km_type[ch] & KM_TMASK) == KM_STRING) {
		if (tbsize) {
		    fputs(override,stdout) FLUSH;
		    termdown(2);
		}
		free(curmap->km_ptr[ch].km_str);
		curmap->km_ptr[ch].km_str = NULL;
	    }
	    curmap->km_type[ch] = KM_KEYMAP + garbage;
	    if (curmap->km_ptr[ch].km_km == NULL)
		curmap->km_ptr[ch].km_km = newkeymap();
	    curmap = curmap->km_ptr[ch].km_km;
	}
	else {
	    if (tbsize && (curmap->km_type[ch] & KM_TMASK) == KM_KEYMAP) {
		fputs(override,stdout) FLUSH;
		termdown(2);
	    }
	    else {
		curmap->km_type[ch] = KM_STRING + garbage;
		curmap->km_ptr[ch].km_str = savestr(m);
	    }
	}
    }
}

static KEYMAP*
newkeymap()
{
    register int i;
    register KEYMAP* map;

#ifndef lint
    map = (KEYMAP*)safemalloc(sizeof(KEYMAP));
#else
    map = NULL;
#endif /* lint */
    for (i = 127; i >= 0; i--) {
	map->km_ptr[i].km_km = NULL;
	map->km_type[i] = KM_NOTHIN;
    }
    return map;
}

void
show_macros()
{
    char prebuf[64];

    if (topmap != NULL) {
	print_lines("Macros:\n",STANDOUT);
	*prebuf = '\0';
	show_keymap(topmap,prebuf);
    }
    else {
	print_lines("No macros defined.\n", NOMARKING);
    }
}

static void
show_keymap(curmap,prefix)
register KEYMAP* curmap;
char* prefix;
{
    register int i;
    register char* next = prefix + strlen(prefix);
    register int kt;

    for (i = 0; i < 128; i++) {
	if ((kt = curmap->km_type[i]) != 0) {
	    if (i < ' ')
		sprintf(next,"^%c",i+64);
	    else if (i == ' ')
		strcpy(next,"\\040");
	    else if (i == 127)
		strcpy(next,"^?");
	    else
		sprintf(next,"%c",i);
	    if ((kt >> KM_GSHIFT) & KM_GMASK) {
		sprintf(cmd_buf,"+%d", (kt >> KM_GSHIFT) & KM_GMASK);
		strcat(next,cmd_buf);
	    }
	    switch (kt & KM_TMASK) {
	      case KM_NOTHIN:
		sprintf(cmd_buf,"%s	%c\n",prefix,i);
		print_lines(cmd_buf,NOMARKING);
		break;
	      case KM_KEYMAP:
		show_keymap(curmap->km_ptr[i].km_km, prefix);
		break;
	      case KM_STRING:
		sprintf(cmd_buf,"%s	%s\n",prefix,curmap->km_ptr[i].km_str);
		print_lines(cmd_buf,NOMARKING);
		break;
	      case KM_BOGUS:
		sprintf(cmd_buf,"%s	BOGUS\n",prefix);
		print_lines(cmd_buf,STANDOUT);
		break;
	    }
	}
    }
}

void
set_mode(new_gmode, new_mode)
char_int new_gmode;
char_int new_mode;
{
    if (gmode != new_gmode || mode != new_mode) {
	gmode = new_gmode;
	mode = new_mode;
	xmouse_check();
    }
}

/* routine to pass to tputs */

int
putchr(ch)
register char_int ch;
{
    putchar(ch);
#ifdef lint
    ch = '\0';
    ch = ch;
#endif
    return 0;
}

int not_echoing = 0;

void
hide_pending()
{
    not_echoing = 1;
    pushchar(0200);
}

bool
finput_pending(check_term)
bool_int check_term;
{
    while (nextout != nextin) {
	if (circlebuf[nextout] != '\200')
	    return 1;
	switch (not_echoing) {
	  case 0:
	    return 1;
	  case 1:
	    nextout++;
	    nextout %= PUSHSIZE;
	    not_echoing = 0;
	    break;
	  default:
	    circlebuf[nextout] = '\n';
	    not_echoing = 0;
	    return 1;
	}
    }
#ifdef PENDING
#ifdef USE_TK
    if (check_term && ttk_running) {
	ttk_do_waiting_events();	/* Update screen, process events. */
	if (ttk_keys && *ttk_keys)
	    return 1;
    }
#endif
    if (check_term) {
# ifdef FIONREAD
	long iocount;
	ioctl(0, FIONREAD, &iocount);
	return (int)iocount;
# else /* !FIONREAD */
#  ifdef HAS_RDCHK
	return rdchk(0);
#  else /* !HAS_RDCHK */
#   ifdef MSDOS
	return kbhit();
#   else /* !MSDOS */
	return circfill();
#   endif /* !MSDOS */
#  endif /* !HAS_RDCHK */
#  endif /* !FIONREAD */
    }
# endif /* !PENDING */
    return 0;
}

/* input the 2nd and succeeding characters of a multi-character command */
/* returns TRUE if command finished, FALSE if they rubbed out first character */

int buflimit = LBUFLEN;

bool
finish_command(donewline)
int donewline;
{
    register char* s;
    char gmode_save = gmode;

    s = buf;
    if (s[1] != FINISHCMD)		/* someone faking up a command? */
	return TRUE;
    set_mode('i',mode);
    if (not_echoing)
	not_echoing = 2;
    do {
	s = edit_buf(s, buf);
	if (s == buf) {			/* entire string gone? */
	    fflush(stdout);		/* return to single char command mode */
	    set_mode(gmode_save,mode);
	    return FALSE;
	}
	if (s - buf == buflimit)
	    break;
	fflush(stdout);
	getcmd(s);
	if (errno || *s == '\f') {
	    *s = Ctl('r');		/* force rewrite on CONT */
	}
    } while (*s != '\r' && *s != '\n'); /* until CR or NL (not echoed) */
    mouse_is_down = FALSE;

    while (s[-1] == ' ') s--;
    *s = '\0';				/* terminate the string nicely */

    if (donewline)
	newline();

    set_mode(gmode_save,mode);
    return TRUE;			/* retrn success */
}

static int
echo_char(ch)
char_int ch;
{
    if (((Uchar)ch & 0x7F) < ' ') {
	putchar('^');
	putchar((ch & 0x7F) | 64);
	return 2;
    }
    if (ch == '\177') {
	putchar('^');
	putchar('?');
	return 2;
    }
    putchar(ch);
    return 1;
}

static bool screen_is_dirty; /*$$ remove this? */

/* Process the character *s in the buffer buf returning the new 's' */

char*
edit_buf(s, cmd)
register char* s;
char* cmd;
{
    static bool quoteone = FALSE;
    if (quoteone) {
	quoteone = FALSE;
	if (s != buf)
	    goto echo_it;
    }
    if (*s == '\033') {		/* substitution desired? */
#ifdef ESCSUBS
	char tmpbuf[4], *cpybuf;

	tmpbuf[0] = '%';
	read_tty(&tmpbuf[1],1);
#ifdef RAWONLY
	tmpbuf[1] &= 0177;
#endif
	tmpbuf[2] = '\0';
	if (tmpbuf[1] == 'h') {
	    (void) help_subs();
	    *s = '\0';
	    reprint();
	}
	else if (tmpbuf[1] == '\033') {
	    *s = '\0';
	    cpybuf = savestr(buf);
	    interpsearch(buf, sizeof buf, cpybuf, cmd);
	    free(cpybuf);
	    s = buf + strlen(buf);
	    reprint();
	}
	else {
	    interpsearch(s, sizeof buf - (s-buf), tmpbuf, cmd);
	    fputs(s,stdout);
	    s += strlen(s);
	}
#else
	notincl("^[");
	*s = '\0';
	reprint();
#endif
	return s;
    }
    else if (*s == ERASECH) {		/* they want to rubout a char? */
	if (s != buf) {
	    rubout();
	    s--;			/* discount the char rubbed out */
	    if (!AT_NORM_CHAR(s))
		rubout();
	}
	return s;
    }
    else if (*s == KILLCH) {		/* wipe out the whole line? */
	while (s != buf) {		/* emulate that many ERASEs */
	    rubout();
	    s--;
	    if (!AT_NORM_CHAR(s))
		rubout();
	}
	return s;
    }
#ifdef WORDERASE
    else if (*s == Ctl('w')) {		/* wipe out one word? */
	if (s == buf)
	    return s;
	*s-- = ' ';
	while (!isspace(*s) || isspace(s[1])) {
	    rubout();
	    if (!AT_NORM_CHAR(s))
		rubout();
	    if (s == buf)
		return buf;
	    s--;
	}
	return s+1;
    }
#endif
    else if (*s == Ctl('r')) {
	*s = '\0';
	reprint();
	return s;
    }
    else if (*s == Ctl('v')) {
	putchar('^');
	backspace();
	fflush(stdout);
	getcmd(s);
    }
    else if (*s == '\\')
	quoteone = TRUE;

echo_it:
    if (!not_echoing)
	echo_char(*s);
    return s+1;
}

bool
finish_dblchar()
{
    bool ret;
    int buflimit_save = buflimit;
    int not_echoing_save = not_echoing;
    buflimit = 2;
    ret = finish_command(FALSE);
    buflimit = buflimit_save;
    not_echoing = not_echoing_save;
    return ret;
}

/* discard any characters typed ahead */

void
eat_typeahead()
{
    static double last_time = 0.;
    double this_time = current_time();

    /* do not eat typeahead while creating virtual group */
    if (univ_ng_virtflag)
      return;
    /* Don't eat twice before getting a character */
    if (!got_a_char)
	return;
    got_a_char = FALSE;

    /* cancel only keyboard stuff */
    if (!allow_typeahead && !mouse_is_down && !macro_pending()
     && this_time - last_time > 0.3) {
#ifdef PENDING
	register KEYMAP* curmap = topmap;
	Uchar lc;
	int i, j;
	for (j = 0; input_pending(); ) {
	    errno = 0;
	    if (read_tty(&buf[j],1) < 0) {
		if (errno && errno != EINTR) {
		    perror(readerr);
		    sig_catcher(0);
		}
		continue;
	    }
	    lc = *(Uchar*)buf;
	    if ((lc & 0200) || curmap == NULL) {
		curmap = topmap;
		j = 0;
		continue;
	    }
	    j++;
	    for (i = (curmap->km_type[lc] >> KM_GSHIFT) & KM_GMASK; i; i--) {
		if (!input_pending())
		    goto dbl_break;
		read_tty(&buf[j++],1);
	    }

	    switch (curmap->km_type[lc] & KM_TMASK) {
	      case KM_STRING:		/* a string? */
	      case KM_NOTHIN:		/* no entry? */
		curmap = topmap;
		j = 0;
		continue;
	      case KM_KEYMAP:		/* another keymap? */
		curmap = curmap->km_ptr[lc].km_km;
		break;
	    }
	}
      dbl_break:
	if (j) {
	    /* Don't delete a partial macro sequence */
	    buf[j] = '\0';
	    pushstring(buf,0);
	}
#else /* this is probably v7 */
# ifdef I_SGTTY
	ioctl(_tty_ch,TIOCSETP,&_tty);
# else
#  ifdef I_TERMIO
	ioctl(_tty_ch,TCSETAW,&_tty);
#  else
#   ifdef I_TERMIOS
	tcsetattr(_tty_ch,TCSAFLUSH,&_tty);
#   else
	..."Don't know how to eat typeahead!"
#   endif
#  endif
# endif
#endif
    }
    last_time = this_time;
}

void
save_typeahead(buf, len)
char* buf;
int len;
{
    int cnt;

    while (input_pending()) {
	cnt = read_tty(buf, len);
	buf += cnt;
	len -= cnt;
    }
    *buf = '\0';
}

void
settle_down()
{
    dingaling();
    fflush(stdout);
    /*sleep(1);*/
    nextout = nextin;			/* empty circlebuf */
    not_echoing = 0;
    eat_typeahead();
}

#ifdef SUPPORT_NNTP
bool ignore_EINTR = FALSE;

Signal_t
alarm_catcher(signo)
int signo;
{
    /*printf("\n*** In alarm catcher **\n"); $$*/
    ignore_EINTR = TRUE;
    check_datasrcs();
    sigset(SIGALRM,alarm_catcher);
    (void) alarm(DATASRC_ALARM_SECS);
}
#endif

/* read a character from the terminal, with multi-character pushback */

int
read_tty(addr,size)
char* addr;
int size;
{
    if (macro_pending()) {
	*addr = circlebuf[nextout++];
	nextout %= PUSHSIZE;
	return 1;
    }
#ifdef USE_TK
    if (ttk_running) {
	ttk_wait_for_input();    /* handle events until input is available */
	if (ttk_keys && *ttk_keys) {
	    int len = strlen(ttk_keys);
	    if (size > len)
		size = len;
	    strncpy(addr, ttk_keys, size);      /* return the first bit */
	    if (len > size)
		pushstring(ttk_keys+size,0);	/* and push the rest */
	    free(ttk_keys);			/* every byte counts... */
	    /* A plain NULL pointer will not work -- it is "\0" in TCL */
	    ttk_keys = savestr(nullstr);
	    return size;
	}
    }
#endif
#ifdef MSDOS
    *addr = getch();
    if (*addr == '\0')
	*addr = Ctl('\035');
    size = 1;
#else
    size = read(0,addr,size);
#endif
#ifdef RAWONLY
    *addr &= 0177;
#endif
    got_a_char = TRUE;
    return size;
}

#ifdef PENDING
# if !defined(FIONREAD) && !defined(HAS_RDCHK) && !defined(MSDOS)
int
circfill()
{
    register int Howmany;

    errno = 0;
    Howmany = read(devtty,circlebuf+nextin,1);

    if (Howmany < 0 && (errno == EAGAIN || errno == EINTR))
	Howmany = 0;
    if (Howmany) {
	nextin += Howmany;
	nextin %= PUSHSIZE;
    }
    return Howmany;
}
# endif /* FIONREAD */
#endif /* PENDING */

void
pushchar(c)
char_int c;
{
    nextout--;
    if (nextout < 0)
	nextout = PUSHSIZE - 1;
    if (nextout == nextin) {
	fputs("\npushback buffer overflow\n",stdout) FLUSH;
	sig_catcher(0);
    }
    circlebuf[nextout] = c;
}

/* print an underlined string, one way or another */

void
underprint(s)
register char* s;
{
    assert(tc_UC);
    if (*tc_UC) {	/* char by char underline? */
	while (*s) {
	    if (!AT_NORM_CHAR(s)) {
		putchar('^');
		backspace();/* back up over it */
		underchar();/* and do the underline */
		putchar((*s & 0x7F) | 64);
		backspace();/* back up over it */
		underchar();/* and do the underline */
	    }
	    else {
		putchar(*s);
		backspace();/* back up over it */
		underchar();/* and do the underline */
	    }
	    s++;
	}
    }
    else {		/* start and stop underline */
	underline();	/* start underlining */
	while (*s)
	    echo_char(*s++);
	un_underline();	/* stop underlining */
    }
}

/* keep screen from flashing strangely on magic cookie terminals */

#ifdef NOFIREWORKS
void
no_sofire()
{
    /* should we disable fireworks? */
    if (!(fire_is_out & STANDOUT) && (term_line|term_col)==0 && *tc_UP && *tc_SE) {
	newline();
	un_standout();
	up_line();
	carriage_return();
    }
}
#endif

#ifdef NOFIREWORKS
void
no_ulfire()
{
    /* should we disable fireworks? */
    if (!(fire_is_out & UNDERLINE) && (term_line|term_col)==0 && *tc_UP && *tc_US) {
	newline();
	un_underline();
	up_line();
	carriage_return();
    }
}
#endif

/* get a character into a buffer */

void
getcmd(whatbuf)
register char* whatbuf;
{
    register KEYMAP* curmap;
    register int i;
    bool no_macros; 
    int times = 0;			/* loop detector */

#ifdef SUPPORT_NNTP
    if (!input_pending()) {
	sigset(SIGALRM,alarm_catcher);
	(void) alarm(DATASRC_ALARM_SECS);
    }
#endif

tryagain:
    curmap = topmap;
#ifdef OLD_RN_WAY
    no_macros = (whatbuf != buf && !macro_pending()); 
#else
    no_macros = (whatbuf != buf && !xmouse_is_on); 
#endif
    for (;;) {
	int_count = 0;
	errno = 0;
#ifdef SUPPORT_NNTP
	ignore_EINTR = FALSE;
#endif
	if (read_tty(whatbuf,1) < 0) {
	    if (!errno)
	        errno = EINTR;
	    if (errno == EINTR) {
#ifdef SUPPORT_NNTP
		if (ignore_EINTR)
		    continue;
		(void) alarm(0);
#endif
		return;
	    }
	    perror(readerr);
	    sig_catcher(0);
	}
	lastchar = *(Uchar*)whatbuf;
	if (lastchar & 0200 || no_macros) {
	    *whatbuf &= 0177;
	    goto got_canonical;
	}
	if (curmap == NULL)
	    goto got_canonical;
	for (i = (curmap->km_type[lastchar] >> KM_GSHIFT) & KM_GMASK; i; i--)
	    read_tty(&whatbuf[i],1);

	switch (curmap->km_type[lastchar] & KM_TMASK) {
	  case KM_NOTHIN:		/* no entry? */
	    if (curmap == topmap)	/* unmapped canonical */
		goto got_canonical;
	    settle_down();
	    goto tryagain;
	  case KM_KEYMAP:		/* another keymap? */
	    curmap = curmap->km_ptr[lastchar].km_km;
	    assert(curmap != NULL);
	    break;
	  case KM_STRING:		/* a string? */
	    pushstring(curmap->km_ptr[lastchar].km_str,0200);
	    if (++times > 20) {		/* loop? */
		fputs("\nmacro loop?\n",stdout);
		termdown(2);
		settle_down();
	    }
	    no_macros = FALSE;
	    goto tryagain;
	}
    }

got_canonical:
    /* This hack is for mouse support */
    if (xmouse_is_on && *whatbuf == Ctl('c')) {
	mouse_input(whatbuf+1);
	times = 0;
	goto tryagain;
    }
#ifdef I_SGTTY
    if (*whatbuf == '\r')
	*whatbuf = '\n';
#endif
    if (whatbuf == buf)
	whatbuf[1] = FINISHCMD;		/* tell finish_command to work */
#ifdef SUPPORT_NNTP
    (void) alarm(0);
#endif
}

void
pushstring(str,bits)
char* str;
char_int bits;
{
    register int i;
    char tmpbuf[PUSHSIZE];
    register char* s = tmpbuf;

    assert(str != NULL);
    interp(tmpbuf,PUSHSIZE,str);
    for (i = strlen(s)-1; i >= 0; i--)
	pushchar(s[i] ^ bits);
}

int
get_anything()
{
    char tmpbuf[64];
    char mode_save = mode;

reask_anything:
    unflush_output();			/* disable any ^O in effect */
    color_object(COLOR_MORE, 1);
#ifdef VERBOSE
    IF(verbose)
	fputs("[Type space to continue] ",stdout);
    ELSE
#endif
#ifdef TERSE
	fputs("[MORE] ",stdout);
#endif
    color_pop();	/* of COLOR_MORE */
    fflush(stdout);
    eat_typeahead();
    if (int_count)
	return -1;
    cache_until_key();
    set_mode(gmode, 'K');
    getcmd(tmpbuf);
    set_mode(gmode,mode_save);
    if (errno || *tmpbuf == '\f') {
	newline();			/* if return from stop signal */
	goto reask_anything;		/* give them a prompt again */
    }
    if (*tmpbuf == 'h') {
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nType q to quit or space to continue.\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs("\nq to quit, space to continue.\n",stdout) FLUSH;
#endif
	termdown(2);
	goto reask_anything;
    }
    else if (*tmpbuf != ' ' && *tmpbuf != '\n') {
	erase_line(0);	/* erase the prompt */
	return *tmpbuf == 'q' ? -1 : *tmpbuf;
    }
    if (*tmpbuf == '\n') {
	page_line = tc_LINES - 1;
	erase_line(0);
    }
    else {
	page_line = 1;
	if (erase_screen)		/* -e? */
	    clear();			/* clear screen */
	else
	    erase_line(0);		/* erase the prompt */
    }
    return 0;
}

int
pause_getcmd()
{
    char mode_save = mode;

    unflush_output();			/* disable any ^O in effect */
    color_object(COLOR_CMD, 1);
#ifdef VERBOSE
    IF(verbose)
	fputs("[Type space or a command] ",stdout);
    ELSE
#endif
#ifdef TERSE
	fputs("[CMD] ",stdout);
#endif
    color_pop();	/* of COLOR_CMD */
    fflush(stdout);
    eat_typeahead();
    if (int_count)
	return -1;
    cache_until_key();
    set_mode(gmode,'K');
    getcmd(buf);
    set_mode(gmode,mode_save);
    if (errno || *buf == '\f')
	return 0;			/* if return from stop signal */
    if (*buf != ' ') {
	erase_line(0);	/* erase the prompt */
	return *buf;
    }
    return 0;
}

void
in_char(prompt, newmode, dflt)
char* prompt;
char_int newmode;
char* dflt;
{
    char mode_save = mode;
    char gmode_save = gmode;
    char* s;
    int newlines;

    for (newlines = 0, s = prompt; *s; s++) {
	if (*s == '\n')
	    newlines++;
    }

reask_in_char:
    unflush_output();			/* disable any ^O in effect */
    printf("%s [%s] ", prompt, dflt);
    fflush(stdout);
    termdown(newlines);
    eat_typeahead();
    set_mode('p',newmode);
    getcmd(buf);
    if (errno || *buf == '\f') {
	newline();			/* if return from stop signal */
	goto reask_in_char;		/* give them a prompt again */
    }
    setdef(buf,dflt);
    set_mode(gmode_save,mode_save);
}

void
in_answer(prompt, newmode)
char* prompt;
char_int newmode;
{
    char mode_save = mode;
    char gmode_save = gmode;

reask_in_answer:
    unflush_output();			/* disable any ^O in effect */
    fputs(prompt,stdout);
    fflush(stdout);
    eat_typeahead();
    set_mode('i',newmode);
reinp_in_answer:
    getcmd(buf);
    if (errno || *buf == '\f') {
	newline();			/* if return from stop signal */
	goto reask_in_answer;		/* give them a prompt again */
    }
    if (*buf == ERASECH)
	goto reinp_in_answer;
    if (*buf != '\n' && *buf != ' ') {
	if (!finish_command(FALSE))
	    goto reinp_in_answer;
    }
    else
	buf[1] = '\0';
    newline();
    set_mode(gmode_save,mode_save);
}

/* If this takes more than one line, return FALSE */

bool
in_choice(prompt, value, choices, newmode)
char* prompt;
char* value;
char* choices;
char_int newmode;
{
    char mode_save = mode;
    char gmode_save = gmode;
    char* cp;
    char* bp;
    char* s;
    char* prefix = NULL;
    int len, number_was = -1, any_val_OK = 0, value_changed;
    char tmpbuf[80], prefixes[80];

    unflush_output();			/* disable any ^O in effect */
    eat_typeahead();
    set_mode('c',newmode);
    screen_is_dirty = FALSE;

    cp = choices;
    if (*cp == '[') {
	for (s = prefixes, cp++; *cp != ']'; ) {
	    if (*cp == '/') {
		*s++ = '\0';
		cp++;
	    }
	    else
		*s++ = *cp++;
	}
	*s++ = '\0';
	*s = '\0';
	if (*++cp == ' ')
	    cp++;
    }
    else
	*prefixes = '\0';
    for (s = tmpbuf; *cp; ) {
	if (*cp == '/') {
	    *s++ = '\0';
	    cp++;
	}
	else if (*cp == '<') {
	    do {
		*s++ = *cp;
	    } while (*cp++ != '>');
	    any_val_OK = 1;		/* flag that '<' was found */
	}
	else
	    *s++ = *cp++;
    }
    cp = s;
    *s++ = '\0';
    *s = '\0';
    strcpy(buf,value);

reask_in_choice:
    len = strlen(buf);
    bp = buf;
    if (*prefixes != '\0') {
	s = prefix;
	for (prefix = prefixes; *prefix; prefix += strlen(prefix)) {
	    if (*prefix == *buf)
		break;
	}
	if (*prefix) {
	    for (bp = buf; *bp && *bp != ' '; bp++) ;
	    while (*bp == ' ') bp++;
	}
	else
	    prefix = NULL;
	value_changed = prefix != s;
    }
    else {
	prefix = NULL;
	value_changed = 0;
    }
    s = cp;
    for (;;) {
	cp += strlen(cp) + 1;
	if (!*cp)
	    cp = tmpbuf;
	if (*cp == '<'
	 && (*buf == '<' || cp[1] != '#' || isdigit(*buf) || !*s)) {
	    prefix = NULL;
	    break;
	}
	if (s == cp) {
	    if (!value_changed) {
		if (prefix)
		    prefix = NULL;
		else
		    dingaling();
	    }
	    break;
	}
	if (!*bp || strnEQ(cp,bp,any_val_OK? len : 1))
	    break;
    }

    if (*cp == '<') {
	if (*buf == '<' || cp[1] == '#') {
	    if (number_was >= 0)
		sprintf(buf, "%d", number_was);
	    else {
		for (s = buf; isdigit(*s); s++) ;
		*s = '\0';
	    }
	}
    }
    else {
	if (prefix) {
	    sprintf(buf, "%s ", prefix);
	    strcat(buf,cp);
	}
	else
	    strcpy(buf,cp);
    }
    s = buf + strlen(buf);
    carriage_return();
    erase_line(0);
    fputs(prompt,stdout);
    fputs(buf,stdout);
    len = strlen(prompt);
    number_was = -1;

reinp_in_choice:
    if ((s-buf) + len >= tc_COLS)
	screen_is_dirty = TRUE;
    fflush(stdout);
    getcmd(s);
    if (errno || *s == '\f')		/* if return from stop signal */
	*s = '\n';
    if (*s != '\n') {
	char ch = *s;
	if (*cp == '<' && ch != '\t' && (ch != ' ' || buf != s)) {
	    if (cp[1] == '#') {
		s = edit_buf(s, (char*)NULL);
		if (s != buf) {
		    if (isdigit(s[-1]))
			goto reinp_in_choice;
		    else
			number_was = atoi(buf);
		}
	    }
	    else {
		s = edit_buf(s, (char*)NULL);
		goto reinp_in_choice;
	    }
	}
	*s = '\0';
	for (s = buf; *s && *s != ' '; s++) ;
	if (*s == ' ') s++;
	if (ch == ' ' || ch == '\t') {
	    if (prefix)
		*s = '\0';
	    else
		*buf = '\0';
	}
	else {
	    char ch1 = buf[0];
	    if (prefix) {
		if (ch == ch1)
		    ch = *s;
		else {
		    ch1 = ch;
		    ch = buf[0];
		}
	    }
	    sprintf(buf,"%c %c",ch == ERASECH || ch == KILLCH? '<' : ch, ch1);
	}
	goto reask_in_choice;
    }
    *s = '\0';

    set_mode(gmode_save,mode_save);
    return !screen_is_dirty;
}

int
print_lines(what_to_print,hilite)
char* what_to_print;
int hilite;
{
#ifndef USE_UTF_HACK
    register
#endif
    char* s;
    register int i;

    for (s=what_to_print; *s; ) {
	i = check_page_line();
	if (i)
	    return i;
	if (hilite == STANDOUT) {
#ifdef NOFIREWORKS
	    if (erase_screen)
		no_sofire();
#endif
	    standout();
	}
	else if (hilite == UNDERLINE) {
#ifdef NOFIREWORKS
	    if (erase_screen)
		no_ulfire();
#endif
	    underline();
	}
	for (i = 0; i < tc_COLS; i++) {
	    if (!*s)
		break;
	    if (AT_NORM_CHAR(s)) {
		i += put_char_adv(&s) - 1;
		s--;
	    }
	    else if (*s == '\t') {
		putchar(*s);
		i = ((i+8) & ~7) - 1; 
	    }
	    else if (*s == '\n') {
		i = 32000;
	    }
	    else {
		i++;
		putchar('^');
		putchar(*s + 64);
	    }
	    s++;
	}
	if (i) {
	    if (hilite == STANDOUT)
		un_standout();
	    else if (hilite == UNDERLINE)
		un_underline();
	    if (tc_AM && i == tc_COLS)
		fflush(stdout);
	    else
		newline();
	}
    }
    return 0;
}

int
check_page_line()
{
    if (page_line < 0)
	return -1;
    if (page_line >= tc_LINES || int_count) {
	register int cmd = -1;
	if (int_count || (cmd = get_anything())) {
	    page_line = -1;		/* disable further printing */
	    if (cmd > 0)
		pushchar(cmd);
	    return cmd;
	}
    }
    page_line++;
    return 0;
}

void
page_start()
{
    page_line = 1;
    if (erase_screen)
	clear();
    else
	newline();
}

void
errormsg(str)
char* str;
{
    if (gmode == 's') {
	if (str != msg)
	    strcpy(msg,str);
	error_occurred = TRUE;
    }
    else if (*str) {
	printf("\n%s\n", str) FLUSH;
	termdown(2);
    }
}

void
warnmsg(str)
char* str;
{
    if (gmode != 's') {
	printf("\n%s\n", str) FLUSH;
	termdown(2);
	pad(just_a_sec/3);
    }
}

void
pad(num)
int num;
{
    register int i;

    for (i = num; i; i--)
	putchar(tc_PC);
    fflush(stdout);
}

/* echo the command just typed */

#ifdef VERIFY
void
printcmd()
{
    if (verify && buf[1] == FINISHCMD) {
	if (!AT_NORM_CHAR(buf)) {
	    putchar('^');
	    putchar((*buf & 0x7F) | 64);
	    backspace();
	    backspace();
	}
	else {
	    putchar(*buf);
	    backspace();
	}
	fflush(stdout);
    }
}
#endif

void
rubout()
{
    backspace();			/* do the old backspace, */
    putchar(' ');			/*   space, */
    backspace();			/*     backspace trick */
}

void
reprint()
{
    register char* s;

    fputs("^R\n",stdout) FLUSH;
    termdown(1);
    for (s = buf; *s; s++)
	echo_char(*s);
    screen_is_dirty = TRUE;
}

void
erase_line(to_eos)
bool_int to_eos;
{
    carriage_return();
    if (to_eos)
	clear_rest();
    else
	erase_eol();
    carriage_return();		/* Resets kernel's tab column counter to 0 */
    fflush(stdout);
}

void
clear()
{
    term_line = term_col = fire_is_out = 0;
    if (tc_CL)
	tputs(tc_CL,tc_LINES,putchr);
    else if (tc_CD) {
	home_cursor();
	tputs(tc_CD,tc_LINES,putchr);
    }
    else {
	int i;
	for (i = 0; i < tc_LINES; i++)
	    putchr('\n');
	home_cursor();
    }
    tputs(tc_CR,1,putchr) FLUSH;
}

void
home_cursor()
{
    char* tgoto();

    if (!*tc_HO) {		/* no home sequence? */
	if (!*tc_CM) {		/* no cursor motion either? */
	    fputs("\n\n\n", stdout);
	    termdown(3);
	    return;		/* forget it. */
	}
	tputs(tgoto(tc_CM, 0, 0), 1, putchr);	/* go to home via CM */
    }
    else {			/* we have home sequence */
	tputs(tc_HO, 1, putchr);/* home via HO */
    }
    carriage_return();	/* Resets kernel's tab column counter to 0 */
    term_line = term_col = 0;
}

void
goto_xy(to_col,to_line)
int to_col;
int to_line;
{
    char* tgoto();
    char* str;
    int cmcost, xcost, ycost;

    if (term_col == to_col && term_line == to_line)
	return;
    if (*tc_CM && !muck_up_clear) {
	str = tgoto(tc_CM,to_col,to_line);
	cmcost = strlen(str);
    } else {
	str = NULL;
	cmcost = 9999;
    }

    if ((ycost = (to_line - term_line)) < 0)
	ycost = (upcost? -ycost * upcost : 7777);
    else if (ycost > 0)
	term_col = 0;

    if ((xcost = (to_col - term_col)) < 0) {
	if (!to_col && ycost+1 < cmcost) {
	    carriage_return();
	    xcost = 0;
	}
	else
	    xcost = -xcost * leftcost;
    }
    else if (xcost > 0 && cmcost < 9999)
	xcost = 9999;

    if (cmcost <= xcost + ycost) {
	tputs(str,1,putchr);
	term_line = to_line;
	term_col = to_col;
	return;
    }

    if (ycost == 7777)
	home_cursor();

    if (to_line >= term_line)
	while(term_line < to_line) newline();
    else
	while(term_line > to_line) up_line();

    if (to_col >= term_col)
	while(term_col < to_col) term_col++, putchar(' ');
    else
	while(term_col > to_col) term_col--, backspace();
}

static void
line_col_calcs()
{
    if (tc_LINES > 0) {		/* is this a crt? */
	if (!initlines || !option_def_vals[OI_INITIAL_ARTICLE_LINES]) {
	    /* no -i or unreasonable value for initlines */
	    if (outspeed >= B9600) 	/* whole page at >= 9600 baud */
		initlines = tc_LINES;
	    else if (outspeed >= B4800)	/* 16 lines at 4800 */
		initlines = 16;
	    else			/* otherwise just header */
		initlines = 8;
	}
	/* Check for initlines bigger than the screen and fix it! */
	if (initlines > tc_LINES)
	    initlines = tc_LINES;
    }
    else {				/* not a crt */
	tc_LINES = 30000;		/* so don't page */
	tc_CL = "\n\n";			/* put a couple of lines between */
	if (!initlines || !option_def_vals[OI_INITIAL_ARTICLE_LINES])
	    initlines = 8;		/* make initlines reasonable */
    }
    if (tc_COLS <= 0)
	tc_COLS = 80;
#ifdef SCAN
    s_resize_win();	/* let various parts know */
#endif
}

#ifdef SIGWINCH
Signal_t
winch_catcher(dummy)
int dummy;
{
    /* Reset signal in case of System V dain bramage */
    sigset(SIGWINCH, winch_catcher);

    /* Come here if window size change signal received */
#ifdef TIOCGWINSZ
    {	struct winsize ws;
	if (ioctl(0, TIOCGWINSZ, &ws) >= 0 && ws.ws_row > 0 && ws.ws_col > 0) {
	    if (tc_LINES != ws.ws_row || tc_COLS != ws.ws_col) {
		tc_LINES = ws.ws_row;
		tc_COLS = ws.ws_col;
		line_col_calcs();
		sprintf(lines_export, "%d", tc_LINES);
		sprintf(cols_export, "%d", tc_COLS);
		if (gmode == 's' || mode == 'a' || mode == 'p') {
		    forceme("\f");	/* cause a refresh */
					/* (defined only if TIOCSTI defined) */
		}
	    }
	}
    }
#else
    /* Well, if SIGWINCH is defined, but TIOCGWINSZ isn't, there's    */
    /* almost certainly something wrong.  Figure it out for yourself, */
    /* because I don't know how to deal with it :-)                   */
    ERROR!
#endif
}
#endif

void
termlib_init()
{
#ifdef USETITE
    if (tc_TI && *tc_TI) {
	tputs(tc_TI,1,putchr);
	fflush(stdout);
    }
#endif
#ifdef USEKSKE
    if (tc_KS && *tc_KS) {
	tputs(tc_KS,1,putchr);
	fflush(stdout);
    }
#endif
    term_line = tc_LINES-1;
    term_col = 0;
    term_scrolled = tc_LINES;
}

void
termlib_reset()
{
#ifdef USETITE
    if (tc_TE && *tc_TE) {
	tputs(tc_TE,1,putchr);
	fflush(stdout);
    }
#endif
#ifdef USEKSKE
    if (tc_KE && *tc_KE) {
	tputs(tc_KE,1,putchr);
	fflush(stdout);
    }
#endif
}

/* Nice-Background routines used to improve interactive performance with
 * background processing.  When called, these routines will wait for a
 * keystroke or a time limit to expire.  Calling this before a
 * background loop may significantly increase responsiveness when a
 * user types commands quickly.
 */

/* NOTE: If you have problems, uncomment this define for some debugging help.
 * (The time limit will be 5 seconds and lots of info will be printed). */
/* #define DEBUG_NICEBG */

#ifdef PENDING
#ifdef NICEBG

#ifdef NBG_SELECT
#undef NBG_TERMIO
#undef NBG_SIGIO
#endif

#ifdef NBG_TERMIO
#undef NBG_SELECT
#undef NBG_SIGIO
#endif

#ifdef NBG_SIGIO
#undef NBG_TERMIO
#undef NBG_SELECT
#endif

#ifdef NBG_SIGIO /* SIGIO style */
Signal_t
waitkey_sig_handler(dummy)
int dummy;
{
#ifdef DEBUG_NICEBG
    /* CAA: I doubt that printf is safe in a signal handler... */
    printf("wait_key_pause signal caught!\n") FLUSH;
    fflush(stdout);
#endif
}
#endif /* NBG_SIGIO */

/* /dev/tty file descriptor */
/* note: for now we let the system close it on exit... */
static int wait_ttyfd INIT(-1);
static bool wait_initialized INIT(FALSE);
static bool wait_fd_opened INIT(FALSE);

#ifdef NBG_SELECT
static struct timeval wait_time;
static struct fd_set wait_fdset;
static int wait_tbl_size;
#endif

/* returns TRUE if input received */
bool
wait_key_pause(ticks)
int ticks;		/* tenths of seconds to wait */
{
#ifdef DEBUG_NICEBG
    ticks = 50;		/* debugging: wait 5 seconds */
#endif
    if (input_pending())
	return TRUE;
#ifdef DEBUG_NICEBG
    printf("entry: wait_key_pause\n") FLUSH; /* */
#endif
    /* common initialization */
    if (!wait_fd_opened) {
	wait_ttyfd = open("/dev/tty",0); /*$$ possible cron prob */
	/* CAA: the open shouldn't really be a problem now that the
	 * open return value is checked below.  If running from cron,
	 * I hope that the open will simply fail.  If the open succeeds,
	 * however, then there should be no cause for complaint by the
	 * routines below.  (The select method may be safest.)  Still,
	 * it might be nice to have some flag which means "we are running
	 * in the background".
	 */
	wait_fd_opened = TRUE;
    }
    if (wait_ttyfd < 0) {
	/* tty open failed.  Don't wait around */
	return input_pending();
    }
#ifdef NBG_TERMIO
    /* First try a nice standard way */
    {
#ifdef I_TERMIO
	struct termio save_tty, wait_tty;
#else	/* must be TERMIOS then */
	struct termios save_tty, wait_tty;
#endif
	char lbuf[1];		/* for the read command */
	int nrd;		/* number read */

#ifdef DEBUG_NICEBG
	printf("wait_key_pause: using termio(s)\n") FLUSH;
#endif
	if (!wait_initialized)
	    wait_initialized = TRUE;
	if (ioctl(wait_ttyfd,TCGETA,&save_tty) == -1) {
	    printf("wait_key_pause (term.c): ioctl TCGETA error\n") FLUSH;
	    assert(FALSE);
	}
	wait_tty = save_tty;
	wait_tty.c_lflag &= ~ICANON;
	wait_tty.c_lflag &= ~ECHO;
	wait_tty.c_cc[VMIN] = 0;
	wait_tty.c_cc[VTIME] = ticks;
	if (ioctl(wait_ttyfd,TCSETAF,&wait_tty) == -1) {
	    printf("wait_key_pause (term.c): ioctl TCSETAF error\n") FLUSH;
	    assert(FALSE);
	}
	nrd = read(wait_ttyfd,&lbuf,1);
	ioctl(wait_ttyfd,TCSETAF,&save_tty);	/* put back the old info */
	if (nrd == 1) {
	    pushchar(lbuf[0]);	/* put it back */
#ifdef DEBUG_NICEBG
	    printf("early exit: wait_key_pause\n") FLUSH; /* */
#endif
	}
#ifdef DEBUG_NICEBG
	printf("exit: wait_key_pause\n") FLUSH; /* */
#endif
	return input_pending();
    }
#endif /* NBG_TERMIO */
#ifdef NBG_SELECT
#ifdef DEBUG_NICEBG
	printf("wait_key_pause: using select\n") FLUSH;
#endif
	if (!wait_initialized) {
	    wait_tbl_size = wait_ttyfd + 1;
	    wait_initialized = TRUE;
	}
	FD_ZERO(&wait_fdset);
	FD_SET(wait_ttyfd,&wait_fdset);

	wait_time.tv_usec = (ticks%10)*1000000;
	wait_time.tv_sec = ticks/10;

	(void)select(wait_tbl_size,&wait_fdset,NULL,NULL,&wait_time);

#ifdef DEBUG_NICEBG
	printf("exit: wait_key_pause\n") FLUSH; /* */
#endif
	return input_pending();
#endif /* NBG_SELECT */
#ifdef NBG_SIGIO
/* SIGIO signal sent for each keystroke style */
/* Method of last resort.  Surprisingly, this code has been quite
 * portable to several systems.
 */
    {
#ifdef DEBUG_NICEBG
	printf("wait_key_pause: using SIGIO style\n") FLUSH;
#endif
	if (!wait_initialized) {	/* not initialized yet? */
/* Portable? (probably not) */
	    wait_initialized = TRUE;
	    sigset(SIGALRM,waitkey_sig_handler);
	    sigset(SIGIO,waitkey_sig_handler);
	    /* enable SIGIO (some systems need it).  ifdef it? */
	    fcntl(wait_ttyfd,F_SETOWN,(int)getpid());
	    fcntl(wait_ttyfd,F_SETFL,FASYNC);
	}
	(void) alarm((ticks+9)/10);	/* sleep if no key pressed */
/* NOTE: There is a race condition here, but not a very important one IMO.
 *	 If the alarm signal is sent *before* pause() is called, then this
 *	 function will simply wait until a key is pressed.  In this case
 *	 no background processing will occur.
 *	 Noted by CAA, August 1992.
 */
/* note2: The race condition can be minimized by using a variable
 * which will tell if the alarm arrived before the pause (really before
 * the variable was set, which should be near-instant.)  Look into
 * this later.
 */
	pause();	/* wait for either ALRM or SIGIO signal */
	(void) alarm(0);
#ifdef DEBUG_NICEBG
	printf("exit: wait_key_pause\n") FLUSH;
#endif
	return input_pending();
    }
#endif /* NBG_SIGIO */
/* if no nice-background methods are defined, return. */
/* Thanks to Kian-Tat Lim for finding an earlier bug here */
#ifndef NBG_SELECT
#ifndef NBG_TERMIO
#ifndef NBG_SIGIO
    /* really kind of an error, but a reasonable way to handle it */
    /* Act as if no input was present */
#ifdef DEBUG_NICEBG
    printf("wait_key_pause: no handler style, returning FALSE.\n");
#endif
    return FALSE;	/* don't worry if not reached */
#endif
#endif
#endif
}
#endif /* NICEBG */
#endif /* PENDING */

void
xmouse_init(progname)
char* progname;
{
    char* s;
    if (!can_home || !use_threads)
	return;
    s = getenv("XTERMMOUSE");
    if (s && *s) {
	interp(msg, sizeof msg, s);
	set_option(OI_USE_MOUSE, msg);
    }
    else if (progname[strlen(progname)-1] == 'x') {
	/* an 'x' at the end means enable Xterm mouse tracking */
	set_option(OI_USE_MOUSE, "y");
    }
}

void
xmouse_check()
{
    mousebar_cnt = 0;
    if (UseMouse) {
	bool turn_it_on;
	char mmode = mode;
	if (gmode == 'p') {
	    turn_it_on = TRUE;
	    mmode = '\0';
	}
	else if (gmode == 'i' || gmode == 'p'
	      || (muck_up_clear && gmode != 's'))
	    turn_it_on = FALSE;
	else {
	    interp(msg, sizeof msg, MouseModes);
	    turn_it_on = (index(msg, mode) != NULL);
	}
	if (turn_it_on) {
	    char* s;
	    int i, j;
	    switch (mmode) {
	      case 'c':
		mousebar_btns = NewsrcSelBtns;
		mousebar_cnt = NewsrcSelBtnCnt;
		break;
	      case 'j':
		mousebar_btns = AddSelBtns;
		mousebar_cnt = AddSelBtnCnt;
		break;
	      case 'l':
		mousebar_btns = OptionSelBtns;
		mousebar_cnt = OptionSelBtnCnt;
		break;
	      case 't':
		mousebar_btns = NewsSelBtns;
		mousebar_cnt = NewsSelBtnCnt;
		break;
	      case 'w':
		mousebar_btns = NewsgroupSelBtns;
		mousebar_cnt = NewsgroupSelBtnCnt;
		break;
	      case 'a':  case 'p':
		mousebar_btns = ArtPagerBtns;
		mousebar_cnt = ArtPagerBtnCnt;
		break;
	      case 'v':
		mousebar_btns = UnivSelBtns;
		mousebar_cnt = UnivSelBtnCnt;
		break;
	      default:
		mousebar_btns = nullstr;
		/*mousebar_cnt = 0;*/
		break;
	    }
	    s = mousebar_btns;
	    mousebar_width = 0;
	    for (i = 0; i < mousebar_cnt; i++) {
		j = strlen(s);
		if (*s == '[') {
		    mousebar_width += j;
		    s += j + 1;
		    j = strlen(s);
		}
		else
		    mousebar_width += (j < 2? 3+1 : (j == 2? 4+1
						     : (j < 5? j: 5+1)));
		s += j + 1;
	    }
	    xmouse_on();
	}
	else
	    xmouse_off();
	mouse_is_down = FALSE;
    }
}

void
xmouse_on()
{
    if (!xmouse_is_on) {
	/* save old highlight mouse tracking and enable mouse tracking */
	fputs("\033[?1001s\033[?1000h",stdout);
	fflush(stdout);
	xmouse_is_on = TRUE;
    }
}

void
xmouse_off()
{
    if (xmouse_is_on) {
	/* disable mouse tracking and restore old highlight mouse tracking */
	fputs("\033[?1000l\033[?1001r",stdout);
	fflush(stdout);
	xmouse_is_on = FALSE;
    }
}

void
draw_mousebar(limit,restore_cursor)
int limit;
bool_int restore_cursor;
{
    int i;
    char* s;
    char* t;
    int save_col = term_col;
    int save_line = term_line;

    mousebar_width = 0;
    if (mousebar_cnt == 0)
	return;

    s = mousebar_btns;
    t = msg;
    for (i = 0; i < mousebar_cnt; i++) {
	if (*s == '[') {
	    while (*++s) *t++ = *s;
	    s++;
	}
	else {
	    switch (strlen(s)) {
	      case 0:
		*t++ = ' ';
		/* FALL THROUGH */
	      case 1:  case 2:
		*t++ = ' ';
		while (*s) *t++ = *s++;
		*t++ = ' ';
		break;
	      case 3:  case 4:
		while (*s) *t++ = *s++;
		break;
	      default:
		strncpy(t, s, 5);
		t += 5;
		break;
	    }
	}
	s += strlen(s) + 1;
	*t++ = '\0';
    }
    mousebar_width = t - msg;
    mousebar_start = 0;

    s = msg;
    while (mousebar_width > limit) {
	int len = strlen(s) + 1;
	s += len;
	mousebar_width -= len;
	mousebar_start++;
    }

    goto_xy(tc_COLS - mousebar_width - 1, tc_LINES-1);
    for (i = mousebar_start; i < mousebar_cnt; i++) {
	putchar(' ');
	color_string(COLOR_MOUSE,s);
	s += strlen(s) + 1;
    }
    term_col = tc_COLS-1;
    if (restore_cursor)
	goto_xy(save_col, save_line);
}

static void
mouse_input(cp)
char* cp;
{
    static int last_btn;
    static int last_x, last_y;
    int btn;
    int x, y;

    if (cp[2] < ' ' || cp[2] > ' '+3)
	return;
    btn = (cp[2] & 3);
    x = cp[1] - 33;
    y = cp[0] - 33;

    if (btn != 3) {
#if defined(HAS_GETTIMEOFDAY) || defined(HAS_FTIME)
	static double last_time = 0.;
	double this_time = current_time();
	if (last_btn == btn && last_y == y && this_time - last_time <= 0.75
	 && (last_x == x || last_x == x-1 || last_x == x+1))
	    btn |= 4;
	last_time = this_time;
#endif /* HAS_GETTIMEOFDAY || HAS_FTIME */
	last_btn = (btn & 3);
	last_x = x;
	last_y = y;
	mouse_is_down = TRUE;
    }
    else {
	if (!mouse_is_down)
	    return;
	mouse_is_down = FALSE;
    }

    if (gmode == 's')
	selector_mouse(btn, x,y, last_btn, last_x,last_y);
    else if (mode == 'a' || mode == 'p')
	pager_mouse(btn, x,y, last_btn, last_x,last_y);
    else
	pushchar(' ');
}

bool
check_mousebar(btn, x,y, btn_clk, x_clk,y_clk)
int btn;
int x, y;
int btn_clk;
int x_clk, y_clk;
{
    char* s = mousebar_btns;
    char* t;
    int i, j;
    int col = tc_COLS - mousebar_width;

    if (mousebar_width != 0 && btn_clk == 0 && y_clk == tc_LINES-1
     && (x_clk -= col-1) > 0) {
	x -= col-1;
	for (i = 0; i < mousebar_start; i++) {
	    if (*s == '[')
		s += strlen(s) + 1;
	    s += strlen(s) + 1;
	}
	while (1) {
	    i = strlen(s);
	    t = s;
	    if (*s == '[') {
		s += i + 1;
		j = 0;
		while (*++t == ' ') j++;
	    }
	    else if (i <= 2) {
		i = 3 + (i==2) + 1;
		j = 1;
	    }
	    else {
		i = (i < 5? i+1 : 5+1);
		j = 0;
	    }
	    if (x_clk < i) {
		int tcol = term_col;
		int tline = term_line;
		goto_xy(col+j,tc_LINES-1);
		if (btn == 3)
		    color_object(COLOR_MOUSE, 1);
		if (s == t) {
		    for (j = 0; j < 5 && *t; j++, t++)
			term_col++, putchar(*t);
		}
		else {
		    for (; *t && *t != ' '; t++)
			term_col++, putchar(*t);
		}
		if (btn == 3)
		    color_pop();	/* of COLOR_MOUSE */
		goto_xy(tcol,tline);
		fflush(stdout);
		if (btn == 3 && x > 0 && x < i)
		    pushstring(s,0);
		break;
	    }
	    if (!(x_clk -= i))
		break;
	    x -= i;
	    col += i;
	    s += strlen(s) + 1;
	}
	return TRUE;
    }
    return FALSE;
}

static int tc_string_cnt = 0;

static struct {
	char* capability;	/* name of capability, e.g. "forground red" */
	char* string;		/* escape sequence, e.g. "\033[31m" */
} tc_strings[TC_STRINGS];

/* Parse a line from the [termcap] section of trnrc. */
void
add_tc_string(capability, string)
char* capability;
char* string;
{
    int i;

    for (i = 0; i < tc_string_cnt; i++) {
	if (strEQ(capability, tc_strings[i].capability)) {
	    free(tc_strings[i].string);
	    break;
	}
    }
    if (i == tc_string_cnt) {
	if (tc_string_cnt == TC_STRINGS) {
	    fprintf(stderr,"trn: too many colors in [termcap] section (max is %d).\n",
		    TC_STRINGS);
	    finalize(1);
	}
	tc_string_cnt++;
	tc_strings[i].capability = savestr(capability);
    }

    tc_strings[i].string = savestr(string);
}

/* Return the named termcap color capability's string. */
char*
tc_color_capability(capability)
char* capability;
{
    int c;

    for (c = 0; c < tc_string_cnt; c++) {
	if (strEQ(tc_strings[c].capability, capability))
	    return tc_strings[c].string;
    }
    return NULL;
}

#ifdef MSDOS
int
tputs(str,num,func)
char* str;
int num;
int (*func) _((char_int));
{
    printf(str,num);
    return 0;
}
#endif

#ifdef MSDOS
char*
tgoto(str,x,y)
char* str;
int x;
int y;
{
    static char gbuf[32];
    sprintf(gbuf,str,y+1,x+1);
    return gbuf;
}
#endif
