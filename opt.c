/* opt.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "hash.h"
#include "init.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "intrp.h"
#include "final.h"
#include "list.h"
#include "art.h"
#include "cache.h"
#include "head.h"
#include "ng.h"
#include "mime.h"
#include "only.h"
#include "ngdata.h"
#include "search.h"
#include "rt-select.h"
#include "univ.h"
#include "rt-page.h"
#include "charsubst.h"
#include "term.h"
#include "sw.h"
#ifdef SCAN
#include "scan.h"
#ifdef SCAN_ART
#include "scanart.h"
#endif
#endif
#ifdef SCORE
#include "score.h"
#include "scorefile.h"
#endif
#include "color.h"
#include "INTERN.h"
#include "opt.h"
#include "opt.ih"

COMPEX optcompex;

void
opt_init(argc,argv,tcbufptr)
int argc;
char* argv[];
char** tcbufptr;
{
    register int i;
    char* s;

    sel_grp_dmode = savestr(sel_grp_dmode) + 1;
    sel_art_dmode = savestr(sel_art_dmode) + 1;
    UnivSelBtnCnt = parse_mouse_buttons(&UnivSelBtns,
	"[Top]^ [PgUp]< [PgDn]> [ OK ]^j [Quit]q [Help]?");
    NewsrcSelBtnCnt = parse_mouse_buttons(&NewsrcSelBtns,
	"[Top]^ [PgUp]< [PgDn]> [ OK ]^j [Quit]q [Help]?");
    AddSelBtnCnt = parse_mouse_buttons(&AddSelBtns,
	"[Top]^ [Bot]$ [PgUp]< [PgDn]> [ OK ]Z [Quit]q [Help]?");
    OptionSelBtnCnt = parse_mouse_buttons(&OptionSelBtns,
	"[Find]/ [FindNext]/^j [Top]^ [Bot]$ [PgUp]< [PgDn]> [Use]^i [Save]S [Abandon]q [Help]?");
    NewsgroupSelBtnCnt = parse_mouse_buttons(&NewsgroupSelBtns,
	"[Top]^ [PgUp]< [PgDn]> [ OK ]Z [Quit]q [Help]?");
    NewsSelBtnCnt = parse_mouse_buttons(&NewsSelBtns,
	"[Top]^ [Bot]$ [PgUp]< [PgDn]> [KillPg]D [ OK ]Z [Quit]q [Help]?");
    ArtPagerBtnCnt = parse_mouse_buttons(&ArtPagerBtns,
	"[Next]n [Sel]+ [Quit]q [Help]h");

    prep_ini_words(options_ini);
    if (argc >= 2 && strEQ(argv[1],"-c"))
	checkflag=TRUE;			/* so we can optimize for -c */
    interp(*tcbufptr,TCBUF_SIZE,GLOBINIT);
    opt_file(*tcbufptr,tcbufptr,FALSE);

    option_def_vals = (char**)safemalloc(INI_LEN(options_ini)*sizeof(char*));
    bzero((char*)option_def_vals,INI_LEN(options_ini) * sizeof (char*));
    /* Set DEFHIDE and DEFMAGIC to current values and clear user_htype list */
    set_header_list(HT_DEFHIDE,HT_HIDE,nullstr);
    set_header_list(HT_DEFMAGIC,HT_MAGIC,nullstr);

    if (use_threads)
	s = getval("TRNRC","%+/trnrc");
    else
	s = getval("RNRC","%+/rnrc");
    ini_file = savestr(filexp(s));

    s = filexp("%+");
    if (stat(s,&filestat) < 0 || !S_ISDIR(filestat.st_mode)) {
	printf("Creating the directory %s.\n",s);
	if (makedir(s,MD_DIR) != 0) {
	    printf("Unable to create `%s'.\n",s);
	    finalize(1); /*$$??*/
	}
    }
    if (stat(ini_file,&filestat) == 0)
	opt_file(ini_file,tcbufptr,TRUE);
    if (!use_threads || (s = getenv("TRNINIT")) == NULL)
	s = getenv("RNINIT");
    if (*safecpy(*tcbufptr,s,TCBUF_SIZE)) {
	if (*s == '-' || *s == '+' || isspace(*s))
	    sw_list(*tcbufptr);
	else
	    sw_file(tcbufptr);
    }
    option_saved_vals = (char**)safemalloc(INI_LEN(options_ini)*sizeof(char*));
    bzero((char*)option_saved_vals,INI_LEN(options_ini) * sizeof (char*));
    option_flags = (char*)safemalloc(INI_LEN(options_ini)*sizeof(char));
    bzero(option_flags,INI_LEN(options_ini) * sizeof (char));

    if (argc > 1) {
	for (i = 1; i < argc; i++)
	    decode_switch(argv[i]);
    }
    init_compex(&optcompex);
}

