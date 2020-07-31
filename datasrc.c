/* datasrc.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "trn.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "term.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "opt.h"
#include "intrp.h"
#include "init.h"
#include "rcstuff.h"
#include "edit_dist.h"
#include "cache.h"
#include "last.h"
#include "rt-mt.h"
#include "rt-ov.h"
#include "rt-util.h"
#include "INTERN.h"
#include "datasrc.h"
#include "datasrc.ih"
#include "EXTERN.h"
#include "nntp.h"

void
datasrc_init()
{
    char** vals = prep_ini_words(datasrc_ini);
    char* machine = NULL;
    char* actname = NULL;
    char* s;

    datasrc_list = new_list(0,0,sizeof(DATASRC),20,LF_ZERO_MEM,NULL);

#ifdef SUPPORT_NNTP
    nntp_auth_file = savestr(filexp(NNTP_AUTH_FILE));

    machine = getenv("NNTPSERVER");
    if (machine && strNE(machine,"local")) {
	vals[DI_NNTP_SERVER] = machine;
	vals[DI_AUTH_USER] = read_auth_file(nntp_auth_file,
					    &vals[DI_AUTH_PASS]);
#ifdef USE_GENAUTH
	vals[DI_AUTH_COMMAND] = getenv("NNTPAUTH");
#endif
	vals[DI_FORCE_AUTH] = getenv("NNTP_FORCE_AUTH");
	new_datasrc("default",vals);
    }
#endif

    trnaccess_mem = read_datasrcs(TRNACCESS);
    s = read_datasrcs(DEFACCESS);
    if (!trnaccess_mem)
	trnaccess_mem = s;
    else if (s)
	free(s);

#ifdef SUPPORT_NNTP
    if (!machine) {
	machine = filexp(SERVER_NAME);
	if (FILE_REF(machine))
	    machine = nntp_servername(machine);
	if (strEQ(machine,"local")) {
	    machine = NULL;
	    actname = ACTIVE;
	}
#else
	actname = ACTIVE;
#endif
	prep_ini_words(datasrc_ini);	/* re-zero the values */

	vals[DI_NNTP_SERVER] = machine;
	vals[DI_ACTIVE_FILE] = actname;
	vals[DI_SPOOL_DIR] = NEWSSPOOL;
	vals[DI_THREAD_DIR] = THREAD_DIR;
	vals[DI_OVERVIEW_DIR] = OVERVIEW_DIR;
	vals[DI_OVERVIEW_FMT] = OVERVIEW_FMT;
	vals[DI_ACTIVE_TIMES] = ACTIVE_TIMES;
	vals[DI_GROUP_DESC] = GROUPDESC;
#ifdef SUPPORT_NNTP
	if (machine) {
	    vals[DI_AUTH_USER] = read_auth_file(nntp_auth_file,
						&vals[DI_AUTH_PASS]);
#ifdef USE_GENAUTH
	    vals[DI_AUTH_COMMAND] = getenv("NNTPAUTH");
#endif
	    vals[DI_FORCE_AUTH] = getenv("NNTP_FORCE_AUTH");
	}
#endif
	new_datasrc("default",vals);
#ifdef SUPPORT_NNTP
    }
#endif
    unprep_ini_words(datasrc_ini);
}

char*
read_datasrcs(filename)
char* filename;
{
    int fd;
    char* s;
    char* section;
    char* cond;
    char* filebuf = NULL;
    char** vals = INI_VALUES(datasrc_ini);

    if ((fd = open(filexp(filename),0)) >= 0) {
	fstat(fd,&filestat);
	if (filestat.st_size) {
	    int len;
	    filebuf = safemalloc((MEM_SIZE)filestat.st_size+2);
	    len = read(fd,filebuf,(int)filestat.st_size);
	    (filebuf)[len] = '\0';
	    prep_ini_data(filebuf,filename);
	    s = filebuf;
	    while ((s = next_ini_section(s,&section,&cond)) != NULL) {
		if (*cond && !check_ini_cond(cond))
		    continue;
		if (strncaseEQ(section, "group ", 6))
		    continue;
		s = parse_ini_section(s, datasrc_ini);
		if (!s)
		    break;
		new_datasrc(section,vals);
	    }
	}
	close(fd);
    }
    return filebuf;
}

DATASRC*
get_datasrc(name)
char* name;
{
    DATASRC* dp;
    for (dp = datasrc_first(); dp && dp->name; dp = datasrc_next(dp))
	if (strEQ(dp->name,name))
	    return dp;
    return NULL;
}

