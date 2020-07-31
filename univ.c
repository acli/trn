/* univ.c
 */

/* Universal selector
 *
 */

#include "EXTERN.h"
#include "common.h"
#include "env.h"
#include "hash.h"
#include "list.h"
#include "ngdata.h"
#include "util.h"
#include "util2.h"
#include "url.h"
#include "term.h"
#include "rcstuff.h"
#include "ng.h"
#include "cache.h"
#include "head.h"
#include "trn.h"
#include "rt-select.h"
#include "rt-util.h"
#include "final.h"
#include "help.h"
#ifdef SCORE
#include "score.h"
#endif
#include "INTERN.h"
#include "univ.h"
#include "univ.ih"

/* TODO:
 *
 * Be friendlier when a file has no contents.
 * Implement virtual groups (largely done)
 * Help scan mode replacement
 * Lots more to do...
 */

static bool univ_virt_pass_needed INIT(FALSE);

static int univ_item_counter INIT(1);

static bool univ_done_startup INIT(FALSE);

/* this score is part of the line format, so it is not ifdefed */
static int univ_min_score INIT(0);
static bool univ_use_min_score INIT(FALSE);

void
univ_init()
{
    univ_level = 0;
    univ_ever_init = 1;
}

void
univ_startup()
{
    bool sys_top_load;
    bool user_top_load;

    sys_top_load = FALSE;
    user_top_load = FALSE;

    /* later: make user top file an option or environment variable? */
    if (!univ_file_load("%+/univ/top","Top Level",(char*)NULL)) {
	univ_open();
	univ_title = savestr("Top Level");
	univ_fname = savestr("%+/univ/usertop");

	/* read in trn default top file */
	(void)univ_include_file("%X/sitetop");		/* pure local */
	sys_top_load = univ_include_file("%X/trn4top");
	user_top_load = univ_use_file("%+/univ/usertop", univ_title, NULL);

	if (!(sys_top_load || user_top_load)) {
	    /* last resort--all newsgroups */
	    univ_close();
	    univ_mask_load(savestr("*"),"All Newsgroups");
	}
	if (user_top_load) {
	    univ_usrtop = TRUE;
	}
    } else {
	univ_usrtop = TRUE;
    }
    univ_done_startup = TRUE;
}

void
univ_open()
{
    first_univ = last_univ = 0;
    sel_page_univ = sel_next_univ = 0;
    univ_fname = univ_title = univ_label = univ_tmp_file = NULL;
    univ_virt_pass_needed = FALSE;
    univ_ng_hash = univ_vg_hash = 0;
    univ_level++;
}

void
univ_close()
{
    UNIV_ITEM* node;
    UNIV_ITEM* nextnode;

    for (node = first_univ; node; node = nextnode) {
	univ_free_data(node);
	safefree(node->desc);
	nextnode = node->next;
	free((char*)node);
    }
    if (univ_tmp_file) {
	UNLINK(univ_tmp_file);
	free(univ_tmp_file);
    }
    safefree(univ_fname);
    safefree(univ_title);
    safefree(univ_label);
    if (univ_ng_hash) {
	hashdestroy(univ_ng_hash);
	univ_ng_hash = 0;
    }
    if (univ_vg_hash) {
	hashdestroy(univ_vg_hash);
	univ_vg_hash = 0;
    }
    first_univ = last_univ = 0;
    sel_page_univ = sel_next_univ = 0;
    univ_level--;
}

UNIV_ITEM*
univ_add(type, desc)
int type;
char* desc;
{
    UNIV_ITEM* node = first_univ;

    node = (UNIV_ITEM*)safemalloc(sizeof (UNIV_ITEM));

    node->flags = 0;
    if (desc)
	node->desc = savestr(desc);
    else
	node->desc = NULL;
    node->type = type;
    node->num = univ_item_counter++;
#ifdef SCORE
    node->score = 0;		/* consider other default scores? */
#endif
    node->next = NULL;
    node->prev = last_univ;
    if (last_univ)
	last_univ->next = node;
    else
	first_univ = node;
    last_univ = node;

    return node;
}

