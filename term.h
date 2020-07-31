/* term.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT char ERASECH;		/* rubout character */
EXT char KILLCH;		/* line delete character */
EXT char circlebuf[PUSHSIZE];
EXT int nextin INIT(0);
EXT int nextout INIT(0);
EXT unsigned char lastchar;

/* stuff wanted by terminal mode diddling routines */

#ifdef I_TERMIO
EXT struct termio _tty, _oldtty;
#else
# ifdef I_TERMIOS
EXT struct termios _tty, _oldtty;
# else
#  ifdef I_SGTTY
EXT struct sgttyb _tty;
EXT int _res_flg INIT(0);
#  endif
# endif
#endif

EXT int _tty_ch INIT(2);
EXT bool bizarre INIT(FALSE);		/* do we need to restore terminal? */

/* terminal mode diddling routines */

#ifdef I_TERMIO

#define crmode() ((bizarre=1),_tty.c_lflag &=~ICANON,_tty.c_cc[VMIN] = 1,ioctl(_tty_ch,TCSETAF,&_tty))
#define nocrmode() ((bizarre=1),_tty.c_lflag |= ICANON,_tty.c_cc[VEOF] = CEOF,stty(_tty_ch,&_tty))
#define echo()	 ((bizarre=1),_tty.c_lflag |= ECHO, ioctl(_tty_ch, TCSETA, &_tty))
#define noecho() ((bizarre=1),_tty.c_lflag &=~ECHO, ioctl(_tty_ch, TCSETA, &_tty))
#define nl()	 ((bizarre=1),_tty.c_iflag |= ICRNL,_tty.c_oflag |= ONLCR,ioctl(_tty_ch, TCSETAW, &_tty))
#define nonl()	 ((bizarre=1),_tty.c_iflag &=~ICRNL,_tty.c_oflag &=~ONLCR,ioctl(_tty_ch, TCSETAW, &_tty))
#define	savetty() (ioctl(_tty_ch, TCGETA, &_oldtty),ioctl(_tty_ch, TCGETA, &_tty))
#define	resetty() ((bizarre=0),ioctl(_tty_ch, TCSETAF, &_oldtty))
#define unflush_output()

#else /* !I_TERMIO */
# ifdef I_TERMIOS

#define crmode() ((bizarre=1), _tty.c_lflag &= ~ICANON,_tty.c_cc[VMIN]=1,tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define nocrmode() ((bizarre=1),_tty.c_lflag |= ICANON,_tty.c_cc[VEOF] = CEOF,tcsetattr(_tty_ch, TCSAFLUSH,&_tty))
#define echo()	 ((bizarre=1),_tty.c_lflag |= ECHO, tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define noecho() ((bizarre=1),_tty.c_lflag &=~ECHO, tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define nl()	 ((bizarre=1),_tty.c_iflag |= ICRNL,_tty.c_oflag |= ONLCR,tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define nonl()	 ((bizarre=1),_tty.c_iflag &=~ICRNL,_tty.c_oflag &=~ONLCR,tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define	savetty() (tcgetattr(_tty_ch, &_oldtty),tcgetattr(_tty_ch, &_tty))
#define	resetty() ((bizarre=0),tcsetattr(_tty_ch, TCSAFLUSH, &_oldtty))
#define unflush_output()

# else /* !I_TERMIOS */
#  ifdef I_SGTTY

#define raw()	 ((bizarre=1),_tty.sg_flags|=RAW, stty(_tty_ch,&_tty))
#define noraw()	 ((bizarre=1),_tty.sg_flags&=~RAW,stty(_tty_ch,&_tty))
#define crmode() ((bizarre=1),_tty.sg_flags |= CBREAK, stty(_tty_ch,&_tty))
#define nocrmode() ((bizarre=1),_tty.sg_flags &= ~CBREAK,stty(_tty_ch,&_tty))
#define echo()	 ((bizarre=1),_tty.sg_flags |= ECHO, stty(_tty_ch, &_tty))
#define noecho() ((bizarre=1),_tty.sg_flags &= ~ECHO, stty(_tty_ch, &_tty))
#define nl()	 ((bizarre=1),_tty.sg_flags |= CRMOD,stty(_tty_ch, &_tty))
#define nonl()	 ((bizarre=1),_tty.sg_flags &= ~CRMOD, stty(_tty_ch, &_tty))
#define	savetty() (gtty(_tty_ch, &_tty), _res_flg = _tty.sg_flags)
#define	resetty() ((bizarre=0),_tty.sg_flags = _res_flg, stty(_tty_ch, &_tty))
#   ifdef LFLUSHO
EXT int lflusho INIT(LFLUSHO);
#define unflush_output() (ioctl(_tty_ch,TIOCLBIC,&lflusho))
#   else /*! LFLUSHO */
#define unflush_output()
#   endif
#  else
#   ifdef MSDOS