DATASRC*
new_datasrc(name,vals)
char* name;
char** vals;
{
    DATASRC* dp = datasrc_ptr(datasrc_cnt++);
    char* v;

    if (vals[DI_NNTP_SERVER]) {
#ifdef SUPPORT_NNTP
	dp->flags |= DF_REMOTE;
#else
	datasrc_cnt--;
	return NULL;
#endif
    }
    else if (!vals[DI_ACTIVE_FILE])
	return NULL; /*$$*/

    dp->name = savestr(name);
    if (strEQ(name,"default"))
	dp->flags |= DF_DEFAULT;

#ifdef SUPPORT_NNTP
    if ((v = vals[DI_NNTP_SERVER]) != NULL) {
	char* cp;
	dp->newsid = savestr(v);
	if ((cp = index(dp->newsid, ';')) != NULL) {
	    *cp = '\0';
	    dp->nntplink.port_number = atoi(cp+1);
	}

	if ((v = vals[DI_ACT_REFETCH]) != NULL && *v)
	    dp->act_sf.refetch_secs = text2secs(v,defRefetchSecs);
	else if (!vals[DI_ACTIVE_FILE])
	    dp->act_sf.refetch_secs = defRefetchSecs;
    }
    else
#endif /* SUPPORT_NNTP */
	dp->newsid = savestr(filexp(vals[DI_ACTIVE_FILE]));

    if (!(dp->spool_dir = file_or_none(vals[DI_SPOOL_DIR])))
	dp->spool_dir = savestr(tmpdir);

    dp->over_dir = dir_or_none(dp,vals[DI_OVERVIEW_DIR],DF_TRY_OVERVIEW);
    dp->over_fmt = file_or_none(vals[DI_OVERVIEW_FMT]);
    dp->thread_dir = dir_or_none(dp,vals[DI_THREAD_DIR],DF_TRY_THREAD);
    dp->grpdesc = dir_or_none(dp,vals[DI_GROUP_DESC],0);
    dp->extra_name = dir_or_none(dp,vals[DI_ACTIVE_TIMES],DF_ADD_OK);
#ifdef SUPPORT_NNTP
    if (dp->flags & DF_REMOTE) {
	/* FYI, we know extra_name to be NULL in this case. */
	if (vals[DI_ACTIVE_FILE]) {
	    dp->extra_name = savestr(filexp(vals[DI_ACTIVE_FILE]));
	    if (stat(dp->extra_name,&filestat) >= 0)
		dp->act_sf.lastfetch = filestat.st_mtime;
	}
	else {
	    dp->extra_name = temp_filename();
	    dp->flags |= DF_TMPACTFILE;
	    if (!dp->act_sf.refetch_secs)
		dp->act_sf.refetch_secs = 1;
	}

	if ((v = vals[DI_DESC_REFETCH]) != NULL && *v)
	    dp->desc_sf.refetch_secs = text2secs(v,defRefetchSecs);
	else if (!dp->grpdesc)
	    dp->desc_sf.refetch_secs = defRefetchSecs;
	if (dp->grpdesc) {
	    if (stat(dp->grpdesc,&filestat) >= 0)
		dp->desc_sf.lastfetch = filestat.st_mtime;
	}
	else {
	    dp->grpdesc = temp_filename();
	    dp->flags |= DF_TMPGRPDESC;
	    if (!dp->desc_sf.refetch_secs)
		dp->desc_sf.refetch_secs = 1;
	}
    }
    if ((v = vals[DI_FORCE_AUTH]) != NULL && (*v == 'y' || *v == 'Y'))
	dp->nntplink.flags |= NNTP_FORCE_AUTH_NEEDED;
    if ((v = vals[DI_AUTH_USER]) != NULL)
	dp->auth_user = savestr(v);
    if ((v = vals[DI_AUTH_PASS]) != NULL)
	dp->auth_pass = savestr(v);
#ifdef USE_GENAUTH
    if ((v = vals[DI_AUTH_COMMAND]) != NULL)
	dp->auth_command = savestr(v);
#endif
    if ((v = vals[DI_XHDR_BROKEN]) != NULL && (*v == 'y' || *v == 'Y'))
	dp->flags |= DF_XHDR_BROKEN;
    if ((v = vals[DI_XREFS]) != NULL && (*v == 'n' || *v == 'N'))
	dp->flags |= DF_NOXREFS;

#endif /* SUPPORT_NNTP */

    return dp;
}

