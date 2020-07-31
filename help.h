/* help.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


/* help locations */
#define UHELP_PAGE 1
#define UHELP_ART 2
#define UHELP_NG 3
#define UHELP_NGSEL 4
#define UHELP_ADDSEL 5
#ifdef ESCSUBS
#define UHELP_SUBS 6
#endif
#define UHELP_ARTSEL 7
#define UHELP_MULTIRC 8
#define UHELP_OPTIONS 9
#ifdef SCAN
#define UHELP_SCANART 10
#endif
#define UHELP_UNIV 11


/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void help_init _((void));
int help_page _((void));
int help_art _((void));
int help_ng _((void));
int help_ngsel _((void));
int help_addsel _((void));
#ifdef ESCSUBS
int help_subs _((void));
#endif
int help_artsel _((void));
int help_multirc _((void));
int help_options _((void));
#ifdef SCAN
int help_scanart _((void));
#endif
int help_univ _((void));
int univ_key_help _((int));