#define crmode() (bizarre=1)
#define nocrmode() (bizarre=1)
#define echo()	 (bizarre=1)
#define noecho() (bizarre=1)
#define nl()	 (bizarre=1)
#define nonl()	 (bizarre=1)
#define	savetty()
#define	resetty() (bizarre=0)
#define unflush_output()
#   else /* !MSDOS */
..."Don't know how to define the term macros!"
#   endif /* !MSDOS */
#  endif /* !I_SGTTY */
# endif /* !I_TERMIOS */

#endif /* !I_TERMIO */

#ifdef TIOCSTI
#define forceme(c) ioctl(_tty_ch,TIOCSTI,c) /* pass character in " " */
#else
#define forceme(c)
#endif

/* termcap stuff */

/*
 * NOTE: if you don't have termlib you'll either have to define these strings
 *    and the tputs routine, or you'll have to redefine the macros below
 */

#ifdef HAS_TERMLIB
EXT int tc_GT;				/* hardware tabs */
EXT char* tc_BC INIT(NULL);		/* backspace character */
EXT char* tc_UP INIT(NULL);		/* move cursor up one line */
EXT char* tc_CR INIT(NULL);		/* get to left margin, somehow */
EXT char* tc_VB INIT(NULL);		/* visible bell */
EXT char* tc_CL INIT(NULL);		/* home and clear screen */
EXT char* tc_CE INIT(NULL);		/* clear to end of line */
EXT char* tc_TI INIT(NULL);		/* initialize terminal */
EXT char* tc_TE INIT(NULL);		/* reset terminal */
EXT char* tc_KS INIT(NULL);		/* enter `keypad transmit' mode */
EXT char* tc_KE INIT(NULL);		/* exit `keypad transmit' mode */
EXT char* tc_CM INIT(NULL);		/* cursor motion */
EXT char* tc_HO INIT(NULL);		/* home cursor */
EXT char* tc_IL INIT(NULL);		/* insert line */
EXT char* tc_CD INIT(NULL);		/* clear to end of display */
EXT char* tc_SO INIT(NULL);		/* begin standout mode */
EXT char* tc_SE INIT(NULL);		/* end standout mode */
EXT int tc_SG INIT(0);			/* blanks left by SO and SE */
EXT char* tc_US INIT(NULL);		/* start underline mode */
EXT char* tc_UE INIT(NULL);		/* end underline mode */
EXT char* tc_UC INIT(NULL);		/* underline a character,
						 if that's how it's done */
EXT int tc_UG INIT(0);			/* blanks left by US and UE */
EXT bool tc_AM INIT(FALSE);		/* does terminal have automatic
								 margins? */
EXT bool tc_XN INIT(FALSE);		/* does it eat 1st newline after
							 automatic wrap? */
EXT char tc_PC INIT(0);			/* pad character for use by tputs() */

#ifdef _POSIX_SOURCE
EXT speed_t outspeed INIT(0);		/* terminal output speed, */
#else
EXT long outspeed INIT(0);		/* 	for use by tputs() */
#endif

EXT int fire_is_out INIT(1);
EXT int tc_LINES INIT(0), tc_COLS INIT(0);/* size of screen */
EXT int term_line, term_col;		/* position of cursor */
EXT int term_scrolled;			/* how many lines scrolled away */
EXT int just_a_sec INIT(960);		/* 1 sec at current baud rate */
					/* (number of nulls) */