static char*
dir_or_none(dp,dir,flag)
DATASRC* dp;
char* dir;
int flag;
{
    if (!dir || !*dir || strEQ(dir, "remote")) {
	dp->flags |= flag;
#ifdef SUPPORT_NNTP
	if (dp->flags & DF_REMOTE)
	    return NULL;
#endif
	if (flag == DF_ADD_OK) {
	    char* cp = safemalloc(strlen(dp->newsid)+6+1);
	    sprintf(cp,"%s.times",dp->newsid);
	    return cp;
	}
	if (flag == 0) {
	    char* cp = rindex(dp->newsid,'/');
	    int len;
	    if (!cp)
		return NULL;
	    len = cp - dp->newsid + 1;
	    cp = safemalloc(len+10+1);
	    strcpy(cp,dp->newsid);
	    strcpy(cp+len,"newsgroups");
	    return cp;
	}
	return dp->spool_dir;
    }

    if (strEQ(dir, "none"))
	return NULL;

    dp->flags |= flag;
    dir = filexp(dir);
    if (strEQ(dir,dp->spool_dir))
	return dp->spool_dir;
    return savestr(dir);
}

static char*
file_or_none(fn)
char* fn;
{
    if (!fn || !*fn || strEQ(fn, "none") || strEQ(fn, "remote"))
	return NULL;
    return savestr(filexp(fn));
}

bool
open_datasrc(dp)
DATASRC* dp;
{
    bool success;

    if (dp->flags & DF_UNAVAILABLE)
	return FALSE;
    set_datasrc(dp);
    if (dp->flags & DF_OPEN)
	return TRUE;
#ifdef SUPPORT_NNTP
    if (dp->flags & DF_REMOTE) {
	if (nntp_connect(dp->newsid,1) <= 0) {
	    dp->flags |= DF_UNAVAILABLE;
	    return FALSE;
	}
	nntp_allow_timeout = FALSE;
	dp->nntplink = nntplink;
	if (dp->act_sf.refetch_secs) {
	    switch (nntp_list("active", "control", 7)) {
	    case 1:
		if (strnNE(ser_line, "control ", 8)) {
		    strcpy(buf, ser_line);
		    dp->act_sf.lastfetch = 0;
		    success = actfile_hash(dp);
		    break;
		}
		if (nntp_gets(buf, sizeof buf - 1) > 0
		 && !nntp_at_list_end(buf)) {
		    nntp_finish_list();
		    success = actfile_hash(dp);
		    break;
		}
		/* FALL THROUGH */
	    case 0:
		dp->flags |= DF_USELISTACT;
		if (dp->flags & DF_TMPACTFILE) {
		    dp->flags &= ~DF_TMPACTFILE;
		    free(dp->extra_name);
		    dp->extra_name = NULL;
		    dp->act_sf.refetch_secs = 0;
		    success = srcfile_open(&dp->act_sf,(char*)NULL,
					   (char*)NULL,(char*)NULL);
		}
		else
		    success = actfile_hash(dp);
		break;
	    case -2:
		printf("Failed to open news server %s:\n%s\n", dp->newsid, ser_line);
		termdown(2);
		success = FALSE;
		break;
	    default:
		success = actfile_hash(dp);
		break;
	    }
	} else
	    success = actfile_hash(dp);
    }
    else
#endif
	success = actfile_hash(dp);
    if (success) {
	dp->flags |= DF_OPEN;
	if (dp->flags & DF_TRY_OVERVIEW)
	    ov_init();
	if (dp->flags & DF_TRY_THREAD)
	    mt_init();
    }
    else
	dp->flags |= DF_UNAVAILABLE;
#ifdef SUPPORT_NNTP
    if (dp->flags & DF_REMOTE)
	nntp_allow_timeout = TRUE;
#endif
    return success;
}

void
set_datasrc(dp)
DATASRC* dp;
{
#ifdef SUPPORT_NNTP
    if (datasrc)
	datasrc->nntplink = nntplink;
    if (dp)
	nntplink = dp->nntplink;
#endif
    datasrc = dp;
}

void
check_datasrcs()
{
#ifdef SUPPORT_NNTP
    DATASRC* dp;
    time_t now = time((time_t*)NULL);
    time_t limit;

    if (datasrc_list) {
	for (dp = datasrc_first(); dp && dp->name; dp = datasrc_next(dp)) {
	    if ((dp->flags & DF_OPEN) && dp->nntplink.rd_fp != NULL) {
		limit = ((dp->flags & DF_ACTIVE)? 30*60 : 10*60);
		if (now - dp->nntplink.last_command > limit) {
		    DATASRC* save_datasrc = datasrc;
		    /*printf("\n*** Closing %s ***\n", dp->name); $$*/
		    set_datasrc(dp);
		    nntp_close(TRUE);
		    dp->nntplink = nntplink;
		    set_datasrc(save_datasrc);
		}
	    }
	}
    }
#endif
}