static void
univ_free_data(ui)
UNIV_ITEM* ui;
{
    if (!ui)
	return;

    switch (ui->type) {
      case UN_NONE:	/* cases where nothing is needed. */
      case UN_TXT:
      case UN_HELPKEY:
	break;
      case UN_DEBUG1:	/* methods that use the string */
	safefree(ui->data.str);
	break;
      case UN_GROUPMASK:	/* methods that have custom data */
	safefree(ui->data.gmask.title);
	safefree(ui->data.gmask.masklist);
	break;
      case UN_CONFIGFILE:
	safefree(ui->data.cfile.title);
	safefree(ui->data.cfile.fname);
	safefree(ui->data.cfile.label);
	break;
      case UN_NEWSGROUP:
      case UN_GROUP_DESEL:
	safefree(ui->data.group.ng);
	break;
      case UN_ARTICLE:
	safefree(ui->data.virt.ng);
	safefree(ui->data.virt.from);
	safefree(ui->data.virt.subj);
	break;
      case UN_VGROUP:
      case UN_VGROUP_DESEL:
	safefree(ui->data.vgroup.ng);
	break;
      case UN_TEXTFILE:
	safefree(ui->data.textfile.fname);
	break;
      case UN_DATASRC:	/* unimplemented methods */
      case UN_VIRTUAL1:
      default:
	break;
    }
}

/* not used now, but may be used later... */
int
univ_lines(ui)
UNIV_ITEM* ui;
{
    /* later use the type and all that jazz... */
    return 1;
}

/* not used now, but may be used later... */
char*
univ_desc_line(ui,linenum)
UNIV_ITEM* ui;				/* universal item */
int linenum;				/* which line to describe (0 base) */
{
    return ui->desc;
}

void
univ_add_text(txt)
char* txt;
{
    /* later check text for bad things */
    (void)univ_add(UN_TXT,txt);
}

/* temp for testing */
void
univ_add_debug(desc,txt)
char* desc;
char* txt;
{
    UNIV_ITEM* ui;
    /* later check text for bad things */
    ui = univ_add(UN_DEBUG1,desc);
    ui->data.str = savestr(txt);
}

void
univ_add_group(desc,grpname)
char* desc;
char* grpname;
{
    UNIV_ITEM* ui;
    char* s;
    HASHDATUM data;

    s = grpname;
    if (!s)
	return;
    /* later check grpname for bad things? */

    if (!univ_ng_hash)
	univ_ng_hash = hashcreate(701, HASH_DEFCMPFUNC);

    data = hashfetch(univ_ng_hash,grpname,strlen(grpname));

    if (data.dat_ptr) {
	/* group was already added */
	/* perhaps it is marked as deleted? */
	for (ui = first_univ; ui; ui = ui->next) {
	    if ((ui->type == UN_GROUP_DESEL) && ui->data.group.ng
	     && strEQ(ui->data.group.ng,grpname)) {
		/* undelete the newsgroup */
		ui->type = UN_NEWSGROUP;
	    }
	}
	return;
    }
    ui = univ_add(UN_NEWSGROUP,desc);
    ui->data.group.ng = savestr(grpname);
    data.dat_ptr = ui->data.group.ng;
    hashstorelast(data);
}

void
univ_add_mask(desc,mask)
char* desc;
char* mask;
{
    UNIV_ITEM* ui;

    ui = univ_add(UN_GROUPMASK,desc);
    ui->data.gmask.masklist = savestr(mask);
    ui->data.gmask.title = savestr(desc);
}

void
univ_add_file(desc,fname,label)
char* desc;
char* fname;				/* May be URL */
char* label;
{
    UNIV_ITEM* ui;

    ui = univ_add(UN_CONFIGFILE,desc);
    ui->data.cfile.title = savestr(desc);
    ui->data.cfile.fname = savestr(fname);
    if (label && *label)
	ui->data.cfile.label = savestr(label);
    else
	ui->data.cfile.label = NULL;
}

UNIV_ITEM*
univ_add_virt_num(desc,grp,art)
char* desc;
char* grp;
ART_NUM art;
{
    UNIV_ITEM* ui;

    ui = univ_add(UN_ARTICLE,desc);
    ui->data.virt.ng = savestr(grp);
    ui->data.virt.num = art;
    ui->data.virt.subj = NULL;
    ui->data.virt.from = NULL;
    return ui;
}

void
univ_add_textfile(desc,name)
char* desc;
char* name;
{
    UNIV_ITEM* ui;
    char* s;
    char* p;
    static char lbuf[1024];

    s = name;
    switch (*s) {
      /* later add URL handling */
      case ':':
	s++;
      default:
	/* XXX later have error checking on length */
	strcpy(lbuf,univ_fname);
	for (p = lbuf+strlen(lbuf); p > lbuf && *p != '/'; p--) ;
	if (p) {
	    *p++ = '/';
	    *p = '\0';
	    strcat(lbuf,s);
	    s = lbuf;
	}
	/* FALL THROUGH */
      case '~':	/* ...or full file names */
      case '%':
      case '/':
	ui = univ_add(UN_TEXTFILE,desc);
	ui->data.textfile.fname = savestr(filexp(s));
	break;
    }
}

