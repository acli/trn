/* nntp.h
*/ 
/* This software is copyrighted as detailed in the LICENSE file. */


#ifdef SUPPORT_NNTP

#define FB_BACKGROUND	0
#define FB_OUTPUT	1
#define FB_SILENT	2
#define FB_DISCARD	3

#define MAX_NNTP_ARTICLES   10

#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

#ifdef SUPPORT_NNTP
int nntp_list _((char*,char*,int));
#endif
void nntp_finish_list _((void));
int nntp_group _((char*,NGDATA*));
int nntp_stat _((ART_NUM));
ART_NUM nntp_stat_id _((char*));
ART_NUM nntp_next_art _((void));
int nntp_header _((ART_NUM));
void nntp_body _((ART_NUM));
long nntp_artsize _((void));
int nntp_finishbody _((int));
int nntp_seekart _((ART_POS));
ART_POS nntp_tellart _((void));
char* nntp_readart _((char*,int));
time_t nntp_time _((void));
int nntp_newgroups _((time_t));
int nntp_artnums _((void));
#if 0
int nntp_rover _((void));
#endif
ART_NUM nntp_find_real_art _((ART_NUM));
char* nntp_artname _((ART_NUM,bool_int));
char* nntp_tmpname _((int));
int nntp_handle_nested_lists _((void));
int nntp_handle_timeout _((void));
void nntp_server_died _((DATASRC*));
#ifdef SUPPORT_XTHREAD
long nntp_readcheck _((void));
long nntp_read _((char*,long));
#endif