void
close_datasrc(dp)
DATASRC* dp;
{
#ifdef SUPPORT_NNTP
    if (dp->flags & DF_REMOTE) {
	if (dp->flags & DF_TMPACTFILE)
	    UNLINK(dp->extra_name);
	else
	    srcfile_end_append(&dp->act_sf, dp->extra_name);
	if (dp->grpdesc) {
	    if (dp->flags & DF_TMPGRPDESC)
		UNLINK(dp->grpdesc);
	    else
		srcfile_end_append(&dp->desc_sf, dp->grpdesc);
	}
    }
#endif

    if (!(dp->flags & DF_OPEN))
	return;

#ifdef SUPPORT_NNTP
    if (dp->flags & DF_REMOTE) {
	DATASRC* save_datasrc = datasrc;
	set_datasrc(dp);
	nntp_close(TRUE);
	dp->nntplink = nntplink;
	set_datasrc(save_datasrc);
    }
#endif
    srcfile_close(&dp->act_sf);
    srcfile_close(&dp->desc_sf);
    dp->flags &= ~DF_OPEN;
    if (datasrc == dp)
	datasrc = NULL;
}

bool
actfile_hash(dp)
DATASRC* dp;
{
    int ret;
#ifdef SUPPORT_NNTP
    if (dp->flags & DF_REMOTE) {
	DATASRC* save_datasrc = datasrc;
	set_datasrc(dp);
	spin_todo = dp->act_sf.recent_cnt;
	ret = srcfile_open(&dp->act_sf, dp->extra_name, "active", dp->newsid);
	if (spin_count > 0)
	    dp->act_sf.recent_cnt = spin_count;
	set_datasrc(save_datasrc);
    }
    else
#endif
	ret = srcfile_open(&dp->act_sf, dp->newsid, (char*)NULL, (char*)NULL);
    return ret;
}

bool
find_actgrp(dp, outbuf, nam, len, high)
DATASRC* dp;
register char* outbuf;
register char* nam;
register int len;
ART_NUM high;
{
    HASHDATUM data;
    ACT_POS act_pos;
    FILE* fp = dp->act_sf.fp;
    char* lbp;
    int lbp_len;

    /* Do a quick, hashed lookup */

    outbuf[0] = '\0';
    data = hashfetch(dp->act_sf.hp, nam, len);
    if (data.dat_ptr) {
	LISTNODE* node = (LISTNODE*)data.dat_ptr;
	/*dp->act_sf.lp->recent = node;*/
	act_pos = node->low + data.dat_len;
	lbp = node->data + data.dat_len;
	lbp_len = index(lbp, '\n') - lbp + 1;
    }
    else {
	lbp = NULL;
	lbp_len = 0;
    }
#ifdef SUPPORT_NNTP
    if ((dp->flags & DF_USELISTACT)
     && (DATASRC_NNTP_FLAGS(dp) & NNTP_NEW_CMD_OK)) {
	DATASRC* save_datasrc = datasrc;
	set_datasrc(dp);
	switch (nntp_list("active", nam, len)) {
	case 0:
	    set_datasrc(save_datasrc);
	    return 0;
	case 1:
	    sprintf(outbuf, "%s\n", ser_line);
	    nntp_finish_list();
	    break;
	case -2:
	    /*$$$$*/
	    break;
	}
	set_datasrc(save_datasrc);
	if (!lbp_len) {
	    if (fp)
		(void) srcfile_append(&dp->act_sf, outbuf, len);
	    return 1;
	}
# ifndef ANCIENT_NEWS
	/* Safely update the low-water mark */
	{
	    char* f = rindex(outbuf, ' ');
	    char* t = lbp + lbp_len;
	    while (*--t != ' ') ;
	    while (t > lbp) {
		if (*--t == ' ')
		    break;
		if (f[-1] == ' ')
		    *t = '0';
		else
		    *t = *--f;
	    }
	}
# endif
	high = (ART_NUM)atol(outbuf+len+1);
    }
#endif

    if (lbp_len) {
#ifdef SUPPORT_NNTP
	if ((dp->flags & DF_REMOTE) && dp->act_sf.refetch_secs) {
	    int num;
	    char* cp;
	    if (high && high != (ART_NUM)atol(cp = lbp+len+1)) {
		while (isdigit(*cp)) cp++;
		while (*--cp != ' ') {
		    num = high % 10;
		    high = high / 10;
		    *cp = '0' + (char)num;
		}
		fseek(fp, act_pos, 0);
		fwrite(lbp, 1, lbp_len, fp);
	    }
	    goto use_cache;
	}
#endif

	/* hopefully this forces a reread */
	fseek(fp,2000000000L,1);

	/*$$ if line has changed length or is not there, we should
	 * discard/close the active file, and re-open it. $$*/
	if (fseek(fp, act_pos, 0) >= 0
	 && fgets(outbuf, LBUFLEN, fp) != NULL
	 && strnEQ(outbuf, nam, len) && outbuf[len] == ' ') {
	    /* Remember the latest info in our cache. */
	    (void) bcopy(outbuf, lbp, lbp_len);
	    return 1;
	}
      use_cache:
	/* Return our cached version */
	(void) bcopy(lbp, outbuf, lbp_len);
	outbuf[lbp_len] = '\0';
	return 1;
    }
    return 0;	/* no such group */
}

