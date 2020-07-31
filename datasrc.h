/* datasrc.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


struct srcfile {
    FILE*	fp;		/* the file pointer to read the data */
    HASHTABLE*	hp;		/* the hash table for the data */
    LIST*	lp;		/* the list used to store the data */
    long	recent_cnt;	/* # lines/bytes this file might be */
#ifdef SUPPORT_NNTP
    time_t	lastfetch;	/* when the data was last fetched */
    time_t	refetch_secs;	/* how long before we refetch this file */
#endif
};

struct datasrc {
    char*	name;		/* our user-friendly name */
    char*	newsid;		/* the active file name or host name */
    SRCFILE	act_sf;		/* the active file's hashed contents */
    char*	grpdesc;	/* the newsgroup description file or tmp */
    SRCFILE	desc_sf;	/* the group description's hashed contents */
    char*	extra_name;	/* local active.times or server's actfile */
#ifdef SUPPORT_NNTP
    NNTPLINK	nntplink;
#endif
    char*	spool_dir;
    char*	over_dir;
    char*	over_fmt;
    char*	thread_dir;
    char*	auth_user;
    char*	auth_pass;
#ifdef USE_GENAUTH
    char*	auth_command;
#endif
    long	lastnewgrp;	/* time of last newgroup check */
    FILE*	ov_in;		/* the overview's file handle */
    time_t	ov_opened;	/* time overview file was opened */
    Uchar	fieldnum[OV_MAX_FIELDS];
    Uchar	fieldflags[OV_MAX_FIELDS];
    int		flags;
};

#define DF_TRY_OVERVIEW	0x0001
#define DF_TRY_THREAD	0x0002
#define DF_ADD_OK	0x0004
#define DF_DEFAULT	0x0008

#define DF_OPEN 	0x0010
#define DF_ACTIVE 	0x0020
#define DF_UNAVAILABLE 	0x0040
#ifdef SUPPORT_NNTP
#define DF_REMOTE	0x0080
#define DF_TMPACTFILE	0x0100
#define DF_TMPGRPDESC	0x0200
#define DF_USELISTACT	0x0400
#define DF_XHDR_BROKEN	0x0800
#define DF_NOXGTITLE	0x1000
#define DF_NOLISTGROUP	0x2000
#define DF_NOXREFS	0x4000
#endif

#define FF_HAS_FIELD	0x01
#define FF_CHECK4FIELD	0x02
#define FF_HAS_HDR	0x04
#define FF_CHECK4HDR	0x08
#define FF_FILTERSEND	0x10

#define DATASRC_NNTP_FLAGS(dp) (((dp) == datasrc? nntplink.flags : (dp)->nntplink.flags))

EXT LIST* datasrc_list;		/* a list of all DATASRCs */
EXT DATASRC* datasrc;		/* the current datasrc */
EXT int datasrc_cnt INIT(0);

#define datasrc_ptr(n)  ((DATASRC*)listnum2listitem(datasrc_list,(long)(n)))
#define datasrc_first() ((DATASRC*)listnum2listitem(datasrc_list,0L))
#define datasrc_next(p) ((DATASRC*)next_listitem(datasrc_list,(char*)(p)))

#define LENGTH_HACK 5	/* Don't bother comparing strings with lengths
			 * that differ by more than this. */
#define MAX_NG 9	/* Maximum number of groups to offer. */

#define DATASRC_ALARM_SECS   (5 * 60)

EXT char* trnaccess_mem INIT(NULL);

#ifdef SUPPORT_NNTP
EXT char* nntp_auth_file;
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void datasrc_init _((void));
char* read_datasrcs _((char*));
DATASRC* get_datasrc _((char*));
DATASRC* new_datasrc _((char*,char**));
bool open_datasrc _((DATASRC*));
void set_datasrc _((DATASRC*));
void check_datasrcs _((void));
void close_datasrc _((DATASRC*));
bool actfile_hash _((DATASRC*));
bool find_actgrp _((DATASRC*,char*,char*,int,ART_NUM));
char* find_grpdesc _((DATASRC*,char*));
int srcfile_open _((SRCFILE*,char*,char*,char*));
#ifdef SUPPORT_NNTP
char* srcfile_append _((SRCFILE*,char*,int));
void srcfile_end_append _((SRCFILE*,char*));
#endif
void srcfile_close _((SRCFILE*));
int find_close_match _((void));