/* mostly the same as the newsgroup stuff */
void
univ_add_virtgroup(grpname)
char* grpname;
{
    UNIV_ITEM* ui;
    char* s;
    HASHDATUM data;

    s = grpname;
    if (!s)
	return;
    /* later check grpname for bad things? */

    /* perhaps leave if group has no unread, or other factor */
    if (!univ_vg_hash)
	univ_vg_hash = hashcreate(701, HASH_DEFCMPFUNC);

    univ_virt_pass_needed = TRUE;
    data = hashfetch(univ_vg_hash,grpname,strlen(grpname));
    if (data.dat_ptr) {
	/* group was already added */
	/* perhaps it is marked as deleted? */
	for (ui = first_univ; ui; ui = ui->next) {
	    if ((ui->type == UN_VGROUP_DESEL) && ui->data.vgroup.ng
	     && strEQ(ui->data.vgroup.ng,grpname)) {
		/* undelete the newsgroup */
		ui->type = UN_VGROUP;
	    }
	}
	return;
    }
    ui = univ_add(UN_VGROUP,NULL);
    ui->data.vgroup.flags = (char)0;
#ifdef SCORE
    if (univ_use_min_score) {
	ui->data.vgroup.flags |= UF_VG_MINSCORE;
	ui->data.vgroup.minscore = univ_min_score;
    }
#endif
    ui->data.vgroup.ng = savestr(grpname);
    data.dat_ptr = ui->data.vgroup.ng;
    hashstorelast(data);
}

static bool univ_begin_found INIT(FALSE);
/* label to start working with */
static char* univ_begin_label INIT(NULL);

/* univ_DoMatch uses a modified Wildmat function which is
 * based on Rich $alz's wildmat, reduced to the simple case of *
 * and text.  The complete version can be found in wildmat.c.
 */
/* an improbable number */
#define ABORT			-42

/*
**  Match text and p, return TRUE, FALSE, or ABORT.
*/
static int
univ_DoMatch(text, p)
register char* text;
register char* p;
{
    register int	matched;

    for ( ; *p; text++, p++) {
	if (*p == '*') {
	    while (*++p == '*')
		/* Consecutive stars act just like one. */
		continue;
	    if (*p == '\0')
		/* Trailing star matches everything. */
		return TRUE;
	    while (*text)
		if ((matched = univ_DoMatch(text++, p)) != FALSE)
		    return matched;
	    return ABORT;
	}
	if (*text != *p) {
	    if (*text == '\0')
		return ABORT;
	    return FALSE;
	}
    }
    return *text == '\0';
}

/* type: 0=newsgroup, 1=virtual (more in future?) */
void
univ_use_pattern(pattern,type)
char* pattern;
int type;
{
    char* s = pattern;
    NGDATA* np;
    UNIV_ITEM* ui;

    if (!s || !*s) {
	printf("\ngroup pattern: empty regular expression\n") FLUSH;
	return;
    }
    /* XXX later: match all newsgroups in current datasrc to the pattern. */
    /* XXX later do a quick check to see if the group is a simple one. */

    if (*s == '!') {
	s++;
	switch (type) {
	  case 0:
	    for (ui = first_univ; ui; ui = ui->next) {
		if (ui->type == UN_NEWSGROUP && ui->data.group.ng
		  && univ_DoMatch(ui->data.group.ng,s) == TRUE) {
		    ui->type = UN_GROUP_DESEL;
		}
	    }
	    break;
	  case 1:
	    for (ui = first_univ; ui; ui = ui->next) {
		if (ui->type == UN_VGROUP && ui->data.vgroup.ng
		  && univ_DoMatch(ui->data.vgroup.ng,s) == TRUE) {
		    ui->type = UN_VGROUP_DESEL;
		}
	    }
	    break;
	}
    }
    else {
	switch (type) {
	  case 0:
	    for (np = first_ng; np; np = np->next) {
		if (univ_DoMatch(np->rcline,s) == TRUE) {
		    univ_add_group(np->rcline,np->rcline);
		}
	    }
	    break;
	  case 1:
	    for (np = first_ng; np; np = np->next) {
		if (univ_DoMatch(np->rcline,s) == TRUE) {
		    univ_add_virtgroup(np->rcline);
		}
	    }
	    break;
	}
    }
}