char*
find_grpdesc(dp, groupname)
DATASRC* dp;
char* groupname;
{
    HASHDATUM data;
    int grouplen;
    int ret;

    if (!dp->grpdesc)
	return nullstr;

    if (!dp->desc_sf.hp) {
#ifdef SUPPORT_NNTP
	if ((dp->flags & DF_REMOTE) && dp->desc_sf.refetch_secs) {
	    /*DATASRC* save_datasrc = datasrc;*/
	    set_datasrc(dp);
	    if ((dp->flags & (DF_TMPGRPDESC|DF_NOXGTITLE)) == DF_TMPGRPDESC
	     && netspeed < 5) {
		(void)srcfile_open(&dp->desc_sf,(char*)NULL,/*$$check return?*/
				   (char*)NULL,(char*)NULL);
		grouplen = strlen(groupname);
		goto try_xgtitle;
	    }
	    spin_todo = dp->desc_sf.recent_cnt;
	    ret = srcfile_open(&dp->desc_sf, dp->grpdesc,
			       "newsgroups", dp->newsid);
	    if (spin_count > 0)
		dp->desc_sf.recent_cnt = spin_count;
	    /*set_datasrc(save_datasrc);*/
	}
	else
#endif
	    ret = srcfile_open(&dp->desc_sf, dp->grpdesc,
			       (char*)NULL, (char*)NULL);
	if (!ret) {
#ifdef SUPPORT_NNTP
	    if (dp->flags & DF_TMPGRPDESC) {
		dp->flags &= ~DF_TMPGRPDESC;
		UNLINK(dp->grpdesc);
	    }
#endif
	    free(dp->grpdesc);
	    dp->grpdesc = NULL;
	    return nullstr;
	}
#ifdef SUPPORT_NNTP
	if (ret == 2 || !dp->desc_sf.refetch_secs)
	    dp->flags |= DF_NOXGTITLE;
#endif
    }

    grouplen = strlen(groupname);
    data = hashfetch(dp->desc_sf.hp, groupname, grouplen);
    if (data.dat_ptr) {
	LISTNODE* node = (LISTNODE*)data.dat_ptr;
	/*dp->act_sf.lp->recent = node;*/
	return node->data + data.dat_len + grouplen + 1;
    }

#ifdef SUPPORT_NNTP
  try_xgtitle:

    if ((dp->flags & (DF_REMOTE|DF_NOXGTITLE)) == DF_REMOTE) {
	set_datasrc(dp);
	sprintf(ser_line, "XGTITLE %s", groupname);
	if (nntp_command(ser_line) > 0 && nntp_check() > 0) {
	    nntp_gets(buf, sizeof buf - 1);
	    if (nntp_at_list_end(buf))
		sprintf(buf, "%s \n", groupname);
	    else {
		nntp_finish_list();
		strcat(buf, "\n");
	    }
	    groupname = srcfile_append(&dp->desc_sf, buf, grouplen);
	    return groupname+grouplen+1;
	}
	dp->flags |= DF_NOXGTITLE;
	if (dp->desc_sf.lp->high == -1) {
	    srcfile_close(&dp->desc_sf);
	    if (dp->flags & DF_TMPGRPDESC)
		return find_grpdesc(dp, groupname);
	    free(dp->grpdesc);
	    dp->grpdesc = NULL;
	}
    }
#endif
    return nullstr;
}