void
opt_file(filename,tcbufptr,bleat)
char* filename;
char** tcbufptr;
bool_int bleat;
{
    char* filebuf = *tcbufptr;
    char* s;
    char* section;
    char* cond;
    int fd = open(filename,0);
	
    if (fd >= 0) {
	fstat(fd,&filestat);
	if (filestat.st_size >= TCBUF_SIZE-1) {
	    filebuf = saferealloc(filebuf,(MEM_SIZE)filestat.st_size+2);
	    *tcbufptr = filebuf;
	}
	if (filestat.st_size) {
	    int len = read(fd,filebuf,(int)filestat.st_size);
	    filebuf[len] = '\0';
	    prep_ini_data(filebuf,filename);
	    s = filebuf;
	    while ((s = next_ini_section(s,&section,&cond)) != NULL) {
		if (*cond && !check_ini_cond(cond))
		    continue;
		if (strEQ(section, "options")) {
		    s = parse_ini_section(s, options_ini);
		    if (!s)
			break;
		    set_options(INI_VALUES(options_ini));
		}
		else if (strEQ(section, "environment")) {
		    while (*s && *s != '[') {
			section = s;
			s += strlen(s) + 1;
			export(section,s);
			s += strlen(s) + 1;
		    }
		}
		else if (strEQ(section, "termcap")) {
		    while (*s && *s != '[') {
			section = s;
			s += strlen(s) + 1;
			add_tc_string(section, s);
			s += strlen(s) + 1;
		    }
		}
		else if (strEQ(section, "attribute")) {
		    while (*s && *s != '[') {
			section = s;
			s += strlen(s) + 1;
			color_rc_attribute(section, s);
			s += strlen(s) + 1;
		    }
		}
	    }
	}
	close(fd);
    }
    else if (bleat) {
	printf(cantopen,filename) FLUSH;
	/*termdown(1);*/
    }

    *filebuf = '\0';
}

#define YES(s) (*(s) == 'y' || *(s) == 'Y')
#define NO(s)  (*(s) == 'n' || *(s) == 'N')

void
set_options(vals)
char** vals;
{
    int limit = INI_LEN(options_ini);
    int i;
    for (i = 1; i < limit; i++) {
	if (*++vals)
	    set_option(i, *vals);
    }
}