/* interprets a line of newsgroups, adding or subtracting each pattern */
/* Newsgroup patterns are separated by spaces and/or commas */
void
univ_use_group_line(line,type)
char* line;
int type;
{
    char* s;
    char* p;
    char ch;

    s = line;
    if (!s || !*s)
	return;

    /* newsgroup patterns will be separated by space(s) and/or comma(s) */
    while (*s) {
	while (*s == ' ' || *s == ',') s++;
	for (p = s; *p && *p != ' ' && *p != ','; p++) ;
	ch = *p;
	*p = '\0';
	univ_use_pattern(s,type);
	*p = ch;
	s = p;
    }
}

/* returns TRUE on success, FALSE otherwise */
static bool
univ_use_file(fname,title,label)
char* fname;
char* title;
char* label;
{
    static char lbuf[LBUFLEN];
    FILE* fp;
    char* s;
    char* p;
    char* open_name;
    bool save_temp;
    bool begin_top;	/* if FALSE, look for "begin group"
			   before interpreting */

    save_temp = FALSE;
    begin_top = TRUE;	/* default assumption (might be changed later) */
    p = NULL;

    if (!fname)
	return FALSE;	/* bad argument */

    s = fname;
    open_name = s;
    /* open URLs and translate them into local temporary filenames */
    if (strncaseEQ(fname,"URL:",4)) {
#ifdef USEURL
	s = fname;
	open_name = temp_filename();
	univ_tmp_file = open_name;
	if (!url_get(fname+4,open_name))
	    open_name = NULL;
	save_temp = TRUE;
	begin_top = FALSE;	/* we will need a "begin group" */
#else /* !USEURL */
	printf("This copy of trn does not have URL support.\n") FLUSH;
	open_name = NULL;
#endif /* USEURL */
    } else if (*s == ':') {	/* relative to last file's directory */
	printf("Colon filespec not supported for |%s|\n",s) FLUSH;
	open_name = NULL;
    }
    if (!open_name)
	return FALSE;
    univ_begin_found = begin_top;
    safefree0(univ_begin_label);
    if (label)
	univ_begin_label = savestr(label);
    fp = fopen(filexp(open_name),"r");
    if (!fp)
	return FALSE;		/* unsuccessful (XXX: complain) */
/* Later considerations:
 * 1. Long lines
 * 2. Backslash continuations
 */
    while ((s = fgets(lbuf,sizeof lbuf,fp)) != NULL) {
	if (!s)		/* end of file */
	    break;
	if (!univ_do_line(s))
	    break;	/* end of useful file */
    }
    fclose(fp);
    if (!univ_begin_found)
	printf("\"begin group\" not found.\n") FLUSH;
    if (univ_begin_label)
	printf("label not found: %s\n",univ_begin_label);
    if (univ_virt_pass_needed) {
	univ_virt_pass();
    }
    sort_univ();
    return TRUE;
}

static bool
univ_include_file(fname)
char* fname;
{
    char* old_univ_fname;
    bool retval;

    old_univ_fname = univ_fname;
    univ_fname = savestr(fname);	/* LEAK */
    retval = univ_use_file(univ_fname,univ_title,NULL);
    univ_fname = old_univ_fname;
    return retval;
}

/* do the '$' extensions of the line. */
static void
univ_do_line_ext1(desc,line)
char* desc;
char* line;			/* may be temporarily edited */
{
    char* s;
    char* p;
    char* q;
    ART_NUM a;
    char ch;

    s = line;

    s++;
    switch (*s) {
      case 'v':
        s++;
	switch (*s) {
	  case '0':		/* test vector: "desc" $v0 */
	    s++;
	    (void)univ_add_virt_num(desc? desc : s,
			   "news.software.readers",(ART_NUM)15000);
	    break;
	  case '1':		/* "desc" $v1 1500 news.admin */
	    /* XXX error checking */
	    s++;
	    while (isspace(*s)) s++;
	    p = s;
	    while (isdigit(*p)) p++;
	    ch = *p;
	    *p = '\0';
	    a = (ART_NUM)atoi(s);
	    *p = ch;
	    if (*p) {
		p++;
	        (void)univ_add_virt_num(desc ? desc : s, p, a);
	    }
	    break;
	  case 'g':		/* $vg [scorenum] news.* !news.foo.* */
	    p = s;
	    p++;
	    while (isspace(*p)) p++;
	    q = p;
	    if ((*p=='+') || (*p=='-'))
	      p++;
	    while (isdigit(*p)) p++;
	    if (isspace(*p)) {
	      *p = '\0';
	      univ_min_score = atoi(q);
	      univ_use_min_score = TRUE;
	      s = p;
	      s++;
	    }
	    univ_use_group_line(s,1);
	    univ_use_min_score = FALSE;
	    break;
	}
	break;
      case 't':		/* text file */
        s++;
	switch (*s) {
	  case '0':		/* test vector: "desc" $t0 */
	    univ_add_textfile(desc? desc : s, "/home/c/caadams/ztext");
	    break;
	}
	break;
	default:
	  break;
    }
}