int
srcfile_open(sfp, filename, fetchcmd, server)
SRCFILE* sfp;
char* filename;
char* fetchcmd;
char* server;
{
    register unsigned offset;
    register char* s;
    HASHDATUM data;
    long node_low;
    int keylen, linelen;
    FILE* fp;
    char* lbp;
#ifdef SUPPORT_NNTP
    time_t now = time((time_t*)NULL);
    bool use_buffered_nntp_gets = 0;

    if (!filename)
	fp = NULL;
    else if (server) {
	if (!sfp->refetch_secs) {
	    server = NULL;
	    fp = fopen(filename, "r");
	    spin_todo = 0;
	}
	else if (now - sfp->lastfetch > sfp->refetch_secs
	      && (sfp->refetch_secs != 2 || !sfp->lastfetch)) {
	    fp = fopen(filename, "w+");
	    if (fp) {
		printf("Getting %s file from %s.", fetchcmd, server);
		fflush(stdout);
		/* tell server we want the file */
		if (!(nntplink.flags & NNTP_NEW_CMD_OK))
		    use_buffered_nntp_gets = 1;
		else if (nntp_list(fetchcmd, nullstr, 0) < 0) {
		    printf("\nCan't get %s file from server: \n%s\n",
			   fetchcmd, ser_line) FLUSH;
		    termdown(2);
		    fclose(fp);
		    return 0;
		}
		sfp->lastfetch = now;
		if (netspeed > 8)
		    spin_todo = 0;
	    }
	}
	else {
	    server = NULL;
	    fp = fopen(filename, "r+");
	    if (!fp) {
		sfp->refetch_secs = 0;
		fp = fopen(filename, "r");
	    }
	    spin_todo = 0;
	}
	if (sfp->refetch_secs & 3)
	    sfp->refetch_secs += 365L*24*60*60;
    }
    else
#endif
    {
	fp = fopen(filename, "r");
	spin_todo = 0;
    }

    if (filename && fp == NULL) {
	printf(cantopen, filename) FLUSH;
	termdown(1);
	return 0;
    }
    setspin(spin_todo > 0? SPIN_BARGRAPH : SPIN_FOREGROUND);

    srcfile_close(sfp);

    /* Create a list with one character per item using a large chunk size. */
    sfp->lp = new_list(0, 0, 1, SRCFILE_CHUNK_SIZE, 0, NULL);
    sfp->hp = hashcreate(3001, srcfile_cmp);
    sfp->fp = fp;

#ifdef SUPPORT_NNTP
    if (!filename) {
	(void) listnum2listitem(sfp->lp, 0);
	sfp->lp->high = -1;
	setspin(SPIN_OFF);
	return 1;
    }
#endif

    lbp = listnum2listitem(sfp->lp, 0);
    data.dat_ptr = (char*)sfp->lp->first;

    for (offset = 0, node_low = 0; ; offset += linelen, lbp += linelen) {
#ifdef SUPPORT_NNTP
	if (server) {
	    if (use_buffered_nntp_gets)
		use_buffered_nntp_gets = 0;
	    else if (nntp_gets(buf, sizeof buf - 1) < 0) {
		printf("\nError getting %s file.\n", fetchcmd) FLUSH;
		termdown(2);
		srcfile_close(sfp);
		setspin(SPIN_OFF);
		return 0;
	    }
	    if (nntp_at_list_end(buf))
		break;
	    strcat(buf,"\n");
	    fputs(buf, fp);
	    spin(200 * netspeed);
	}
#endif
	ElseIf (!fgets(buf, sizeof buf, fp))
	    break;

	for (s = buf; *s && !isspace(*s); s++) ;
	if (!*s) {
	    linelen = 0;
	    continue;
	}
	keylen = s - buf;
	if (*++s != '\n' && isspace(*s)) {
	    while (*++s != '\n' && isspace(*s)) ;
	    strcpy(buf+keylen+1, s);
	    s = buf+keylen+1;
	}
	for (s++; *s && *s != '\n'; s++) {
	    if (AT_GREY_SPACE(s))
		*s = ' ';
	}
	linelen = s - buf + 1;
	if (*s != '\n') {
	    if (linelen == sizeof buf) {
		linelen = 0;
		continue;
	    }
	    *s++ = '\n';
	    *s = '\0';
	}
	if (offset + linelen > SRCFILE_CHUNK_SIZE) {
	    LISTNODE* node = sfp->lp->recent;
	    node_low += offset;
	    node->high = node_low - 1;
	    node->data_high = node->data + offset - 1;
	    offset = 0;
	    lbp = listnum2listitem(sfp->lp, node_low);
	    data.dat_ptr = (char*)sfp->lp->recent;
	}
	data.dat_len = offset;
	(void) bcopy(buf, lbp, linelen);
	hashstore(sfp->hp, buf, keylen, data);
    }
    sfp->lp->high = node_low + offset - 1;
    setspin(SPIN_OFF);

#ifdef SUPPORT_NNTP
    if (server) {
	fflush(fp);
	if (ferror(fp)) {
	    printf("\nError writing the %s file %s.\n",fetchcmd,filename) FLUSH;
	    termdown(2);
	    srcfile_close(sfp);
	    return 0;
	}
	newline();
    }
#endif
    fseek(fp,0L,0);

    return server? 2 : 1;
}