/* define a few handy macros */

#define termdown(x) term_line+=(x), term_col=0
#define newline() term_line++, term_col=0, putchar('\n') FLUSH
#define backspace() tputs(tc_BC,0,putchr) FLUSH
#define erase_eol() tputs(tc_CE,1,putchr) FLUSH
#define clear_rest() tputs(tc_CD,tc_LINES,putchr) FLUSH
#define maybe_eol() if(erase_screen&&erase_each_line)tputs(tc_CE,1,putchr) FLUSH
#define underline() tputs(tc_US,1,putchr) FLUSH
#define un_underline() fire_is_out|=UNDERLINE, tputs(tc_UE,1,putchr) FLUSH
#define underchar() tputs(tc_UC,0,putchr) FLUSH
#define standout() tputs(tc_SO,1,putchr) FLUSH
#define un_standout() fire_is_out|=STANDOUT, tputs(tc_SE,1,putchr) FLUSH
#define up_line() term_line--, tputs(tc_UP,1,putchr) FLUSH
#define insert_line() tputs(tc_IL,1,putchr) FLUSH
#define carriage_return() term_col=0, tputs(tc_CR,1,putchr) FLUSH
#define dingaling() tputs(tc_VB,1,putchr) FLUSH
#else /* !HAS_TERMLIB */
..."Don't know how to define the term macros!"
#endif /* !HAS_TERMLIB */

#define input_pending() finput_pending(TRUE)
#define macro_pending() finput_pending(FALSE)

EXT int page_line INIT(1);	/* line number for paging in
				 print_line (origin 1) */
EXT bool error_occurred INIT(FALSE);

EXT char* mousebar_btns;
EXT int mousebar_cnt INIT(0);
EXT int mousebar_start INIT(0);
EXT int mousebar_width INIT(0);
EXT bool xmouse_is_on INIT(FALSE);
EXT bool mouse_is_down INIT(FALSE);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void term_init _((void));
void term_set _((char*));
void set_macro _((char*,char*));
void arrow_macros _((char*));
void mac_line _((char*,char*,int));
void show_macros _((void));
void set_mode _((char_int,char_int));
int putchr _((char_int));
void hide_pending _((void));
bool finput_pending _((bool_int));
bool finish_command _((int));
char* edit_buf _((char*,char*));
bool finish_dblchar _((void));
void eat_typeahead _((void));
void save_typeahead _((char*,int));
void settle_down _((void));
Signal_t alarm_catcher _((int));
int read_tty _((char*,int));
#if !defined(FIONREAD) && !defined(HAS_RDCHK) && !defined(MSDOS)
int circfill _((void));
#endif
void pushchar _((char_int));
void underprint _((char*));
#ifdef NOFIREWORKS
void no_sofire _((void));
void no_ulfire _((void));
#endif
void getcmd _((char*));
void pushstring _((char*,char_int));
int get_anything _((void));
int pause_getcmd _((void));
void in_char _((char*,char_int,char*));
void in_answer _((char*,char_int));
bool in_choice _((char*,char*,char*,char_int));
int print_lines _((char*,int));
int check_page_line _((void));
void page_start _((void));
void errormsg _((char*));
void warnmsg _((char*));
void pad _((int));
#ifdef VERIFY
void printcmd _((void));
#endif
void rubout _((void));
void reprint _((void));
void erase_line _((bool_int));
void clear _((void));
void home_cursor _((void));
void goto_xy _((int,int));
#ifdef SIGWINCH
Signal_t winch_catcher _((int));
#endif
void termlib_init _((void));
void termlib_reset _((void));
#ifdef NBG_SIGIO
Signal_t waitkey_sig_handler _((int));
#endif
bool wait_key_pause _((int));
void xmouse_init _((char*));
void xmouse_check _((void));
void xmouse_on _((void));
void xmouse_off _((void));
void draw_mousebar _((int,bool_int));
bool check_mousebar _((int,int,int,int,int,int));
void add_tc_string _((char*,char*));
char* tc_color_capability _((char*));
#ifdef MSDOS
int tputs _((char*,int,int(*) _((char_int))));
char* tgoto _((char*,int,int));
#endif