void
set_option(num, s)
int num;
char* s;
{
    if (option_saved_vals) {
	if (!option_saved_vals[num]) {
	    option_saved_vals[num] = savestr(option_value(num));
	    if (!option_def_vals[num])
		option_def_vals[num] = option_saved_vals[num];
	}
    }
    else if (option_def_vals) {
	if (!option_def_vals[num])
	    option_def_vals[num] = savestr(option_value(num));
    }
    switch (num) {
      case OI_USE_THREADS:
	use_threads = YES(s);
	break;
      case OI_USE_MOUSE:
	UseMouse = YES(s);
	if (UseMouse) {
	    /* set up the Xterm mouse sequence */
	    set_macro("\033[M+3","\003");
	}
	break;
      case OI_MOUSE_MODES:
	safecpy(MouseModes, s, sizeof MouseModes);
	break;
      case OI_USE_UNIV_SEL:
	UseUnivSelector = YES(s);
	break;
      case OI_UNIV_SEL_CMDS:
	*UnivSelCmds = *s;
	if (s[1])
	    UnivSelCmds[1] = s[1];
	break;
      case OI_UNIV_SEL_BTNS:
	UnivSelBtnCnt = parse_mouse_buttons(&UnivSelBtns,s);
	break;
      case OI_UNIV_SEL_ORDER:
	set_sel_order(SM_UNIVERSAL,s);
	break;
      case OI_UNIV_FOLLOW:
	univ_follow = YES(s);
	break;
      case OI_USE_NEWSRC_SEL:
	UseNewsrcSelector = YES(s);
	break;
      case OI_NEWSRC_SEL_CMDS:
	*NewsrcSelCmds = *s;
	if (s[1])
	    NewsrcSelCmds[1] = s[1];
	break;
      case OI_NEWSRC_SEL_BTNS:
	NewsrcSelBtnCnt = parse_mouse_buttons(&NewsrcSelBtns,s);
	break;
      case OI_USE_ADD_SEL:
	UseAddSelector = YES(s);
	break;
      case OI_ADD_SEL_CMDS:
	*AddSelCmds = *s;
	if (s[1])
	    AddSelCmds[1] = s[1];
	break;
      case OI_ADD_SEL_BTNS:
	AddSelBtnCnt = parse_mouse_buttons(&AddSelBtns,s);
	break;
      case OI_USE_NEWSGROUP_SEL:
	UseNewsgroupSelector = YES(s);
	break;
      case OI_NEWSGROUP_SEL_ORDER:
	set_sel_order(SM_NEWSGROUP,s);
	break;
      case OI_NEWSGROUP_SEL_CMDS:
	*NewsgroupSelCmds = *s;
	if (s[1])
	    NewsgroupSelCmds[1] = s[1];
	break;
      case OI_NEWSGROUP_SEL_BTNS:
	NewsgroupSelBtnCnt = parse_mouse_buttons(&NewsgroupSelBtns,s);
	break;
      case OI_NEWSGROUP_SEL_STYLES:
	free(option_value(OI_NEWSGROUP_SEL_STYLES)-1);
	sel_grp_dmode = safemalloc(strlen(s)+2);
	*sel_grp_dmode++ = '*';
	strcpy(sel_grp_dmode, s);
	break;
      case OI_USE_NEWS_SEL:
	if (isdigit(*s))
	    UseNewsSelector = atoi(s);
	else
	    UseNewsSelector = YES(s)-1;
	break;
      case OI_NEWS_SEL_MODE: {
	int save_sel_mode = sel_mode;
	set_sel_mode(*s);
	if (save_sel_mode != SM_ARTICLE && save_sel_mode != SM_SUBJECT
	 && save_sel_mode != SM_THREAD) {
	    sel_mode = save_sel_mode;
	    set_selector(0,0);
	}
	break;
      }
      case OI_NEWS_SEL_ORDER:
	set_sel_order(sel_defaultmode,s);
	break;
      case OI_NEWS_SEL_CMDS:
	*NewsSelCmds = *s;
	if (s[1])
	    NewsSelCmds[1] = s[1];
	break;
      case OI_NEWS_SEL_BTNS:
	NewsSelBtnCnt = parse_mouse_buttons(&NewsSelBtns,s);
	break;
      case OI_NEWS_SEL_STYLES:
	free(option_value(OI_NEWS_SEL_STYLES)-1);
	sel_art_dmode = safemalloc(strlen(s)+2);
	*sel_art_dmode++ = '*';
	strcpy(sel_art_dmode, s);
	break;
      case OI_OPTION_SEL_CMDS:
	*OptionSelCmds = *s;
	if (s[1])
	    OptionSelCmds[1] = s[1];
	break;
      case OI_OPTION_SEL_BTNS:
	OptionSelBtnCnt = parse_mouse_buttons(&OptionSelBtns,s);
	break;
      case OI_AUTO_SAVE_NAME:
	if (!checkflag) {
	    if (YES(s)) {
		export("SAVEDIR",  "%p/%c");
		export("SAVENAME", "%a");
	    }
	    else if (strEQ(getval("SAVEDIR",nullstr),"%p/%c")
		  && strEQ(getval("SAVENAME",nullstr),"%a")) {
		export("SAVEDIR", "%p");
		export("SAVENAME", "%^C");
	    }
	}
	break;
      case OI_BKGND_THREADING:
	thread_always = !YES(s);
	break;
      case OI_AUTO_ARROW_MACROS: {
	int prev = auto_arrow_macros;
	if (YES(s) || *s == 'r' || *s == 'R')
	    auto_arrow_macros = 2;
	else
	    auto_arrow_macros = !NO(s);
	if (mode != 'i' && auto_arrow_macros != prev) {
	    char tmpbuf[1024];
	    arrow_macros(tmpbuf);
	}
	break;
      }
      case OI_READ_BREADTH_FIRST:
	breadth_first = YES(s);
	break;
      case OI_BKGND_SPINNER:
	bkgnd_spinner = YES(s);
	break;
      case OI_CHECKPOINT_NEWSRC_FREQUENCY:
	docheckwhen = atoi(s);
	break;
      case OI_SAVE_DIR:
	if (!checkflag) {
	    savedir = savestr(s);
	    if (cwd) {
		chdir(cwd);
		free(cwd);
	    }
	    cwd = savestr(filexp(s));
	}
	break;
      case OI_ERASE_SCREEN:
	erase_screen = YES(s);
	break;
      case OI_NOVICE_DELAYS:
	novice_delays = YES(s);
	break;
      case OI_CITED_TEXT_STRING:
	indstr = savestr(s);
	break;
      case OI_GOTO_LINE_NUM:
	gline = atoi(s)-1;
	break;
      case OI_FUZZY_NEWSGROUP_NAMES:
	fuzzyGet = YES(s);
	break;
      case OI_HEADER_MAGIC:
	if (!checkflag)
	    set_header_list(HT_MAGIC, HT_DEFMAGIC, s);
	break;
      case OI_HEADER_HIDING:
	set_header_list(HT_HIDE, HT_DEFHIDE, s);
	break;
      case OI_INITIAL_ARTICLE_LINES:
	initlines = atoi(s);
	break;
      case OI_APPEND_UNSUBSCRIBED_GROUPS:
	append_unsub = YES(s);
	break;
      case OI_FILTER_CONTROL_CHARACTERS:
	dont_filter_control = !YES(s);
	break;
      case OI_JOIN_SUBJECT_LINES:
	if (isdigit(*s))
	    change_join_subject_len(atoi(s));
	else
	    change_join_subject_len(YES(s)? 30 : 0);
	break;
      case OI_IGNORE_THRU_ON_SELECT:
	kill_thru_kludge = YES(s);
	break;
      case OI_AUTO_GROW_GROUPS:
	keep_the_group_static = !YES(s);
	break;
      case OI_MUCK_UP_CLEAR:
	muck_up_clear = YES(s);
	break;
      case OI_ERASE_EACH_LINE:
	erase_each_line = YES(s);
	break;
      case OI_SAVEFILE_TYPE:
	mbox_always = (*s == 'm' || *s == 'M');
	norm_always = (*s == 'n' || *s == 'N');
	break;
      case OI_PAGER_LINE_MARKING:
	if (isdigit(*s))
	    marking_areas = atoi(s);
	else
	    marking_areas = HALFPAGE_MARKING;
	if (NO(s))
	    marking = NOMARKING;
	else if (*s == 'u')
	    marking = UNDERLINE;
	else
	    marking = STANDOUT;
	break;
      case OI_OLD_MTHREADS_DATABASE:
	if (isdigit(*s))
	    olden_days = atoi(s);
	else
	    olden_days = YES(s);
	break;
      case OI_SELECT_MY_POSTS:
	if (NO(s))
	    auto_select_postings = 0;
	else {
	    switch (*s) {
	      case 't':
		auto_select_postings = '+';
		break;
	      case 'p':
		auto_select_postings = 'p';
		break;
	      default:
		auto_select_postings = '.';
		break;
	    }
	}
	break;
      case OI_MULTIPART_SEPARATOR:
	multipart_separator = savestr(s);
	break;
      case OI_AUTO_VIEW_INLINE:
	auto_view_inline = YES(s);
	break;
      case OI_NEWGROUP_CHECK:
	quickstart = !YES(s);
	break;
      case OI_RESTRICTION_INCLUDES_EMPTIES:
	empty_only_char = YES(s)? 'o' : 'O';
	break;
      case OI_CHARSET:
#ifdef CHARSUBST
	charsets = savestr(s);
#endif
	break;
      case OI_INITIAL_GROUP_LIST:
	if (isdigit(*s)) {
	    countdown = atoi(s);
	    suppress_cn = (countdown == 0);
	}
	else {
	    suppress_cn = NO(s);
	    if (!suppress_cn)
		countdown = 5;
	}
	break;
      case OI_RESTART_AT_LAST_GROUP:
	findlast = YES(s) * (mode == 'i'? 1 : -1);
	break;
      case OI_SCANMODE_COUNT:
#ifdef ARTSEARCH
	if (isdigit(*s))
	    scanon = atoi(s);
	else
	    scanon = YES(s)*3;
#endif
	break;
      case OI_TERSE_OUTPUT:
#if defined(VERBOSE) && defined(TERSE)
	verbose = !YES(s);
	if (!verbose)
	    novice_delays = FALSE;
#endif
	break;
      case OI_EAT_TYPEAHEAD:
	allow_typeahead = !YES(s);
	break;
      case OI_COMPRESS_SUBJECTS:
	unbroken_subjects = !YES(s);
	break;
      case OI_VERIFY_INPUT:
#ifdef VERIFY
	verify = YES(s);
#endif
	break;
      case OI_ARTICLE_TREE_LINES:
	if (isdigit(*s)) {
	    if ((max_tree_lines = atoi(s)) > 11)
		max_tree_lines = 11;
	} else
	    max_tree_lines = YES(s) * 6;
	break;
      case OI_WORD_WRAP_MARGIN:
	if (isdigit(*s))
	    word_wrap_offset = atoi(s);
	else if (YES(s))
	    word_wrap_offset = 8;
	else
	    word_wrap_offset = -1;
	break;
      case OI_DEFAULT_REFETCH_TIME:
	defRefetchSecs = text2secs(s, DEFAULT_REFETCH_SECS);
	break;
      case OI_ART_PAGER_BTNS:
	ArtPagerBtnCnt = parse_mouse_buttons(&ArtPagerBtns,s);
	break;
#ifdef SCAN
      case OI_SCAN_ITEMNUM:
	s_itemnum = YES(s);
	break;
      case OI_SCAN_VI:
	s_mode_vi = YES(s);
	break;
#ifdef SCAN_ART
      case OI_SCANA_FOLLOW:
	sa_follow = YES(s);
	break;
      case OI_SCANA_FOLD:
	sa_mode_fold = YES(s);
	break;
      case OI_SCANA_UNZOOMFOLD:
	sa_unzoomrefold = YES(s);
	break;
      case OI_SCANA_MARKSTAY:
	sa_mark_stay = YES(s);
	break;
      case OI_SCANA_DISPANUM:
	sa_mode_desc_artnum = YES(s);
	break;
      case OI_SCANA_DISPAUTHOR:
	sa_mode_desc_author = YES(s);
	break;
      case OI_SCANA_DISPSCORE:
#ifdef SCORE
	sa_mode_desc_score = YES(s);
#endif
	break;
      case OI_SCANA_DISPSUBCNT:
	sa_mode_desc_threadcount = YES(s);
	break;
      case OI_SCANA_DISPSUBJ:
#if 0
	/* CAA: for now, always on. */
	sa_mode_desc_subject = YES(s);
#endif
	break;
      case OI_SCANA_DISPSUMMARY:
	sa_mode_desc_summary = YES(s);
	break;
      case OI_SCANA_DISPKEYW:
	sa_mode_desc_keyw = YES(s);
	break;
#endif
#endif /* SCAN */
#ifdef SCORE
      case OI_SC_VERBOSE:
	sf_verbose = YES(s);
	break;
#endif
      case OI_USE_SEL_NUM:
	UseSelNum = YES(s);
	break;
      case OI_SEL_NUM_GOTO:
	SelNumGoto = YES(s);
	break;
      default:
	printf("*** Internal error: Unknown Option ***\n");
	break;
    }
}