/* if non-NULL, the description (printing name) of the entry */
static char* univ_line_desc;

/* returns FALSE when no more lines should be interpreted */
static bool
univ_do_line(line)
char* line;
{
    char* s;
    char* p;

    s = line + strlen(line)-1;
    if (*s == '\n')
	*s = '\0';				/* delete newline */

    s = line;
    while (isspace(*s)) s++;
    if (*s == '\0')
	return TRUE;	/* empty line */

    if (!univ_begin_found) {
	if (strncaseNE(s,"begin group",11))
	    return TRUE;	/* wait until "begin group" is found */
	univ_begin_found = TRUE;
    }
    if (univ_begin_label) {
	if (*s == '>' && s[1] == ':' && strEQ(s+2,univ_begin_label)) {
	    safefree0(univ_begin_label); /* interpret starting at next line */
	}
	return TRUE;
    }
    safefree0(univ_line_desc);
    if (*s == '"') {	/* description name */
	p = cpytill(s,s+1,'"');
	if (!*p) {
	    printf("univ: unmatched quote in string:\n\"%s\"\n", s) FLUSH;
	    return TRUE;
	}
	*p = '\0';
	univ_line_desc = savestr(s);
	s = p+1;
    }
    while (isspace(*s)) s++;
    if (strncaseEQ(s,"end group",9))
	return FALSE;
    if (strncaseEQ(s,"URL:",4)) {
	for (p = s; *p && *p != '>'; p++) ;
	if (*p) {
	    p++;
	    if (!*p)		/* empty label */
		p = NULL;
	    /* XXX later do more error checking */
	} else
	    p = NULL;
	/* description defaults to name */
	univ_add_file(univ_line_desc? univ_line_desc : s, s, p);
    }
    else {
	switch (*s) {
	  case '#':	/* comment */
	    break;
	  case ':':	/* relative to univ_fname */
	    /* XXX hack the variable and fall through */
	    if (univ_fname && strlen(univ_fname)+strlen(s) < 1020) {
		static char lbuf[1024];
		strcpy(lbuf,univ_fname);
		for (p = lbuf+strlen(lbuf); p > lbuf && *p != '/'; p--) ;
		if (p) {
		    *p++ = '/';
		    *p = '\0';
		    s++;
		    strcat(lbuf,s);
		    s = lbuf;
		}
	    } /* XXX later have else which will complain */
	    /* FALL THROUGH */
	  case '~':	/* ...or full file names */
	  case '%':
	  case '/':
	    for (p = s; *p && *p != '>'; p++) ;
	    if (*p) {
		if (strlen(s) < 1020) {
		    static char lbuf[1024];
		    strcpy(lbuf,s);
		    s = lbuf;

		    for (p = s; *p && *p != '>'; p++) ; /* XXX Ick! */
		    *p++ = '\0';	/* separate label */

		    if (!*p)		/* empty label */
			p = NULL;
		    /* XXX later do more error checking */
		}
	    } else
		p = NULL;
	    /* description defaults to name */
	    univ_add_file(univ_line_desc? univ_line_desc : s, filexp(s), p);
	    break;
	  case '-':	/* label within same file */
	    s++;
	    if (*s++ != '>') {
		/* XXX give an error message later */
		break;
	    }
	    if (univ_tmp_file)
		p = univ_tmp_file;
	    else
		p = univ_fname;
	    univ_add_file(univ_line_desc? univ_line_desc : s, univ_fname, s);
	    break;
	  case '>':
	    if (s[1] == ':')
		return FALSE;	/* label found, end of previous block */
	    break;	/* just ignore the line (print warning later?) */
	  case '@':	/* virtual newsgroup file */
	    break;	/* not used now */
	  case '&':     /* text file shortcut (for help files) */
	    s++;
	    univ_add_textfile(univ_line_desc? univ_line_desc : s, s);
	    break;
	  case '$':	/* extension 1 */
	    univ_do_line_ext1(univ_line_desc,s);
	    break;
	  default:
	    /* if there is a description, this must be a restriction list */
	    if (univ_line_desc) {
		univ_add_mask(univ_line_desc,s);
		break;
	    }
	    /* one or more newsgroups instead */
	    univ_use_group_line(s,0);
	    break;
	}
    }
    return TRUE;	/* continue reading */
}

