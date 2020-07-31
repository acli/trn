/* tktree.c
 *
 * XXX tktree.c should become some other name, like tkmisc?
 * Was only article tree drawing.  Now includes lots more.
 * Perhaps later allow much of it to be accessed from pure tcl.
 */

#include "EXTERN.h"
#include "common.h"
#ifdef USE_TK
#include <tcl.h>
#include <tk.h>
#include "hash.h"
#include "cache.h"
#include "rt-select.h"
#include "rt-wumpus.h"
#include "ng.h"
#include "util.h"
#include "tkstuff.h"
#include "term.h"
#ifdef SCORE
#include "ngdata.h"			/* absfirst */
#include "score.h"
#endif
#include "INTERN.h"
#include "tktree.h"
#include "tktree.ih"


extern HASHTABLE* msgid_hash;
extern Tcl_Interp* ttcl_interp;

static int ttk_article_counter = 0;

/* linked variable for passing a messageid */
static char* ttk_msgid = 0;

/* variables for tree dimensions (x and y) */
static int ttk_tree_x = 0;
static int ttk_tree_y = 0;

typedef struct {
    ARTICLE* ap;
} TTK_ART;

static void
ttk_article_delete(cd)
ClientData cd;
{
    safefree(cd);
}

/* this one is named "ttk__art0" */
static TTK_ART* ttk_fastart;

static char ttk_letters[] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+";

static void
ttk_treeprep_helper(ap)
ARTICLE* ap;
{
    for (;;) {
	ap->flags2 &= ~(AF2_WASUNREAD|AF2_CHANGED|AF2_NODEDRAWN);
	if (ap->flags & AF_UNREAD)
	    ap->flags2 |= AF2_WASUNREAD;
	if (ap->child1)
	    ttk_treeprep_helper(ap->child1);
	if (!(ap = ap->sibling))
	    break;
    }
}

static void
ttk_treechange_helper(ap)
ARTICLE* ap;
{
    int changed;			/* later indicate num of changes? */

    for (;;) {
	changed=0;
	if (ap->flags & AF_UNREAD) {
	    if (!(ap->flags2 & AF2_WASUNREAD))
		changed++;
	} else {
	    if (ap->flags2 & AF2_WASUNREAD)
		changed++;
	}
	if (ap->flags2 & AF2_CHANGED)
	    changed++;
	if (changed > 0) {
	    ttk_fastart->ap = ap;
	    ttcl_eval("eval $tree_nodechanged ttk__art0");
	    /* consider doing something with result */
	}
	if (ap->flags & AF_UNREAD)
	    ap->flags2 |= AF2_WASUNREAD;
	else
	    ap->flags2 &= ~AF2_WASUNREAD;
	ap->flags2 &= ~AF2_CHANGED;
	if (ap->child1)
	    ttk_treechange_helper(ap->child1);
	if (!(ap = ap->sibling))
	    break;
    }
}