void
save_options(filename)
char* filename;
{
    int i;
    int fd_in;
    FILE* fp_out;
    char* filebuf = NULL;
    char* line = NULL;
    static bool first_time = TRUE;

    sprintf(buf,"%s.new",filename);
    fp_out = fopen(buf,"w");
    if (!fp_out) {
	printf(cantcreate,buf);
	return;
    }
    if ((fd_in = open(filename,0)) >= 0) {
	char* cp;
	char* nlp = NULL;
	char* comments = NULL;
	fstat(fd_in,&filestat);
	if (filestat.st_size) {
	    int len;
	    filebuf = safemalloc((MEM_SIZE)filestat.st_size+2);
	    len = read(fd_in,filebuf,(int)filestat.st_size);
	    filebuf[len] = '\0';
	}
	close(fd_in);

	for (line = filebuf; line && *line; line = nlp) {
	    cp = line;
	    nlp = index(cp, '\n');
	    if (nlp)
		*nlp++ = '\0';
	    while (isspace(*cp)) cp++;
	    if (*cp == '[' && strnEQ(cp+1,"options]",8)) {
		for (cp += 9; isspace(*cp); cp++) ;
		if (!*cp)
		    break;
	    }
	    fputs(line, fp_out);
	    fputc('\n', fp_out);
	}
	for (line = nlp; line && *line; line = nlp) {
	    cp = line;
	    nlp = index(cp, '\n');
	    if (nlp)
		nlp++;
	    while (*cp != '\n' && isspace(*cp)) cp++;
	    if (*cp == '[')
		break;
	    if (isalpha(*cp))
		comments = NULL;
	    else if (!comments)
		comments = line;
	}
	if (comments)
	    line = comments;
    }
    else {
	char *t = use_threads? "T" : "";
	printf("\n\
This is the first save of the option file, %s.\n\
By default this file overrides your %sRNINIT variable, but if you\n\
want to continue to use an old-style init file (that overrides the\n\
settings in the option file), edit the option file and change the\n\
line that sets %sRNINIT.\n", ini_file, t, t);
	get_anything();
	fprintf(fp_out, "# trnrc file auto-generated\n[environment]\n");
	write_init_environment(fp_out);
	fprintf(fp_out, "%sRNINIT = ''\n\n", t);
    }
    fprintf(fp_out,"[options]\n");
    for (i = 1; options_ini[i].checksum; i++) {
	if (*options_ini[i].item == '*')
	    fprintf(fp_out,"# ==%s========\n",options_ini[i].item+1);
	else {
	    fprintf(fp_out,"%s = ",options_ini[i].item);
	    if (!option_def_vals[i])
		fputs("#default of ",fp_out);
	    fprintf(fp_out,"%s\n",quote_string(option_value(i)));
	    if (option_saved_vals[i]) {
		if (option_saved_vals[i] != option_def_vals[i])
		    free(option_saved_vals[i]);
		option_saved_vals[i] = NULL;
	    }
	}
    }
    if (line) {
	/*putc('\n',fp_out);*/
	fputs(line,fp_out);
    }
    fclose(fp_out);

    safefree(filebuf);

    if (first_time) {
	if (fd_in >= 0) {
	    sprintf(buf,"%s.old",filename);
	    UNLINK(buf);
	    RENAME(filename,buf);
	}
	first_time = FALSE;
    }
    else
	UNLINK(filename);

    sprintf(buf,"%s.new",filename);
    RENAME(buf,filename);
}