#ifdef SUPPORT_NNTP
char*
srcfile_append(sfp, bp, keylen)
SRCFILE* sfp;
char* bp;
int keylen;
{
    LISTNODE* node;
    long pos;
    HASHDATUM data;
    char* s;
    char* lbp;
    int linelen;

    pos = sfp->lp->high + 1;
    lbp = listnum2listitem(sfp->lp, pos);
    node = sfp->lp->recent;
    data.dat_len = pos - node->low;

    s = bp + keylen + 1;
    if (sfp->fp && sfp->refetch_secs && *s != '\n') {
	fseek(sfp->fp, 0, 2);
	fputs(bp, sfp->fp);
    }

    if (*s != '\n' && isspace(*s)) {
	while (*++s != '\n' && isspace(*s)) ;
	strcpy(bp+keylen+1, s);
	s = bp+keylen+1;
    }
    for (s++; *s && *s != '\n'; s++) {
	if (AT_GREY_SPACE(s))
	    *s = ' ';
    }
    linelen = s - bp + 1;
    if (*s != '\n') {
	*s++ = '\n';
	*s = '\0';
    }
    if (data.dat_len + linelen > SRCFILE_CHUNK_SIZE) {
	node->high = pos - 1;
	node->data_high = node->data + data.dat_len - 1;
	lbp = listnum2listitem(sfp->lp, pos);
	node = sfp->lp->recent;
	data.dat_len = 0;
    }
    data.dat_ptr = (char*)node;
    (void) bcopy(bp, lbp, linelen);
    hashstore(sfp->hp, bp, keylen, data);
    sfp->lp->high = pos + linelen - 1;

    return lbp;
}
#endif /* SUPPORT_NNTP */

#ifdef SUPPORT_NNTP
void
srcfile_end_append(sfp, filename)
SRCFILE* sfp;
char* filename;
{
    if (sfp->fp && sfp->refetch_secs) {
	fflush(sfp->fp);

	if (sfp->lastfetch) {
	    struct utimbuf ut;
	    time(&ut.actime);
	    ut.modtime = sfp->lastfetch;
	    (void) utime(filename, &ut);
	}
    }
}
#endif /* SUPPORT_NNTP */

void
srcfile_close(sfp)
SRCFILE* sfp;
{
    if (sfp->fp) {
	fclose(sfp->fp);
	sfp->fp = NULL;
    }
    if (sfp->lp) {
	delete_list(sfp->lp);
	sfp->lp = NULL;
    }
    if (sfp->hp) {
	hashdestroy(sfp->hp);
	sfp->hp = NULL;
    }
}

static int
srcfile_cmp(key, keylen, data)
char* key;
int keylen;
HASHDATUM data;
{
    /* We already know that the lengths are equal, just compare the strings */
    return bcmp(key, ((LISTNODE*)data.dat_ptr)->data + data.dat_len, keylen);
}

#ifdef EDIT_DISTANCE

/* Edit Distance extension to trn
 *
 *	Mark Maimone (mwm@cmu.edu)
 *	Carnegie Mellon Computer Science
 *	9 May 1993
 *
 *	This code helps trn handle typos in newsgroup names much more
 *   gracefully.  Instead of "... does not exist!!", it will pick the
 *   nearest one, or offer you a choice if there are several options.
 */

/* find_close_match -- Finds the closest match for the string given in
 * global ngname.  If found, the result will be the corrected string
 * returned in that global.
 *
 * We compare the (presumably misspelled) newsgroup name with all legal
 * newsgroups, using the Edit Distance metric.  The edit distance between
 * two strings is the minimum number of simple operations required to
 * convert one string to another (the implementation here supports INSERT,
 * DELETE, CHANGE and SWAP).  This gives every legal newsgroup an integer
 * rank.
 *
 * You might want to present all of the closest matches, and let the user
 * choose among them.  But because I'm lazy I chose to only keep track of
 * all with newsgroups with the *single* smallest error, in array ngptrs[].
 * A more flexible approach would keep around the 10 best matches, whether
 * or not they had precisely the same edit distance, but oh well.
 */

static char** ngptrs;		/* List of potential matches */
static int ngn;			/* Length of list in ngptrs[] */
static int best_match;		/* Value of best match */