static int
ttk_article_objectcmd(cd, interp, argc, argv)
ClientData cd;
Tcl_Interp* interp;
int argc;
char* argv[];
{
    TTK_ART* p = (TTK_ART*)cd;
    ARTICLE* ap;
    char* cmd;
    char* s;
    HASHDATUM data;

    if (argc < 2) {
	interp->result = "not enough args";
	return TCL_ERROR;
    }
    ap = p->ap;
    cmd = argv[1];

    if (strEQ(cmd,"setid")) {
	if (argc != 3) {
	    interp->result = "setid: needs id argument";
	    return TCL_ERROR;
	} 
	data = hashfetch(msgid_hash, argv[2], strlen(argv[2]));
	if (!(ap = (ARTICLE*)data.dat_ptr) || data.dat_len) {
	    interp->result = "0";
	    p->ap = 0;
	} else {
	    interp->result = "1";
	    p->ap = ap;
	}
	return TCL_OK;
    }
    if (strEQ(cmd,"setcurrent")) {
	if (argc != 2) {
	    interp->result = "setcurrent: too many arguments";
	    return TCL_ERROR;
	} 
	ap = curr_artp;
	if (!ap) {
	    interp->result = "0";
	    p->ap = 0;
	} else {
	    interp->result = "1";
	    p->ap = ap;
	}
	return TCL_OK;
    }
    if (strEQ(cmd,"header")) {
	char* which;
	if (argc != 3) {
	    interp->result = "header: needs header name argument";
	    return TCL_ERROR;
	}
	which = argv[2];
	if (!ap) {
	    s = "NO ARTICLE";
	    /* return error? */
	}
	else if (strEQ(which,"from")) {
	    s = "NO AUTHOR";
	    if (ap->from) {
		s = ap->from;
	    }
	    Tcl_AppendResult(interp, s, 0);
	    return TCL_OK;
	}
	else if (strEQ(which,"subject")) {
	    s = "NO SUBJECT";
	    if (ap->subj) {
		s = ap->subj->str +((ap->flags & AF_HAS_RE) ? 0 : 4);
	    }
	}
	else if (strEQ(which,"msgid")) {
	    s = "NO ID (***ERROR***)";
	    if (ap->msgid) {
		s = ap->msgid;
	    }
	}
	else {
	    /* not a known header */
	    Tcl_AppendResult(interp, "unknown header", 0);
	    return TCL_ERROR;
	}
	Tcl_AppendResult(interp, s, 0);
	return TCL_OK;
    }
    if (strEQ(cmd,"move")) {
	char* dir;
	if (argc != 3) {
	    interp->result = "move: needs direction";
	    return TCL_ERROR;
	}
	dir = argv[2];

	if (!ap) {
	    /* error later? */
	    interp->result = "0";
	    return TCL_OK;
	}

	/* later add threadtop and subject directions */
	if (strEQ(dir,"parent")) {
	    ap = ap->parent;
	}
	else if (strEQ(dir,"sibling")) {
	    ap = ap->sibling;
	}
	else if (strEQ(dir,"child")) {
	    ap = ap->child1;
	}
	else {
	    interp->result = "move: invalid direction";
	    return TCL_ERROR;
	}
	p->ap = ap;
	if (ap) {
	    interp->result = "1";
	}
	else {
	    interp->result = "0";
	}
	return TCL_OK;
    }
    if (strEQ(cmd,"flag")) {
	char* flag;
	int status = 0;
	if (argc != 3) {
	    interp->result = "move: needs direction";
	    return TCL_ERROR;
	}
	flag = argv[2];
	if (!ap) {
	    /* error later? (not error if checking existence?) */
	    interp->result = "0";
	    return TCL_OK;
	}
	if (strEQ(flag,"current")) {
	    if (ap == curr_artp)
		status = 1;
	}
	else if (strEQ(flag,"exists")) {
	    if (ap->flags & AF_EXISTS)
		status = 1;
	}
	else if (strEQ(flag,"unread")) {
	    if (ap->flags & AF_UNREAD)
		status = 1;
	}
	else if (strEQ(flag,"selected")) {
	    if (ap->flags & AF_SEL)
		status = 1;
	}
	else if (strEQ(flag,"wasunread")) {
	    if (ap->flags2 & AF2_WASUNREAD)
		status = 1;
	}
	else if (strEQ(flag,"changed")) {
	    if (ap->flags2 & AF2_CHANGED)
		status = 1;
	}
	else if (strEQ(flag,"parent")) {
	    if (ap->parent)
		status = 1;
	}
	else if (strEQ(flag,"child")) {
	    if (ap->child1)
		status = 1;
	}
	else if (strEQ(flag,"nextsibling")) {
	    if (ap->sibling)
		status = 1;
	}
	else if (strEQ(flag,"priorsibling")) {
	    if ((ap->parent) && (ap->parent->child1)
	     && (ap != ap->parent->child1))
		status = 1;
	}
	else if (strEQ(flag,"parentsubject")) {
	    if ((ap->subj) && (ap->parent) && (ap->parent->subj)
	     && (ap->subj == ap->parent->subj))
		status = 1;
	}
	else if (strEQ(flag,"selectedonly")) {
/* XXX Move this out to a trn-globals section */
	    if (selected_only)
		status = 1;
	}
	else {
	    interp->result = "unknown flag";
	    return TCL_ERROR;
	}
	if (status) {
	    interp->result = "1";
	} else {
	    interp->result = "0";
	}
	return TCL_OK;
    }
    if (strEQ(cmd,"copy")) {
	TTK_ART* p2;
	if (argc != 2) {
	    interp->result = "too many arguments";
	    return TCL_ERROR;
	}
	p2 = (TTK_ART*)safemalloc(sizeof(TTK_ART));
	p2->ap = p->ap;
	sprintf(interp->result, "trn_article%d",ttk_article_counter++);
	Tcl_CreateCommand(interp,interp->result, ttk_article_objectcmd,
			  p2,ttk_article_delete);
	return TCL_OK;
    }
    if (strEQ(cmd,"subjectletter")) {
	int subj = 0;
	ARTICLE* p;
	SUBJECT* sp;

	if ((!ap) || !(ap->flags & AF_EXISTS) || !(ap->subj)) {
	    interp->result[0] = ' ';
	    interp->result[1] = '\0';

	    return TCL_OK;
	}
	p = ap;
	while (p->parent) {
	    p = p->parent;
	}
	sp = p->subj;
	while (sp != ap->subj) {
	    subj++;
	    sp = sp->thread_link;
	}
	interp->result[0] = ttk_letters[subj>9+26+26? 9+26+26:subj];
	interp->result[1] = '\0';
	return TCL_OK;
    }
    /* prepare article and all children for other tree usage */
    if (strEQ(cmd,"treeprepare")) {
	ttk_tree_x=0;
	ttk_tree_y=0;
	if (ap) {
	    ttk_treeprep_helper(ap);
	    interp->result = "1";
	} else {
	    interp->result = "0";
	}
	return TCL_OK;
    }
    /* call a function for all changed articles */
    if (strEQ(cmd,"treechange")) {
	if (ap) {
	    ttk_treechange_helper(ap);
	    interp->result = "1";
	} else {
	    interp->result = "0";
	}
	return TCL_OK;
    }
    /* get the article number for this article, or 0 if non-existant */
    if (strEQ(cmd,"artnum")) {
	ART_NUM num;
	if (ap) {
	    if (ap->flags & AF_EXISTS) {
		num = article_num(ap);
		if (num>0) {
		    sprintf(interp->result,"%d",(int)num);
		    return TCL_OK;
		}
	    }
	}
	interp->result = "0";
	return TCL_OK;
    }
    if (strEQ(cmd,"score")) {
#ifdef SCORE
	/* Consider a quick query to see if the article is scored... */
	ART_NUM num;
	int artscore;
	if (ap) {
	    if (ap->flags & AF_EXISTS) {
		num = article_num(ap);
		if (num>0) {
		    artscore = sc_score_art(num,TRUE);
		    sprintf(interp->result,"%d",artscore);
		    return TCL_OK;
		}
	    }
	}
	interp->result = "0";
	return TCL_OK;
#else
	interp->result = "article scoring not compiled in this trn executable";
	return TCL_ERROR;
#endif
    }
    if (strEQ(cmd,"scorequick")) {
#ifdef SCORE
	ART_NUM num;
	int artscore;
	if (ap) {
	    if (ap->flags & AF_EXISTS) {
		num = article_num(ap);
		if (num>0) {
		    if (SCORED(num)) {
			artscore = sc_score_art(num,TRUE);
		    } else {
			artscore = 0;
		    }
		    sprintf(interp->result,"%d",artscore);
		    return TCL_OK;
		}
	    }
	}
	interp->result = "0";
	return TCL_OK;
#else
	interp->result = "article scoring not compiled in this trn executable";
	return TCL_ERROR;
#endif
    }
    interp->result = "unknown article command";
    return TCL_ERROR;
}

