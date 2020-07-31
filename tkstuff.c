/* tkstuff.c
 */

#include "EXTERN.h"
#include "common.h"
#ifdef USE_TCL
#include "term.h"
#include "util.h"
#include "util2.h"
#include <tcl.h>
#ifdef USE_TK
#include <tk.h>
#include "tktree.h"
#endif
#include "INTERN.h"
#include "tkstuff.h"
#include "tkstuff.ih"

Tcl_Interp* ttcl_interp;	/* Interpreter for this application. */

#ifdef USE_TK
Tk_Window mainWindow;	/* The main window for the application. */

static char* ttk_idle_pending_buf;
#endif

/* all these separate ifdef blocks are to keep mkpro happy */

/* XXX clean up the waiting/get-input interaction... */
/* This needs to be made *fast* when nothing is waiting... */
#ifdef USE_TK
static void
ttk_waitidle_pending()
{
    if (ttk_idle_flag) {
	(void)Tcl_Eval(ttcl_interp,ttk_idle_pending_buf);
    }
}
#endif

#ifdef USE_TK
void
ttk_do_waiting_events()
{
    if (ttk_running) {
	ttk_waitidle_pending();
	if (ttk_do_waiting_flag) {
	    while (Tk_DoOneEvent(TK_DONT_WAIT))
		;				/* EMPTY */
	} else {
	    /* always at least do file events */
	    while (Tk_DoOneEvent(TK_FILE_EVENTS|TK_DONT_WAIT))
		;				/* EMPTY */
	}
    }
}
#endif

/* later maybe have options to wait a number of milliseconds, then
 * start some background task...
 */
#ifdef USE_TK
void
ttk_wait_for_input()
{
    if (ttk_running) {
	while (!finput_pending(1)) {
	    ttk_waitidle_pending();
	    Tk_DoOneEvent(0);
	}
    }
}
#endif /* USE_TK */

void
ttcl_init()
{
    char* name;
    char* class;
    static char pending_buf[64];

    /* we are *not* currently operating */
#ifdef USE_TK
    ttk_running = 0;
#endif
    ttcl_running = 0;

#ifdef USE_TK
    if (UseTk)
      UseTcl = 1;	/* TCL is required for TK */
#endif

    /* do we really want to run TCL? */
    if (!UseTcl) {
	return;
    }

#ifdef USE_TK
    if (UseTk) {
	/* set up the idle buffer */
	strcpy(pending_buf,"ttk_idlepending");
	ttk_idle_pending_buf = pending_buf;

	/* plain NULL pointer does not work (it is "NULL" in TCL) */
	ttk_keys = (char*)malloc(sizeof(char));
	if (!ttk_keys)		/* sheer paranoia */
	    return;
	*ttk_keys = '\0';
    }
#endif

    if (TCL_MAJOR_VERSION<7) {
	UseTcl = UseTk = 0;	/* TCL is too old */
	return;
    }

#ifdef USE_TK
    if (TK_MAJOR_VERSION<4) {
	UseTk = 0;	/* TK is too old.  (We probably won't compile.) */
    }
#endif

    ttcl_interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(ttcl_interp);
#endif

#ifdef USE_TK
    ttk_running = 1;	/* be optimistic */
#if TK_MINOR_VERSION == 0
    if (UseTk && (TK_MINOR_VERSION==0)) {
	/* TK 4.0 initialization */
	name = "tktrn";
	class = (char*)ckalloc((unsigned) (strlen(name) + 1));
	strcpy(class, name);
	class[0] = toupper((unsigned char) class[0]);
	mainWindow = Tk_CreateMainWindow(ttcl_interp, 0, name, class);
	ckfree(class);
	if (mainWindow == NULL) {
#if 0
	    /* XXX handle error better */
	    fprintf(stderr, "%s\n", ttcl_interp->result);
#endif
	    ttk_running = 0;
	}
    }
#endif
#endif

    if (Tcl_Init(ttcl_interp) == TCL_ERROR) {
#ifdef USE_TK
	ttk_running = 0;
#endif
	return;
    }
    ttcl_running = 1;

#ifdef USE_TK
    if (ttk_running) {
	if (Tk_Init(ttcl_interp) == TCL_ERROR) {
	    ttk_running = 0;
	}
#if TK_MINOR_VERSION > 0
	/* TK 4.1 or higher initialization */
	Tcl_StaticPackage(ttcl_interp, "Tk", Tk_Init,
		  (Tcl_PackageInitProc *) NULL);
#endif
    }
#endif

    /* later: call initializations of extensions */

#ifdef USE_TK
    if (ttk_running) {
	/* misc. initialization done *before* the user tkinit file */
	Tcl_LinkVar(ttcl_interp, "ttk_keys", (char*)&ttk_keys,
		    TCL_LINK_STRING);
	ttk_idle_flag = 0;
	Tcl_LinkVar(ttcl_interp, "ttk_idle_flag",
		    (char*)&ttk_idle_flag, TCL_LINK_INT);
	/* later make the article tree stuff optional, check value */
	(void)ttk_tree_init();
    }
#endif

    /* Load the user TCL startup code */
    if (Tcl_EvalFile(ttcl_interp, savestr(filexp("%+/tclinit")))
	!= TCL_OK) {
	/* XXX Later print some message about problem? */
	/* This file is optional, so don't do anything */
	;
    }

#ifdef USE_TK
    if (ttk_running) {
	if (Tcl_EvalFile(ttcl_interp, savestr(filexp("%+/tkinit")))
	    != TCL_OK) {
	    /* XXX Later print some message about problem? */
	    ttk_running = 0;		/* don't try to run */
	}
	/* do simple initialization internally */
	Tcl_Eval(ttcl_interp,
      "fileevent stdin readable { set ttk_keys \"$ttk_keys[read stdin 1]\" }");
	ttk_do_waiting_events();
    }
#endif
}

/* might use status later */
void
ttcl_finalize(status)
int status;
{
#ifdef USE_TK
    if (ttk_running)
      (void)Tcl_Eval(ttcl_interp,"ttk_finalize");
#endif
    if (ttcl_running)
      (void)Tcl_Eval(ttcl_interp,"ttcl_finalize");
}

void
ttcl_set_int(varname,val)
char* varname;
int val;
{
    static char buf[20];

    sprintf(buf,"%d",val);
    /* check errors later? */
    (void)Tcl_SetVar(ttcl_interp,varname,buf,0);
}

void
ttcl_set_str(varname,val)
char* varname;
char* val;
{
    (void)Tcl_SetVar(ttcl_interp,varname,val,0);
}

int
ttcl_get_int(varname)
char* varname;
{
    char* s;
    int result = 0;
    /* check errors later? */
    s = Tcl_GetVar(ttcl_interp,varname,0);
    if (!s)
	return 0;
    (void)Tcl_GetInt(ttcl_interp,s,&result);
    return result;
}

char*
ttcl_get_str(varname)
char* varname;
{
    return Tcl_GetVar(ttcl_interp,varname,0);
}

void
ttcl_eval(str)
char* str;
{
    static char buf[1024];
    char* p;
    int len;

    if (ttcl_running) {
	if ((len = strlen(str)) > 1020) {
	    p = savestr(str);
	} else {
	    strcpy(buf,str);
	    p = buf;
	}
	/* later do error checking */
	(void)Tcl_Eval(ttcl_interp,p);
	if (len > 1020) {
	    free(p);
	}
    }
}
#endif /* USE_TCL */