char*
option_value(num)
int num;
{
    switch (num) {
      case OI_USE_THREADS:
	return YESorNO(use_threads);
      case OI_USE_MOUSE:
	return YESorNO(UseMouse);
      case OI_MOUSE_MODES:
	return MouseModes;
      case OI_USE_UNIV_SEL:
	return YESorNO(UseUnivSelector);
      case OI_UNIV_SEL_CMDS:
	return UnivSelCmds;
      case OI_UNIV_SEL_BTNS:
	return expand_mouse_buttons(UnivSelBtns,UnivSelBtnCnt);
      case OI_UNIV_SEL_ORDER:
	return get_sel_order(SM_UNIVERSAL);
      case OI_UNIV_FOLLOW:
	return YESorNO(univ_follow);
	break;
      case OI_USE_NEWSRC_SEL:
	return YESorNO(UseNewsrcSelector);
      case OI_NEWSRC_SEL_CMDS:
	return NewsrcSelCmds;
      case OI_NEWSRC_SEL_BTNS:
	return expand_mouse_buttons(NewsrcSelBtns,NewsrcSelBtnCnt);
      case OI_USE_ADD_SEL:
	return YESorNO(UseAddSelector);
      case OI_ADD_SEL_CMDS:
	return AddSelCmds;
      case OI_ADD_SEL_BTNS:
	return expand_mouse_buttons(AddSelBtns,AddSelBtnCnt);
      case OI_USE_NEWSGROUP_SEL:
	return YESorNO(UseNewsgroupSelector);
      case OI_NEWSGROUP_SEL_ORDER:
	return get_sel_order(SM_NEWSGROUP);
      case OI_NEWSGROUP_SEL_CMDS:
	return NewsgroupSelCmds;
      case OI_NEWSGROUP_SEL_BTNS:
	return expand_mouse_buttons(NewsgroupSelBtns,NewsgroupSelBtnCnt);
      case OI_NEWSGROUP_SEL_STYLES: {
	char* s = sel_grp_dmode;
	while (s[-1] != '*') s--;
	return s;
      }
      case OI_USE_NEWS_SEL:
	if (UseNewsSelector < 1)
	    return YESorNO(UseNewsSelector+1);
	sprintf(buf,"%d",UseNewsSelector);
	return buf;
      case OI_NEWS_SEL_MODE: {
	int save_sel_mode = sel_mode;
	int save_Threaded = ThreadedGroup;
	char* s;
	ThreadedGroup = TRUE;
	set_selector(sel_defaultmode, 0);
	s = sel_mode_string;
	sel_mode = save_sel_mode;
	ThreadedGroup = save_Threaded;
	set_selector(0, 0);
	return s;
      }
      case OI_NEWS_SEL_ORDER:
	return get_sel_order(sel_defaultmode);
      case OI_NEWS_SEL_CMDS:
	return NewsSelCmds;
      case OI_NEWS_SEL_BTNS:
	return expand_mouse_buttons(NewsSelBtns,NewsSelBtnCnt);
      case OI_NEWS_SEL_STYLES: {
	char* s = sel_art_dmode;
	while (s[-1] != '*') s--;
	return s;
      }
      case OI_OPTION_SEL_CMDS:
	return OptionSelCmds;
      case OI_OPTION_SEL_BTNS:
	return expand_mouse_buttons(OptionSelBtns,OptionSelBtnCnt);
      case OI_AUTO_SAVE_NAME:
	return YESorNO(strEQ(getval("SAVEDIR",SAVEDIR),"%p/%c"));
      case OI_BKGND_THREADING:
	return YESorNO(!thread_always);
      case OI_AUTO_ARROW_MACROS:
	switch (auto_arrow_macros) {
	  case 2:
	    return "regular";
	  case 1:
	    return "alternate";
	  default:
	    return YESorNO(0);
	}
      case OI_READ_BREADTH_FIRST:
	return YESorNO(breadth_first);
      case OI_BKGND_SPINNER:
	return YESorNO(bkgnd_spinner);
      case OI_CHECKPOINT_NEWSRC_FREQUENCY:
	sprintf(buf,"%d",docheckwhen);
	return buf;
      case OI_SAVE_DIR:
	return savedir? savedir : "%./News";
      case OI_ERASE_SCREEN:
	return YESorNO(erase_screen);
      case OI_NOVICE_DELAYS:
	return YESorNO(novice_delays);
      case OI_CITED_TEXT_STRING:
	return indstr;
      case OI_GOTO_LINE_NUM:
	sprintf(buf,"%d",gline+1);
	return buf;
      case OI_FUZZY_NEWSGROUP_NAMES:
	return YESorNO(fuzzyGet);
      case OI_HEADER_MAGIC:
	return magic_list();
      case OI_HEADER_HIDING:
	return hidden_list();
      case OI_INITIAL_ARTICLE_LINES:
	if (!option_def_vals[OI_INITIAL_ARTICLE_LINES])
	    return "$LINES";
	sprintf(buf,"%d",initlines);
    	return buf;
      case OI_APPEND_UNSUBSCRIBED_GROUPS:
	return YESorNO(append_unsub);
      case OI_FILTER_CONTROL_CHARACTERS:
	return YESorNO(!dont_filter_control);
      case OI_JOIN_SUBJECT_LINES:
	if (join_subject_len) {
	    sprintf(buf,"%d",join_subject_len);
	    return buf;
	}
	return YESorNO(0);
      case OI_IGNORE_THRU_ON_SELECT:
	return YESorNO(kill_thru_kludge);
      case OI_AUTO_GROW_GROUPS:
	return YESorNO(!keep_the_group_static);
      case OI_MUCK_UP_CLEAR:
	return YESorNO(muck_up_clear);
      case OI_ERASE_EACH_LINE:
	return YESorNO(erase_each_line);
      case OI_SAVEFILE_TYPE:
	return mbox_always? "mail" : (norm_always? "norm" : "ask");
      case OI_PAGER_LINE_MARKING:
	if (marking == NOMARKING)
	    return YESorNO(0);
	if (marking_areas != HALFPAGE_MARKING)
	    sprintf(buf,"%d",marking_areas);
	else
	    *buf = '\0';
	if (marking == UNDERLINE)
	    strcat(buf,"underline");
	else
	    strcat(buf,"standout");
	return buf;
      case OI_OLD_MTHREADS_DATABASE:
	if (olden_days <= 1)
	    return YESorNO(olden_days);
	sprintf(buf,"%d",olden_days);
	return buf;
      case OI_SELECT_MY_POSTS:
	switch (auto_select_postings) {
	  case '+':
	    return "thread";
	  case 'p':
	    return "parent";
	  case '.':
	    return "subthread";
	  default:
	    break;
	}
	return YESorNO(0);
      case OI_MULTIPART_SEPARATOR:
	return multipart_separator;
      case OI_AUTO_VIEW_INLINE:
	return YESorNO(auto_view_inline);
      case OI_NEWGROUP_CHECK:
	return YESorNO(!quickstart);
      case OI_RESTRICTION_INCLUDES_EMPTIES:
	return YESorNO(empty_only_char == 'o');
      case OI_CHARSET:
#ifdef CHARSUBST
	return charsets;
#else
	return "<DISABLED>";
#endif
      case OI_INITIAL_GROUP_LIST:
	if (suppress_cn)
	    return YESorNO(0);
	sprintf(buf,"%d",countdown);
	return buf;
      case OI_RESTART_AT_LAST_GROUP:
	return YESorNO(findlast != 0);
#ifdef ARTSEARCH
      case OI_SCANMODE_COUNT:
	sprintf(buf,"%d",scanon);
	return buf;
#endif
#if defined(VERBOSE) && defined(TERSE)
      case OI_TERSE_OUTPUT:
	return YESorNO(!verbose);
#endif
      case OI_EAT_TYPEAHEAD:
	return YESorNO(!allow_typeahead);
      case OI_COMPRESS_SUBJECTS:
	return YESorNO(!unbroken_subjects);
#ifdef VERIFY
      case OI_VERIFY_INPUT:
	return YESorNO(verify);
#endif
      case OI_ARTICLE_TREE_LINES:
	sprintf(buf,"%d",max_tree_lines);
	return buf;
      case OI_WORD_WRAP_MARGIN:
	if (word_wrap_offset >= 0) {
	    sprintf(buf,"%d",word_wrap_offset);
	    return buf;
	}
	return YESorNO(0);
      case OI_DEFAULT_REFETCH_TIME:
	return secs2text(defRefetchSecs);
      case OI_ART_PAGER_BTNS:
	return expand_mouse_buttons(ArtPagerBtns,ArtPagerBtnCnt);
#ifdef SCAN
      case OI_SCAN_ITEMNUM:
	return YESorNO(s_itemnum);
      case OI_SCAN_VI:
	return YESorNO(s_mode_vi);
#ifdef SCAN_ART
      case OI_SCANA_FOLLOW:
	return YESorNO(sa_follow);
      case OI_SCANA_FOLD:
	return YESorNO(sa_mode_fold);
      case OI_SCANA_UNZOOMFOLD:
	return YESorNO(sa_unzoomrefold);
      case OI_SCANA_MARKSTAY:
	return YESorNO(sa_mark_stay);
      case OI_SCANA_DISPANUM:
	return YESorNO(sa_mode_desc_artnum);
      case OI_SCANA_DISPAUTHOR:
	return YESorNO(sa_mode_desc_author);
      case OI_SCANA_DISPSCORE:
	return YESorNO(sa_mode_desc_score);
      case OI_SCANA_DISPSUBCNT:
	return YESorNO(sa_mode_desc_threadcount);
      case OI_SCANA_DISPSUBJ:
	return YESorNO(sa_mode_desc_subject);
      case OI_SCANA_DISPSUMMARY:
	return YESorNO(sa_mode_desc_summary);
      case OI_SCANA_DISPKEYW:
	return YESorNO(sa_mode_desc_keyw);
#endif
#endif
#ifdef SCORE
      case OI_SC_VERBOSE:
	return YESorNO(sf_verbose);
#endif
      case OI_USE_SEL_NUM:
	return YESorNO(UseSelNum);
	break;
      case OI_SEL_NUM_GOTO:
	return YESorNO(SelNumGoto);
	break;
      default:
	printf("*** Internal error: Unknown Option ***\n");
	break;
    }
    return "<UNKNOWN>";
}