/* XXX Cleanup the tree drawing, replace "wipetree" with special procedure */
/* Called from trn to draw an article tree. */
void
ttk_draw_tree(ap,x,y)
ARTICLE* ap;
int x,y;				/* starting X and Y positions */
{
    static char lbuf[100];

    if (!ttk_running)
	return;				/* no wasted time */
    if (!(ap) || (!ap->subj) || (!ap->subj->thread) ||
	(!ap->subj->thread->msgid)) {
	ttcl_eval("wipetree");
	return;
    }
    ttk_msgid = ap->subj->thread->msgid;
    strcpy(lbuf,"trn_draw_article_tree 0 0");
    Tcl_Eval(ttcl_interp,lbuf);
}

static int
ttk_article(cd, interp, argc, argv)
ClientData cd;
Tcl_Interp* interp;
int argc;
char* argv[];
{
    TTK_ART* p;

    /* error checking later */
    if (argc != 1) {
	interp->result = "wrong # args";
	return TCL_ERROR;
    }
    p = (TTK_ART*)safemalloc(sizeof(TTK_ART));
    p->ap = 0;
    sprintf(interp->result, "trn_article%d",ttk_article_counter++);
    Tcl_CreateCommand(interp,interp->result, ttk_article_objectcmd,
		      p,ttk_article_delete);
    return TCL_OK;
}