/* features to return later (?):
 *   text files
 */

/* level generator */
bool
univ_file_load(fname,title,label)
char* fname;
char* title;
char* label;
{
    bool flag;

    univ_open();

    if (fname)
	univ_fname = savestr(fname);
    if (title)
	univ_title = savestr(title);
    if (label)
	univ_label = savestr(label);
    flag = univ_use_file(fname,title,label);
    if (!flag) {
	univ_close();
    }
    if (int_count) {
	int_count = 0;
    }
    if (finput_pending(TRUE)) {
	;		/* later, *maybe* eat input */
    }
    return flag;
}

/* level generator */
void
univ_mask_load(mask,title)
char* mask;
char* title;
{
    univ_open();

    univ_use_group_line(mask,0);
    if (title)
	univ_title = savestr(title);
    if (int_count) {
	int_count = 0;
    }
}

void
univ_redofile()
{
    char* tmp_fname;
    char* tmp_title;
    char* tmp_label;

    tmp_fname = (univ_fname ? savestr(univ_fname) : 0);
    tmp_title = (univ_title ? savestr(univ_title) : 0);
    tmp_label = (univ_label ? savestr(univ_label) : 0);

    univ_close();
    if (univ_level)
	(void)univ_file_load(tmp_fname,tmp_title,tmp_label);
    else
	univ_startup();

    safefree(tmp_fname);
    safefree(tmp_title);
    safefree(tmp_label);
}


static char*
univ_edit_new_userfile()
{
    char* s;
    FILE* fp;

    s = savestr(filexp("%+/univ/usertop"));	/* LEAK */

    /* later, create a new user top file, and return its filename.
     * later perhaps ask whether to create or edit current file.
     * note: the user may need to restart in order to use the new file.
     *       (trn could do a univ_redofile, but it may be confusing.)
     */

    /* if the file exists, do not create a new one */
    fp = fopen(s,"r");
    if (fp) {
	fclose(fp);
	return univ_fname;	/* as if this function was not called */
    }

    makedir(s,MD_FILE);

    fp = fopen(s,"w");
    if (!fp) {
	printf("Could not create new user file.\n");
	printf("Editing current system file\n") FLUSH;
	(void)get_anything();
	return univ_fname;
    }
    fprintf(fp,"# User Toplevel (Universal Selector)\n");
    fclose(fp);
    printf("New User Toplevel file created.\n") FLUSH;
    printf("After editing this file, exit and restart trn to use it.\n") FLUSH;
    (void)get_anything();
    univ_usrtop = TRUE;		/* do not overwrite this file */
    return s;
}

/* code adapted from edit_kfile in kfile.c */
/* XXX problem if elements expand to larger than cmd_buf */
void
univ_edit()
{
    char* s;

    if (univ_usrtop || !(univ_done_startup)) {
	if (univ_tmp_file) {
	    s = univ_tmp_file;
	} else {
	    s = univ_fname;
	}
    } else {
	s = univ_edit_new_userfile();
    }

    /* later consider directory push/pop pair around editing */
    (void)edit_file(s);
}

/* later use some internal pager */
void
univ_page_file(fname)
char* fname;
{
    if (!fname || !*fname)
	return;

    sprintf(cmd_buf,"%s ",
	    filexp(getval("HELPPAGER",getval("PAGER","more"))));
    strcat(cmd_buf, filexp(fname));
    termdown(3);
    resetty();			/* make sure tty is friendly */
    doshell(sh,cmd_buf);	/* invoke the shell */
    noecho();			/* and make terminal */
    crmode();			/*   unfriendly again */
    /* later: consider something else that will return the key, and
     *        returning different codes based on the key.
     */
    if (strnEQ(cmd_buf,"more ",5))
	get_anything();
}

static UNIV_ITEM* current_vg_ui;

/* virtual newsgroup second pass function */
/* called from within newsgroup */
void
univ_ng_virtual()
{
    switch (current_vg_ui->type) {
      case UN_VGROUP:
	univ_vg_addgroup();
	break;
      case UN_ARTICLE:
	/* get article number from message-id */
	break;
      default:
	break;
    }

    /* later, get subjects and article numbers when needed */
    /* also do things like check scores, add newsgroups, etc. */
}