static char*
hidden_list()
{
    int i;
    buf[0] = '\0';
    buf[1] = '\0';
    for (i = 1; i < user_htype_cnt; i++) {
	sprintf(buf+strlen(buf),",%s%s", user_htype[i].flags? nullstr : "!",
		user_htype[i].name);
    }
    return buf+1;
}

static char*
magic_list()
{
    int i;
    buf[0] = '\0';
    buf[1] = '\0';
    for (i = HEAD_FIRST; i < HEAD_LAST; i++) {
	if (!(htype[i].flags & HT_MAGIC) != !(htype[i].flags & HT_DEFMAGIC)) {
	    sprintf(buf+strlen(buf),",%s%s",
		    (htype[i].flags & HT_DEFMAGIC)? "!" : nullstr,
		    htype[i].name);
	}
    }
    return buf+1;
}

static void
set_header_list(flag,defflag,str)
int flag;
int defflag;
char* str;
{
    int i;
    bool setit;
    char* cp;

    if (flag == HT_HIDE || flag == HT_DEFHIDE) {
	/* Free old user_htype list */
	while (user_htype_cnt > 1)
	    free(user_htype[--user_htype_cnt].name);
	bzero((char*)user_htypeix, sizeof user_htypeix);
    }

    if (!*str)
	str = " ";
    for (i = HEAD_FIRST; i < HEAD_LAST; i++)
	htype[i].flags = ((htype[i].flags & defflag)
			? (htype[i].flags | flag)
			: (htype[i].flags & ~flag));
    for (;;) {
	if ((cp = index(str,',')) != NULL)
	    *cp = '\0';
	if (*str == '!') {
	    setit = FALSE;
	    str++;
	}
	else
	    setit = TRUE;
	set_header(str,flag,setit);
	if (!cp)
	    break;
	*cp = ',';
	str = cp+1;
    }
}

