/* tkstuff.h
 */

#ifdef USE_TK
EXT char* ttk_keys;

EXT int ttk_idle_flag;

/* if true, we are really running Tk */
EXT int ttk_running INIT(0);

/* if true, allow update via ttk_do_waiting_events() */
EXT int ttk_do_waiting_flag INIT(1);
#endif

/* if true, we are really running TCL */
EXT int ttcl_running INIT(0);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

#ifdef USE_TK
void ttk_do_waiting_events _((void));
void ttk_wait_for_input _((void));
#endif
void ttcl_init _((void));
void ttcl_finalize _((int));
void ttcl_set_int _((char*,int));
void ttcl_set_str _((char*,char*));
int ttcl_get_int _((char*));
char* ttcl_get_str _((char*));
void ttcl_eval _((char*));