static void
univ_vg_addart(a)
ART_NUM a;
{
    char* subj;
    char* from;
    char lbuf[70];
    UNIV_ITEM* ui;
    int score;

#ifdef SCORE
    score = sc_score_art(a,FALSE);
    if (univ_use_min_score && (score<univ_min_score))
	return;
#endif
    subj = fetchsubj(a,FALSE);
    if (!subj || !*subj)
	return;
    from = fetchfrom(a,FALSE);
    if (!from || !*from)
        from = "<No Author>";

    safecpy(lbuf,subj,sizeof lbuf - 4);
    /* later scan/replace bad characters */

    /* later consider author in description, scoring, etc. */
    ui = univ_add_virt_num(NULL,ngname,a);
#ifdef SCORE
    ui->score = score;
#endif
    ui->data.virt.subj = savestr(subj);
    ui->data.virt.from = savestr(from);
}


static void
univ_vg_addgroup()
{
    ART_NUM a;

/* later: allow was-read articles, etc... */
    for (a = article_first(firstart); a <= lastart; a = article_next(a)) {
	if (!article_unread(a))
	    continue;
	/* minimum score check */
	univ_vg_addart(a);
    }
}

/* returns do_newsgroup() value */
int
univ_visit_group_main(gname)
char* gname;
{
    int ret;
    NGDATA* np;
    bool old_threaded;

    if (!gname || !*gname)
	return NG_ERROR;

    np = find_ng(gname);
    if (!np) {
	printf("Univ/Virt: newsgroup %s not found!", gname) FLUSH;
	return NG_ERROR;
    }
    /* unsubscribed, bogus, etc. groups are not visited */
    if (np->toread <= TR_UNSUB)
      return NG_ERROR;

    set_ng(np);
    if (np != current_ng) {
	/* probably unnecessary... */
	recent_ng = current_ng;
	current_ng = np;
    }
    old_threaded = ThreadedGroup;
    ThreadedGroup = (use_threads && !(np->flags & NF_UNTHREADED));
    printf("\nScanning newsgroup %s\n",gname);
    ret = do_newsgroup(nullstr);
    ThreadedGroup = old_threaded;
    return ret;
}

/* LATER: allow the loop to be interrupted */
void
univ_virt_pass()
{
    UNIV_ITEM* ui;

    univ_ng_virtflag = TRUE;
    univ_virt_pass_needed = FALSE;

    for (ui = first_univ; ui; ui = ui->next) {
	if (input_pending()) {
	    /* later consider cleaning up the remains */
	    break;
	}
	switch (ui->type) {
	  case UN_VGROUP:
	    if (!ui->data.vgroup.ng)
		break;			/* XXX whine */
	    current_vg_ui = ui;
#ifdef SCORE
	    if (ui->data.vgroup.flags & UF_VG_MINSCORE) {
		univ_use_min_score = TRUE;
		univ_min_score = ui->data.vgroup.minscore;
	    }
#endif
	    (void)univ_visit_group(ui->data.vgroup.ng);
	    univ_use_min_score = FALSE;
	    /* later do something with return value */
	    univ_free_data(ui);
	    safefree(ui->desc);
	    ui->type = UN_DELETED;
	    break;
	  case UN_ARTICLE:
	    /* if article number is not set, visit newsgroup with callback */
	    /* later also check for descriptions */
	    if ((ui->data.virt.num) && (ui->desc))
	      break;
	    if (ui->data.virt.subj)
	      break;
	    current_vg_ui = ui;
	    (void)univ_visit_group(ui->data.virt.ng);
	    /* later do something with return value */
	    break;
	  default:
	    break;
	}
    }
    univ_ng_virtflag = FALSE;
}

static int
univ_order_number(ui1, ui2)
register UNIV_ITEM** ui1;
register UNIV_ITEM** ui2;
{
    return (int)((*ui1)->num - (*ui2)->num) * sel_direction;
}

#ifdef SCORE
static int
univ_order_score(ui1, ui2)
register UNIV_ITEM** ui1;
register UNIV_ITEM** ui2;
{
    if ((*ui1)->score != (*ui2)->score)
	return (int)((*ui2)->score - (*ui1)->score) * sel_direction;
    else
	return (int)((*ui1)->num - (*ui2)->num) * sel_direction;
}
#endif