int
find_close_match()
{
    DATASRC* dp;
    int ret = 0;

    best_match = -1;
    ngptrs = (char**)safemalloc(MAX_NG * sizeof (char*));
    ngn = 0;

    /* Iterate over all legal newsgroups */
    for (dp = datasrc_first(); dp && dp->name; dp = datasrc_next(dp)) {
	if (dp->flags & DF_OPEN) {
	    if (dp->act_sf.hp)
		hashwalk(dp->act_sf.hp, check_distance, 0);
	    else
		ret = -1;
	}
    }

    if (ret < 0) {
	hashwalk(newsrc_hash, check_distance, 1);
	ret = 0;
    }

    /* ngn is the number of possibilities.  If there's just one, go with it. */

    switch (ngn) {
        case 0:
	    break;
	case 1: {
	    char* cp = index(ngptrs[0], ' ');
	    if (cp)
		*cp = '\0';
#ifdef VERBOSE
	    IF(verbose)
		printf("(I assume you meant %s)\n", ngptrs[0]) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("(Using %s)\n", ngptrs[0]) FLUSH;
#endif
	    set_ngname(ngptrs[0]);
	    if (cp)
		*cp = ' ';
	    ret = 1;
	    break;
	}
	default:
	    ret = get_near_miss();
	    break;
    }
    free((char*)ngptrs);
    return ret;
}

static int
check_distance(len, data, newsrc_ptr)
int len;
HASHDATUM* data;
int newsrc_ptr;
{
    int value;
    char* name;

    if (newsrc_ptr)
	name = ((NGDATA*)data->dat_ptr)->rcline;
    else
	name = ((LISTNODE*)data->dat_ptr)->data + data->dat_len;

    /* Efficiency: don't call edit_dist when the lengths are too different. */
    if (len < ngname_len) {
	if (ngname_len - len > LENGTH_HACK)
	    return 0;
    }
    else {
	if (len - ngname_len > LENGTH_HACK)
	    return 0;
    }

    value = edit_distn(ngname, ngname_len, name, len);
    if (value > MIN_DIST)
	return 0;

    if (value < best_match)
	ngn = 0;
    if (best_match < 0 || value <= best_match) {
	int i;
	for (i = 0; i < ngn; i++) {
	    if (strEQ(name, ngptrs[i]))
		return 0;
	}
	best_match = value;
	if (ngn < MAX_NG)
	    ngptrs[ngn++] = name;
    }
    return 0;
}

/* Now we've got several potential matches, and have to choose between them
** somehow.  Again, results will be returned in global ngname.
*/
static int
get_near_miss()
{
    char promptbuf[256];
    char options[MAX_NG+10];
    char* op = options;
    int i;

#ifdef VERBOSE
    IF(verbose)
	printf("However, here are some close matches:\n") FLUSH;
#endif
    if (ngn > 9)
	ngn = 9;	/* Since we're using single digits.... */
    for (i = 0; i < ngn; i++) {
	char* cp = index(ngptrs[i], ' ');
	if (cp)
	    *cp = '\0';
	printf("  %d.  %s\n", i+1, ngptrs[i]);
	sprintf(op++, "%d", i+1);	/* Expensive, but avoids ASCII deps */
	if (cp)
	    *cp = ' ';
    }
    *op++ = 'n';
    *op = '\0';

#ifdef VERBOSE
    IF(verbose)
	sprintf(promptbuf, "Which of these would you like?");
    ELSE
#endif
#ifdef TERSE
	sprintf(promptbuf, "Which?");
#endif
reask:
    in_char(promptbuf, 'A', options);
#ifdef VERIFY
    printcmd();
#endif
    putchar('\n') FLUSH;
    switch (*buf) {
        case 'n':
	case 'N':
	case 'q':
	case 'Q':
	case 'x':
	case 'X':
	    return 0;
	case 'h':
	case 'H':
#ifdef VERBOSE
	    IF(verbose)
		fputs("  You entered an illegal newsgroup name, and these are the nearest possible\n  matches.  If you want one of these, then enter its number.  Otherwise\n  just say 'n'.\n", stdout) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("Illegal newsgroup, enter a number or 'n'.\n", stdout) FLUSH;
#endif
	    goto reask;
	default:
	    if (isdigit(*buf)) {
		char* s = index(options, *buf);

		i = s ? (s - options) : ngn;
		if (i >= 0 && i < ngn) {
		    char* cp = index(ngptrs[i], ' ');
		    if (cp)
			*cp = '\0';
		    set_ngname(ngptrs[i]);
		    if (cp)
			*cp = ' ';
		    return 1;
		}
	    }
	    fputs(hforhelp, stdout) FLUSH;
	    break;
    }

    settle_down();
    goto reask;
}

#endif /* EDIT_DISTANCE */