void
set_header(s, flag, setit)
char* s;
int flag;
bool_int setit;
{
    int i;
    int len = strlen(s);
    for (i = HEAD_FIRST; i < HEAD_LAST; i++) {
	if (!len || strncaseEQ(s,htype[i].name,len)) {
	    if (setit && (flag != HT_MAGIC || (htype[i].flags & HT_MAGICOK)))
		htype[i].flags |= flag;
	    else
		htype[i].flags &= ~flag;
	}
    }
    if (flag == HT_HIDE && *s && isalpha(*s)) {
	char ch = isupper(*s)? tolower(*s) : *s;
	int add_at = 0, killed = 0;
	bool save_it = TRUE;
	for (i = user_htypeix[ch - 'a']; *user_htype[i].name == ch; i--) {
	    if (len <= user_htype[i].length
	     && strncaseEQ(s,user_htype[i].name,len)) {
		free(user_htype[i].name);
		user_htype[i].name = NULL;
		killed = i;
	    }
	    else if (len > user_htype[i].length
		  && strncaseEQ(s,user_htype[i].name,user_htype[i].length)) {
		if (!add_at) {
		    if (user_htype[i].flags == (setit? flag : 0))
			save_it = FALSE;
		    add_at = i+1;
		}
	    }
	}
	if (save_it) {
	    if (!killed || (add_at && user_htype[add_at].name)) {
		if (user_htype_cnt >= user_htype_max) {
		    user_htype = (USER_HEADTYPE*)
			realloc(user_htype, (user_htype_max += 10)
					    * sizeof (USER_HEADTYPE));
		}
		if (!add_at) {
		    add_at = user_htypeix[ch - 'a']+1;
		    if (add_at == 1)
			add_at = user_htype_cnt;
		}
		for (i = user_htype_cnt; i > add_at; i--)
		    user_htype[i] = user_htype[i-1];
		user_htype_cnt++;
	    }
	    else if (!add_at)
		add_at = killed;
	    user_htype[add_at].length = len;
	    user_htype[add_at].flags = setit? flag : 0;
	    user_htype[add_at].name = savestr(s);
	    for (s = user_htype[add_at].name; *s; s++) {
		if (isupper(*s))
		    *s = tolower(*s);
	    }
	}
	if (killed) {
	    while (killed < user_htype_cnt && user_htype[killed].name != NULL)
		killed++;
	    for (i = killed+1; i < user_htype_cnt; i++) {
		if (user_htype[i].name != NULL)
		    user_htype[killed++] = user_htype[i];
	    }
	    user_htype_cnt = killed;
	}
	bzero((char*)user_htypeix, sizeof user_htypeix);
	for (i = 1; i < user_htype_cnt; i++)
	    user_htypeix[*user_htype[i].name - 'a'] = i;
    }
}