/* XXX XXX Move this elsewhere. (general TRN stuff, not just article trees) */
static int
ttk_trn(cd, interp, argc, argv)
ClientData cd;
Tcl_Interp* interp;
int argc;
char* argv[];
{
    char* cmd;
    static char lbuf[32];		/* long enough for integers... */

    cmd = argv[1];
    if (strEQ(cmd,"mode")) {
	lbuf[0] = mode;
	lbuf[1] = '\0';
	Tcl_AppendResult(interp, lbuf, 0);
	return TCL_OK;
    }
    if (strEQ(cmd,"pending")) {
	ttk_do_waiting_flag = 0;	/* do not update TK */
	if (finput_pending(1)) {
	    lbuf[0] = '1';
	} else {
	    lbuf[0] = '0';
	}
	ttk_do_waiting_flag = 1;	/* resume updating TK */
	lbuf[1] = '\0';
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, lbuf, 0);
	return TCL_OK;
    }
    interp->result = "unknown trn_misc command";
    return TCL_ERROR;
}

int
ttk_tree_init()
{
    char lbuf[20];

    /* create the fast shared ArticlePointer (ap) ttk__art0 */
    ttk_fastart = (TTK_ART*)safemalloc(sizeof(TTK_ART));
    ttk_fastart->ap = 0;
    strcpy(lbuf,"ttk__art0");
    Tcl_CreateCommand(ttcl_interp,lbuf,ttk_article_objectcmd,
		      ttk_fastart,ttk_article_delete);
    Tcl_LinkVar(ttcl_interp, "ttk_msgid", (char*)&ttk_msgid, TCL_LINK_STRING);
    Tcl_LinkVar(ttcl_interp, "ttk_tree_x", (char*)&ttk_tree_x, TCL_LINK_INT);
    Tcl_LinkVar(ttcl_interp, "ttk_tree_y", (char*)&ttk_tree_y, TCL_LINK_INT);
    Tcl_CreateCommand(ttcl_interp, "trn_article", ttk_article, 0, 0);
    Tcl_CreateCommand(ttcl_interp, "trn_misc", ttk_trn, 0, 0);
    return TCL_OK;
}
#endif /* USE_TK */
