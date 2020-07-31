/* nntpclient.h
*/ 
/* This software is copyrighted as detailed in the LICENSE file. */


#ifdef SUPPORT_NNTP

struct nntplink {
    FILE*	rd_fp;
    FILE*	wr_fp;
    time_t	last_command;
    int		port_number;
    int		flags;
#ifdef USE_GENAUTH
    int		cookiefd;
#endif
    bool	trailing_CR;
};

#define NNTP_NEW_CMD_OK		0x0001
#define NNTP_FORCE_AUTH_NEEDED	0x0002
#define NNTP_FORCE_AUTH_NOW	0x0004

EXT NNTPLINK nntplink;		/* the current server's file handles */
EXT bool nntp_allow_timeout INIT(FALSE);

#define nntp_get_a_line(buf,len,realloc) get_a_line(buf,len,realloc,nntplink.rd_fp)

/* RFC 977 defines these, so don't change them */

#define	NNTP_CLASS_INF  	'1'
#define NNTP_CLASS_OK   	'2'
#define	NNTP_CLASS_CONT 	'3'
#define	NNTP_CLASS_ERR  	'4'
#define	NNTP_CLASS_FATAL	'5'

#define	NNTP_POSTOK_VAL 	200	/* Hello -- you can post */
#define	NNTP_NOPOSTOK_VAL	201	/* Hello -- you can't post */
#define NNTP_LIST_FOLLOWS_VAL	215	/* There's a list a-comin' next */

#define NNTP_GOODBYE_VAL	400	/* Have to hang up for some reason */
#define	NNTP_NOSUCHGROUP_VAL	411	/* No such newsgroup */
#define NNTP_NONEXT_VAL		421	/* No next article */
#define NNTP_NOPREV_VAL		422	/* No previous article */
#define	NNTP_POSTFAIL_VAL	441	/* Posting failed */

#define	NNTP_AUTH_NEEDED_VAL 	480	/* Authorization Failed */
#define	NNTP_AUTH_REJECT_VAL	482	/* Authorization data rejected */

#define	NNTP_BAD_COMMAND_VAL	500	/* Command not recognized */
#define	NNTP_SYNTAX_VAL		501	/* Command syntax error */
#define	NNTP_ACCESS_VAL 	502	/* Access to server denied */
#define	NNTP_TMPERR_VAL  	503	/* Program fault, command not performed */
#define	NNTP_AUTH_BAD_VAL 	580	/* Authorization Failed */

#define	NNTP_STRLEN	512

EXT char ser_line[NNTP_STRLEN];

EXT char last_command[NNTP_STRLEN];

#endif /* SUPPORT_NNTP */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int nntp_connect _((char*,bool_int));
char* nntp_servername _((char*));
int nntp_command _((char*));
int nntp_check _((void));
bool nntp_at_list_end _((char*));
int nntp_gets _((char*,int));
void nntp_close _((bool_int));