static int
parse_mouse_buttons(cpp, btns)
char** cpp;
char* btns;
{
    char* t = *cpp;
    int cnt = 0;

    safefree(t);
    while (*btns == ' ') btns++;
    t = *cpp = safemalloc(strlen(btns)+1);

    while (*btns) {
	if (*btns == '[') {
	    if (!btns[1])
		break;
	    while (*btns) {
		if (*btns == '\\' && btns[1])
		    btns++;
		else if (*btns == ']')
		    break;
		*t++ = *btns++;
	    }
	    *t++ = '\0';
	    if (*btns)
		while (*++btns == ' ') ;
	}
	while (*btns && *btns != ' ') *t++ = *btns++;
	while (*btns == ' ') btns++;
	*t++ = '\0';
	cnt++;
    }

    return cnt;
}

static char*
expand_mouse_buttons(cp, cnt)
char* cp;
int cnt;
{
    *buf = '\0';
    while (cnt--) {
	if (*cp == '[') {
	    int len = strlen(cp);
	    cp[len] = ']';
	    safecat(buf,cp,sizeof buf);
	    cp += len;
	    *cp++ = '\0';
	}
	else
	    safecat(buf,cp,sizeof buf);
	cp += strlen(cp)+1;
    }
    return buf;
}

char*
quote_string(val)
char* val;
{
    static char* bufptr = NULL;
    char* cp;
    int needs_quotes = 0;
    int ticks = 0;
    int quotes = 0;
    int backslashes = 0;

    safefree(bufptr);
    bufptr = NULL;

    if (isspace(*val))
	needs_quotes = 1;
    for (cp = val; *cp; cp++) {
	switch (*cp) {
	  case ' ':
	  case '\t':
	    if (!cp[1] || isspace(cp[1]))
		needs_quotes++;
	    break;
	  case '\n':
	  case '#':
	    needs_quotes++;
	    break;
	  case '\'':
	    ticks++;
	    break;
	  case '"':
	    quotes++;
	    break;
	  case '\\':
	    backslashes++;
	    break;
	}
    }

    if (needs_quotes || ticks || quotes || backslashes) {
	char usequote = quotes > ticks? '\'' : '"';
	bufptr = safemalloc(strlen(val)+2+(quotes > ticks? ticks : quotes)
			    +backslashes+1);
	cp = bufptr;
	*cp++ = usequote;
	while (*val) {
	    if (*val == usequote || *val == '\\')
		*cp++ = '\\';
	    *cp++ = *val++;
	}
	*cp++ = usequote;
	*cp = '\0';
	return bufptr;
    }
    return val;
}

void
cwd_check()
{
    char tmpbuf[LBUFLEN];

    if (!cwd)
	cwd = savestr(filexp("~/News"));
    strcpy(tmpbuf,cwd);
    if (chdir(cwd) != 0) {
	safecpy(tmpbuf,filexp(cwd),sizeof tmpbuf);
	if (makedir(tmpbuf,MD_DIR) != 0 || chdir(tmpbuf) != 0) {
	    interp(cmd_buf, (sizeof cmd_buf), "%~/News");
	    if (makedir(cmd_buf,MD_DIR) != 0)
		strcpy(tmpbuf,homedir);
	    else
		strcpy(tmpbuf,cmd_buf);
	    chdir(tmpbuf);
#ifdef VERBOSE
	    IF(verbose)
		printf("\
Cannot make directory %s--\n\
	articles will be saved to %s\n\
\n\
",cwd,tmpbuf) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\
Can't make %s--\n\
	using %s\n\
\n\
",cwd,tmpbuf) FLUSH;
#endif
	}
    }
    free(cwd);
    trn_getwd(tmpbuf, sizeof(tmpbuf));
    if (eaccess(tmpbuf,2)) {
#ifdef VERBOSE
	IF(verbose)
	    printf("\
Current directory %s is not writeable--\n\
	articles will be saved to home directory\n\n\
",tmpbuf) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    printf("%s not writeable--using ~\n\n",tmpbuf) FLUSH;
#endif
	strcpy(tmpbuf,homedir);
    }
    cwd = savestr(tmpbuf);
}