void
sort_univ()
{
    int cnt,i;
    UNIV_ITEM* ui;
    UNIV_ITEM** lp;
    UNIV_ITEM** univ_sort_list;
    int (*sort_procedure)();

    cnt = 0;
    for (ui = first_univ; ui; ui = ui->next) {
	cnt++;
    }

    if (cnt<=1)
	return;

    switch (sel_sort) {
#ifdef SCORE
      case SS_SCORE:
	sort_procedure = univ_order_score;
	break;
#endif
      case SS_NATURAL:
      default:
	sort_procedure = univ_order_number;
	break;
    }

    univ_sort_list = (UNIV_ITEM**)safemalloc(cnt*sizeof(UNIV_ITEM*));
    for (lp = univ_sort_list, ui = first_univ; ui; ui = ui->next)
	*lp++ = ui;
    assert(lp - univ_sort_list == cnt);

    qsort(univ_sort_list, cnt, sizeof (UNIV_ITEM*), sort_procedure);

    first_univ = ui = univ_sort_list[0];
    for (i = cnt, lp = univ_sort_list; --i; lp++) {
	lp[0]->next = lp[1];
	lp[1]->prev = lp[0];
    }
    last_univ = lp[0];
    last_univ->next = NULL;

    free((char*)univ_sort_list);
}

/* return a description of the article */
/* do this better later, like the code in sadesc.c */
char*
univ_article_desc(ui)
UNIV_ITEM* ui;
{
    char* s;
    char* f;
    static char dbuf[200];
    static char sbuf[200];
    static char fbuf[200];

    s = ui->data.virt.subj;
    f = ui->data.virt.from;
    if (!f) {
	strcpy(fbuf,"<No Author> ");
    } else {
	safecpy(fbuf,compress_from(f,16),17);
    }
    if (!s) {
	strcpy(sbuf,"<No Subject>");
    } else {
	if ((s[0] == 'R') &&
	    (s[1] == 'e') &&
	    (s[2] == ':') &&
	    (s[3] == ' ')) {
	    sbuf[0] = '>';
	    safecpy(sbuf+1,s+4,79);
	} else {
	    safecpy(sbuf,s,80);
	}
    }
    fbuf[16] = '\0';
    sbuf[55] = '\0';
#ifdef SCORE
    sprintf(dbuf,"[%3d] %16s %s",ui->score,fbuf,sbuf);
#else
    sprintf(dbuf,"%16s %55s",fbuf,sbuf);
#endif
    for (s = dbuf; *s; s++) {
	if ((*s==Ctl('h')) || (*s=='\t') || (*s=='\n') || (*s=='\r')) {
	    *s = ' ';
	}
    }
    dbuf[70] = '\0';
    return dbuf;
}

/* Help start */

/* later: add online help as a new item type, add appropriate item
 *        to the new level
 */
void
univ_help_main(where)
int where;	/* what context were we in--use later for key help? */
{
    UNIV_ITEM *ui;
    bool flag;

    univ_open();
    univ_title = savestr("Extended Help");

    /* first add help on current mode */
    ui = univ_add(UN_HELPKEY, NULL);
    ui->data.i = where;

    /* later, do other mode sensitive stuff */

    /* site-specific help */
    univ_include_file("%X/sitehelp/top");

    /* read in main help file */
    univ_fname = savestr("%X/HelpFiles/top");
    flag = univ_use_file(univ_fname,univ_title,univ_label);

    /* later: if flag is not true, then add message? */
}

void
univ_help(where)
int where;
{
    univ_visit_help(where);	/* push old selector info to stack */
}

char*
univ_keyhelp_modestr(ui)
UNIV_ITEM* ui;
{
    switch (ui->data.i) {
      case UHELP_PAGE:
	return "Article Pager Mode";
      case UHELP_ART:
	return "Article Display/Selection Mode";
      case UHELP_NG:
	return "Newsgroup Browse Mode";
      case UHELP_NGSEL:
	return "Newsgroup Selector";
      case UHELP_ADDSEL:
	return "Add-Newsgroup Selector";
#ifdef ESCSUBS
      case UHELP_SUBS:
	return "Escape Substitutions";
#endif
      case UHELP_ARTSEL:
	return "Thread/Subject/Article Selector";
      case UHELP_MULTIRC:
	return "Newsrc Selector";
      case UHELP_OPTIONS:
	return "Option Selector";
#ifdef SCAN
      case UHELP_SCANART:
	return "Article Scan Mode";
#endif
      case UHELP_UNIV:
	return "Universal Selector";
      default:
	return 0;
    }
}
