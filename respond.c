/* respond.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "intrp.h"
#include "hash.h"
#include "cache.h"
#include "head.h"
#include "term.h"
#include "mime.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "ng.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "trn.h"
#include "art.h"
#include "artio.h"
#include "search.h"
#include "artstate.h"
#include "final.h"
#include "decode.h"
#include "uudecode.h"
#include "charsubst.h"
#include "INTERN.h"
#include "respond.h"
#include "respond.ih"

static char nullart[] = "\nEmpty article.\n";

void
respond_init()
{
}

int
save_article()
{
    bool_int use_pref;
    register char* s;
    register char* c;
    char altbuf[CBUFLEN];
    int i;
    bool interactive = (buf[1] == FINISHCMD);
    char cmd = *buf;
    
    if (!finish_command(interactive))	/* get rest of command */
	return SAVE_ABORT;
    if ((use_pref = isupper(cmd)) != 0)
	cmd = tolower(cmd);
    parseheader(art);
    mime_SetArticle();
    clear_artbuf();
    savefrom = (cmd == 'w' || cmd == 'e')? htype[PAST_HEADER].minpos : 0;
    if (artopen(art,savefrom) == NULL) {
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nCan't save an empty article.\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs(nullart,stdout) FLUSH;
#endif
	termdown(2);
	return SAVE_DONE;
    }
    if (chdir(cwd)) {
	printf(nocd,cwd) FLUSH;
	sig_catcher(0);
    }
    if (cmd == 'e') {		/* is this an extract command? */
	static bool custom_extract = FALSE;
	char* cmdstr;
	int partOpt = 0, totalOpt = 0;

	s = buf+1;		/* skip e */
	while (*s == ' ') s++;	/* skip leading spaces */
	if (*s == '-' && isdigit(s[1])) {
	    partOpt = atoi(s+1);
	    do s++; while (isdigit(*s));
	    if (*s == '/') {
		totalOpt = atoi(s+1);
		do s++; while (isdigit(*s));
		while (*s == ' ') s++;
	    }
	    else
		totalOpt = partOpt;
	}
	safecpy(altbuf,filexp(s),sizeof altbuf);
	s = altbuf;
	if (*s) {
	    cmdstr = cpytill(buf,s,'|');	/* check for | */
	    s = buf + strlen(buf)-1;
	    while (*s == ' ') s--;		/* trim trailing spaces */
	    *++s = '\0';
	    if (*cmdstr) {
		s = cmdstr+1;			/* skip | */
		while (*s == ' ') s++;
		if (*s)	{			/* if new command, use it */
		    safefree(extractprog);
		    extractprog = savestr(s);	/* put extracter in %e */
		}
		else
		    cmdstr = extractprog;
	    }
	    else
		cmdstr = NULL;
	    s = buf;
	}
	else {
	    if (extractdest)
		strcpy(s, extractdest);
	    if (custom_extract)
		cmdstr = extractprog;
	    else
		cmdstr = NULL;
	}
	custom_extract = (cmdstr != 0);

	if (FILE_REF(s) != '/') {	/* relative path? */
	    c = (s==buf ? altbuf : buf);
	    interp(c, (sizeof buf), getval("SAVEDIR",SAVEDIR));
	    if (makedir(c,MD_DIR))	/* ensure directory exists */
		strcpy(c,cwd);
	    if (*s) {
		while (*c) c++;
		*c++ = '/';
		strcpy(c,s);		/* add filename */
	    }
	    s = (s==buf ? altbuf : buf);
	}
	if (FILE_REF(s) != '/') {	/* path still relative? */
	    c = (s==buf ? altbuf : buf);
	    sprintf(c, "%s/%s", cwd, s);
	    s = c;			/* absolutize it */
	}
	safefree(extractdest);
	s = extractdest = savestr(s); /* make it handy for %E */
	if (makedir(s, MD_DIR)) {	/* ensure directory exists */
	    int_count++;
	    return SAVE_DONE;
	}
	if (chdir(s)) {
	    printf(nocd,s) FLUSH;
	    sig_catcher(0);
	}
	c = trn_getwd(buf, sizeof(buf));	/* simplify path for output */
	if (custom_extract) {
	    printf("Extracting article into %s using %s\n",c,extractprog) FLUSH;
	    termdown(1);
	    interp(cmd_buf, sizeof cmd_buf, getval("CUSTOMSAVER",CUSTOMSAVER));
	    invoke(cmd_buf, (char*)NULL);
	}
	else if (is_mime) {
	    printf("Extracting MIME article into %s:\n", c) FLUSH;
	    termdown(1);
	    mime_DecodeArticle(FALSE);
	}
	else {
	    char* filename;
	    int part, total;
	    int decode_type = 0;
	    int cnt = 0;

	    /* Scan subject for filename and part number information */
	    filename = decode_subject(art, &part, &total);
	    if (partOpt)
		part = partOpt;
	    if (totalOpt)
		total = totalOpt;
	    for (artpos = savefrom;
		 readart(art_line,sizeof art_line) != NULL;
		 artpos = tellart())
	    {
		if (*art_line <= ' ')
		    continue;	/* Ignore empty or initially-whitespace lines */
		if (((*art_line == '#' || *art_line == ':')
		  && (strnEQ(art_line+1, "! /bin/sh", 9)
		   || strnEQ(art_line+1, "!/bin/sh", 8)
		   || strnEQ(art_line+2, "This is ", 8)))
#if 0
		 || strnEQ(art_line, "sed ", 4)
		 || strnEQ(art_line, "cat ", 4)
		 || strnEQ(art_line, "echo ", 5)
#endif
		) {
		    savefrom = artpos;
		    decode_type = 1;
		    break;
		}
		else if (uue_prescan(art_line,&filename,&part,&total)) {
		    savefrom = artpos;
		    seekart(savefrom);
		    decode_type = 2;
		    break;
		}
		else if (++cnt == 300)
		    break;
	    }/* for */
	    switch (decode_type) {
	      case 1:
		printf("Extracting shar into %s:\n", c) FLUSH;
		termdown(1);
		interp(cmd_buf,(sizeof cmd_buf),getval("SHARSAVER",SHARSAVER));
		invoke(cmd_buf, (char*)NULL);
		break;
	      case 2:
		printf("Extracting uuencoded file into %s:\n", c) FLUSH;
		termdown(1);
		mime_section->type = IMAGE_MIME;
		safefree(mime_section->filename);
		mime_section->filename = filename? savestr(filename) : NULL;
		mime_section->encoding = MENCODE_UUE;
		mime_section->part = part;
		mime_section->total = total;
		if (!decode_piece((MIMECAP_ENTRY*)NULL,(char*)NULL) && *msg) {
		    newline();
		    fputs(msg,stdout);
		}
		newline();
		break;
	      default:
		printf("Unable to determine type of file.\n") FLUSH;
		termdown(1);
		break;
	    }
	}/* if */
    }
    else if ((s = index(buf,'|')) != NULL) { /* is it a pipe command? */
	s++;			/* skip the | */
	while (*s == ' ') s++;
	safecpy(altbuf,filexp(s),sizeof altbuf);
	safefree(savedest);
	savedest = savestr(altbuf);
#ifdef SUPPORT_NNTP
	if (datasrc->flags & DF_REMOTE)
	    nntp_finishbody(FB_SILENT);
#endif
	interp(cmd_buf, (sizeof cmd_buf), getval("PIPESAVER",PIPESAVER));
				/* then set up for command */
	termlib_reset();
	resetty();		/* restore tty state */
	if (use_pref)		/* use preferred shell? */
	    doshell((char*)NULL,cmd_buf);
				/* do command with it */
	else
	    doshell(sh,cmd_buf);	/* do command with sh */
	noecho();		/* and stop echoing */
	crmode();		/* and start cbreaking */
	termlib_init();
    }
    else {			/* normal save */
	bool there, mailbox;
	char* savename = getval("SAVENAME",SAVENAME);

	s = buf+1;		/* skip s or S */
	if (*s == '-') {	/* if they are confused, skip - also */
#ifdef VERBOSE
	    IF(verbose)
		fputs("Warning: '-' ignored.  This isn't readnews.\n",stdout)
		  FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("'-' ignored.\n",stdout) FLUSH;
#endif
	    termdown(1);
	    s++;
	}
	for (; *s == ' '; s++);	/* skip spaces */
	safecpy(altbuf,filexp(s),sizeof altbuf);
	s = altbuf;
	if (!FILE_REF(s)) {
	    interp(buf, (sizeof buf), getval("SAVEDIR",SAVEDIR));
	    if (makedir(buf,MD_DIR))	/* ensure directory exists */
		strcpy(buf,cwd);
	    if (*s) {
		for (c = buf; *c; c++) ;
		*c++ = '/';
		strcpy(c,s);		/* add filename */
	    }
	    s = buf;
	}
	for (i = 0;
	    (there = stat(s,&filestat) >= 0) && S_ISDIR(filestat.st_mode);
	    i++) {			/* is it a directory? */

	    c = (s+strlen(s));
	    *c++ = '/';			/* put a slash before filename */
	    interp(c, s==buf?(sizeof buf):(sizeof altbuf),
		i ? "News" : savename );
				/* generate a default name somehow or other */
	}
	makedir(s,MD_FILE);
	if (FILE_REF(s) != '/') {	/* relative path? */
	    c = (s==buf ? altbuf : buf);
	    sprintf(c, "%s/%s", cwd, s);
	    s = c;			/* absolutize it */
	}
	safefree(savedest);
	s = savedest = savestr(s);	/* doesn't move any more */
					/* make it handy for %b */
	tmpfp = NULL;
	if (!there) {
	    if (mbox_always)
		mailbox = TRUE;
	    else if (norm_always)
		mailbox = FALSE;
	    else {
		char* dflt = (instr(savename,"%a", TRUE) ? "nyq" : "ynq");
		
		sprintf(cmd_buf,
		"\nFile %s doesn't exist--\n	use mailbox format?",s);
	      reask_save:
		in_char(cmd_buf, 'M', dflt);
		newline();
#ifdef VERIFY
		printcmd();
#endif
		if (*buf == 'h') {
#ifdef VERBOSE
		    IF(verbose)
			printf("\n\
Type y to create %s as a mailbox.\n\
Type n to create it as a normal file.\n\
Type q to abort the save.\n\
",s) FLUSH;
		    ELSE
#endif
#ifdef TERSE
			fputs("\n\
y to create mailbox.\n\
n to create normal file.\n\
q to abort.\n\
",stdout) FLUSH;
#endif
		    termdown(4);
		    goto reask_save;
		}
		else if (*buf == 'n') {
		    mailbox = FALSE;
		}
		else if (*buf == 'y') {
		    mailbox = TRUE;
		}
		else if (*buf == 'q') {
		    goto s_bomb;
		}
		else {
		    fputs(hforhelp,stdout) FLUSH;
		    termdown(1);
		    settle_down();
		    goto reask_save;
		}
	    }
	}
	else if (S_ISCHR(filestat.st_mode))
	    mailbox = FALSE;
	else {
	    tmpfp = fopen(s,"r+");
	    if (!tmpfp)
		mailbox = FALSE;
	    else {
		if (fread(buf,1,LBUFLEN,tmpfp)) {
		    c = buf;
		    if (!isspace(MBOXCHAR))   /* if non-zero, */
			while (isspace(*c))   /* check the first character */
			    c++;
		    mailbox = (*c == MBOXCHAR);
		} else
		    mailbox = mbox_always;    /* if zero length, recheck -M */
	    }
	}

	s = getenv(mailbox ? "MBOXSAVER" : "NORMSAVER");
	if (s) {
	    if (tmpfp)
		fclose(tmpfp);
	    safecpy(cmd_buf, filexp(s), sizeof cmd_buf);
#ifdef SUPPORT_NNTP
	    if (datasrc->flags & DF_REMOTE)
		nntp_finishbody(FB_SILENT);
#endif
	    termlib_reset();
	    resetty();		/* make terminal behave */
	    i = doshell(use_pref?(char*)NULL:SH,cmd_buf);
	    termlib_init();
	    noecho();		/* make terminal do what we want */
	    crmode();
	}
	else if (tmpfp != NULL || (tmpfp = fopen(savedest, "a")) != NULL) {
	    bool quote_From = FALSE;
	    fseek(tmpfp,0,2);
	    if (mailbox) {
#if MBOXCHAR == '\001'
		fprintf(tmpfp,"\001\001\001\001\n");
#else
		interp(cmd_buf, sizeof cmd_buf, "From %t %`LANG= date`\n");
		fputs(cmd_buf, tmpfp);
		quote_From = TRUE;
#endif
	    }
	    if (savefrom == 0 && art != 0)
		fprintf(tmpfp,"Article: %ld of %s\n", (long)art, ngname);
	    seekart(savefrom);
	    while (readart(buf,LBUFLEN) != NULL) {
		if (quote_From && strncaseEQ(buf,"from ",5))
		    putc('>', tmpfp);
		fputs(buf, tmpfp);
	    }
	    fputs("\n\n", tmpfp);
#if MBOXCHAR == '\001'
	    if (mailbox)
		fprintf(tmpfp,"\001\001\001\001\n");
#endif
	    fclose(tmpfp);
	    i = 0; /*$$ set non-zero on write error */
	}
	else
	    i = 1;
	if (i)
	    fputs("Not saved",stdout);
	else {
	    printf("%s to %s %s", there? "Appended" : "Saved",
		   mailbox? "mailbox" : "file", savedest);
	}
	if (interactive)
	    newline();
    }
s_bomb:
    chdir_newsdir();
    return SAVE_DONE;
}

int
view_article()
{
    parseheader(art);
    mime_SetArticle();
    clear_artbuf();
    savefrom = htype[PAST_HEADER].minpos;
    if (artopen(art,savefrom) == NULL) {
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nNo attatchments on an empty article.\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs(nullart,stdout) FLUSH;
#endif
	termdown(2);
	return SAVE_DONE;
    }
    printf("Processing attachments...\n") FLUSH;
    termdown(1);
    if (is_mime)
	mime_DecodeArticle(TRUE);
    else {
	char* filename;
	int part, total;
	int cnt = 0;

	/* Scan subject for filename and part number information */
	filename = decode_subject(art, &part, &total);
	for (artpos = savefrom;
	     readart(art_line,sizeof art_line) != NULL;
	     artpos = tellart())
	{
	    if (*art_line <= ' ')
		continue;	/* Ignore empty or initially-whitespace lines */
	    if (uue_prescan(art_line, &filename, &part, &total)) {
		MIMECAP_ENTRY* mc = mime_FindMimecapEntry("image/jpeg",0); /*$$ refine this */
		savefrom = artpos;
		seekart(savefrom);
		mime_section->type = UNHANDLED_MIME;
		safefree(mime_section->filename);
		mime_section->filename = filename? savestr(filename) : NULL;
		mime_section->encoding = MENCODE_UUE;
		mime_section->part = part;
		mime_section->total = total;
		if (mc && !decode_piece(mc,(char*)NULL) && *msg) {
		    newline();
		    fputs(msg,stdout);
		}
		newline();
		cnt = 0;
	    }
	    else if (++cnt == 300)
		break;
	}/* for */
	if (cnt) {
	    printf("Unable to determine type of file.\n") FLUSH;
	    termdown(1);
	}
    }
    chdir_newsdir();
    return SAVE_DONE;
}

int
cancel_article()
{
    char hbuf[5*LBUFLEN];
    char* ngs_buf;
    char* from_buf;
    char* reply_buf;
    int myuid = getuid();
    int r = -1;

    if (artopen(art,(ART_POS)0) == NULL) {
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nCan't cancel an empty article.\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs(nullart,stdout) FLUSH;
#endif
	termdown(2);
	return r;
    }
    reply_buf = fetchlines(art,REPLY_LINE);
    from_buf = fetchlines(art,FROM_LINE);
    ngs_buf = fetchlines(art,NGS_LINE);
    if (strcaseNE(getval("FROM",""),from_buf)
     && (!instr(from_buf,hostname,FALSE)
      || (!instr(from_buf,loginName,TRUE)
       && !instr(reply_buf,loginName,TRUE)
#ifdef NEWS_ADMIN
       && myuid != newsuid
#endif
       && myuid != ROOTID))) {
#ifdef DEBUG
	if (debug) {
	    printf("\n%s@%s != %s\n",loginName,hostname,from_buf) FLUSH;
	    printf("%s != %s\n",getval("FROM",""),from_buf) FLUSH;
	    termdown(3);
	}
#endif
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nYou can't cancel someone else's article\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs("\nNot your article\n",stdout) FLUSH;
#endif
	termdown(2);
    }
    else {
	tmpfp = fopen(headname,"w");	/* open header file */
	if (tmpfp == NULL) {
	    printf(cantcreate,headname) FLUSH;
	    termdown(1);
	    goto done;
	}
	interp(hbuf, sizeof hbuf, getval("CANCELHEADER",CANCELHEADER));
	fputs(hbuf,tmpfp);
	fclose(tmpfp);
	fputs("\nCanceling...\n",stdout) FLUSH;
	termdown(2);
	export_nntp_fds = TRUE;
	r = doshell(sh,filexp(getval("CANCEL",CALL_INEWS)));
	export_nntp_fds = FALSE;
    }
done:
    free(ngs_buf);
    free(from_buf);
    free(reply_buf);
    return r;
}

int
supersede_article()		/* Supersedes: */
{
    char hbuf[5*LBUFLEN];
    char* ngs_buf;
    char* from_buf;
    char* reply_buf;
    int myuid = getuid();
    int r = -1;
    bool incl_body = (*buf == 'Z');

    if (artopen(art,(ART_POS)0) == NULL) {
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nCan't supersede an empty article.\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs(nullart,stdout) FLUSH;
#endif
	termdown(2);
	return r;
    }
    reply_buf = fetchlines(art,REPLY_LINE);
    from_buf = fetchlines(art,FROM_LINE);
    ngs_buf = fetchlines(art,NGS_LINE);
    if (strcaseNE(getval("FROM",""),from_buf)
     && (!instr(from_buf,hostname,FALSE)
      || (!instr(from_buf,loginName,TRUE)
       && !instr(reply_buf,loginName,TRUE)
#ifdef NEWS_ADMIN
       && myuid != newsuid
#endif
       && myuid != ROOTID))) {
#ifdef DEBUG
	if (debug) {
	    printf("\n%s@%s != %s\n",loginName,hostname,from_buf) FLUSH;
	    printf("%s != %s\n",getval("FROM",""),from_buf) FLUSH;
	    termdown(3);
	}
#endif
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nYou can't supersede someone else's article\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs("\nNot your article\n",stdout) FLUSH;
#endif
	termdown(2);
    }
    else {
	tmpfp = fopen(headname,"w");	/* open header file */
	if (tmpfp == NULL) {
	    printf(cantcreate,headname) FLUSH;
	    termdown(1);
	    goto done;
	}
	interp(hbuf, sizeof hbuf, getval("SUPERSEDEHEADER",SUPERSEDEHEADER));
	fputs(hbuf,tmpfp);
	if (incl_body && artfp != NULL) {
	    parseheader(art);
	    seekart(htype[PAST_HEADER].minpos);
	    while (readart(buf,LBUFLEN) != NULL)
		fputs(buf,tmpfp);
	}
	fclose(tmpfp);
	follow_it_up();
	r = 0;
    }
done:
    free(ngs_buf);
    free(from_buf);
    free(reply_buf);
    return r;
}

static void
follow_it_up()
{
    safecpy(cmd_buf,filexp(getval("NEWSPOSTER",NEWSPOSTER)), sizeof cmd_buf);
    if (invoke(cmd_buf,origdir) == 42) {
	int ret;
#ifdef SUPPORT_NNTP
	if ((datasrc->flags & DF_REMOTE)
	 && (nntp_command("DATE") <= 0
	  || (nntp_check() < 0 && atoi(ser_line) != NNTP_BAD_COMMAND_VAL)))
	    ret = 1;
	else
#endif
	{
	    export_nntp_fds = TRUE;
	    ret = invoke(filexp(CALL_INEWS),origdir);
	    export_nntp_fds = FALSE;
	}
	if (ret) {
	    int appended = 0;
	    char* deadart = filexp("%./dead.article");
	    FILE* fp_in;
	    FILE* fp_out;
	    if ((fp_out = fopen(deadart, "a")) != NULL) {
		if ((fp_in = fopen(headname, "r")) != NULL) {
		    while (fgets(cmd_buf, sizeof cmd_buf, fp_in))
			fputs(cmd_buf, fp_out);
		    fclose(fp_in);
		    appended = 1;
		}
		fclose(fp_out);
	    }
	    if (appended)
		printf("Article appended to %s\n", deadart) FLUSH;
	    else
		printf("Unable to append article to %s\n", deadart) FLUSH;
	}
    }
}

void
reply()
{
    char hbuf[5*LBUFLEN];
    bool incl_body = (*buf == 'R' && art);
    char* maildoer = savestr(getval("MAILPOSTER",MAILPOSTER));

    artopen(art,(ART_POS)0);
    tmpfp = fopen(headname,"w");	/* open header file */
    if (tmpfp == NULL) {
	printf(cantcreate,headname) FLUSH;
	termdown(1);
	goto done;
    }
    interp(hbuf, sizeof hbuf, getval("MAILHEADER",MAILHEADER));
    fputs(hbuf,tmpfp);
    if (!instr(maildoer,"%h",TRUE)) {
#ifdef VERBOSE
	IF(verbose)
	    printf("\n%s\n(Above lines saved in file %s)\n",buf,headname)
	      FLUSH;
	ELSE
#endif
#ifdef TERSE
	    printf("\n%s\n(Header in %s)\n",buf,headname) FLUSH;
#endif
	termdown(3);
    }
    if (incl_body && artfp != NULL) {
	char* s;
	char* t;
	interp(buf, (sizeof buf), getval("YOUSAID",YOUSAID));
	fprintf(tmpfp,"%s\n",buf);
	parseheader(art);
	mime_SetArticle();
	clear_artbuf();
	seekart(htype[PAST_HEADER].minpos);
	wrapped_nl = '\n';
	while ((s = readartbuf(FALSE)) != NULL) {
	    if ((t = index(s, '\n')) != NULL)
		*t = '\0';
#ifdef CHARSUBST
	    strcharsubst(hbuf,s,sizeof hbuf,*charsubst);
	    fprintf(tmpfp,"%s%s\n",indstr,hbuf);
#else
	    fprintf(tmpfp,"%s%s\n",indstr,s);
#endif
	    if (t)
		*t = '\0';
	}
	fprintf(tmpfp,"\n");
	wrapped_nl = WRAPPED_NL;
    }
    fclose(tmpfp);
    safecpy(cmd_buf,filexp(maildoer),sizeof cmd_buf);
    invoke(cmd_buf,origdir);
done:
    free(maildoer);
}
  
void
forward()
{
    char hbuf[5*LBUFLEN];
    char* maildoer = savestr(getval("FORWARDPOSTER",FORWARDPOSTER));
#ifdef REGEX_WORKS_RIGHT
    COMPEX mime_compex;
#else
    char* s;
    char* eol;
#endif
    char* mime_boundary;

#ifdef REGEX_WORKS_RIGHT
    init_compex(&mime_compex);
#endif
    artopen(art,(ART_POS)0);
    tmpfp = fopen(headname,"w");	/* open header file */
    if (tmpfp == NULL) {
	printf(cantcreate,headname) FLUSH;
	termdown(1);
	goto done;
    }
    interp(hbuf, sizeof hbuf, getval("FORWARDHEADER",FORWARDHEADER));
    fputs(hbuf,tmpfp);
#ifdef REGEX_WORKS_RIGHT
    if (!compile(&mime_compex,"Content-Type: multipart/.*; *boundary=\"\\([^\"]*\\)\"",TRUE,TRUE)
     && execute(&mime_compex,hbuf) != NULL)
	mime_boundary = getbracket(&mime_compex,1);
    else
	mime_boundary = NULL;
#else
    mime_boundary = NULL;
    for (s = hbuf; s; s = eol) {
	eol = index(s, '\n');
	if (eol)
	    eol++;
	if (*s == 'C' && strncaseEQ(s, "Content-Type: multipart/", 24)) {
	    s += 24;
	    for (;;) {
		for ( ; *s && *s != ';'; s++) {
		    if (*s == '\n' && !isspace(s[1]))
			break;
		}
		if (*s != ';')
		    break;
		while (*++s == ' ') ;
		if (*s == 'b' && strncaseEQ(s, "boundary=\"", 10)) {
		    mime_boundary = s+10;
		    if ((s = index(mime_boundary, '"')) != NULL)
			*s = '\0';
		    mime_boundary = savestr(mime_boundary);
		    if (s)
			*s = '"';
		    break;
		}
	    }
	}
    }
#endif
    if (!instr(maildoer,"%h",TRUE)) {
#ifdef VERBOSE
	IF(verbose)
	    printf("\n%s\n(Above lines saved in file %s)\n",hbuf,headname)
	      FLUSH;
	ELSE
#endif
#ifdef TERSE
	    printf("\n%s\n(Header in %s)\n",hbuf,headname) FLUSH;
#endif
	termdown(3);
    }
    if (artfp != NULL) {
	interp(buf, sizeof buf, getval("FORWARDMSG",FORWARDMSG));
	if (mime_boundary) {
	    if (*buf && strncaseNE(buf, "Content-", 8))
		strcpy(buf, "Content-Type: text/plain\n");
	    fprintf(tmpfp,"--%s\n%s\n[Replace this with your comments.]\n\n--%s\nContent-Type: message/rfc822\n\n",
		    mime_boundary,buf,mime_boundary);
	}
	else if (*buf)
	    fprintf(tmpfp,"%s\n",buf);
	parseheader(art);
	seekart((ART_POS)0);
	while (readart(buf,sizeof buf) != NULL) {
	    if (!mime_boundary && *buf == '-') {
		putchar('-');
		putchar(' ');
	    }
	    fprintf(tmpfp,"%s",buf);
	}
	if (mime_boundary)
	    fprintf(tmpfp,"\n--%s--\n",mime_boundary);
	else {
	    interp(buf, (sizeof buf), getval("FORWARDMSGEND",FORWARDMSGEND));
	    if (*buf)
		fprintf(tmpfp,"%s\n",buf);
	}
    }
    fclose(tmpfp);
    safecpy(cmd_buf,filexp(maildoer),sizeof cmd_buf);
    invoke(cmd_buf,origdir);
  done:
    free(maildoer);
#ifdef REGEX_WORKS_RIGHT
    free_compex(&mime_compex);
#else
    safefree(mime_boundary);
#endif
}

void
followup()
{
    char hbuf[5*LBUFLEN];
    bool incl_body = (*buf == 'F' && art);
    ART_NUM oldart = art;

    if (!incl_body && art <= lastart) {
	termdown(2);
	in_answer("\n\nAre you starting an unrelated topic? [ynq] ", 'F');
	setdef(buf,"y");
	if (*buf == 'q')  /*TODO: need to add 'h' also */
	    return;
	if (*buf != 'n')
	    art = lastart + 1;
    }
    artopen(art,(ART_POS)0);
    tmpfp = fopen(headname,"w");
    if (tmpfp == NULL) {
	printf(cantcreate,headname) FLUSH;
	termdown(1);
	art = oldart;
	return;
    }
    interp(hbuf, sizeof hbuf, getval("NEWSHEADER",NEWSHEADER));
    fputs(hbuf,tmpfp);
    if (incl_body && artfp != NULL) {
	char* s;
	char* t;
#ifdef VERBOSE
	if (verbose)
	    fputs("\n\
(Be sure to double-check the attribution against the signature, and\n\
trim the quoted article down as much as possible.)\n\
",stdout) FLUSH;
#endif
	interp(buf, (sizeof buf), getval("ATTRIBUTION",ATTRIBUTION));
	fprintf(tmpfp,"%s\n",buf);
	parseheader(art);
	mime_SetArticle();
	clear_artbuf();
	seekart(htype[PAST_HEADER].minpos);
	wrapped_nl = '\n';
	while ((s = readartbuf(FALSE)) != NULL) {
	    if ((t = index(s, '\n')) != NULL)
		*t = '\0';
#ifdef CHARSUBST
	    strcharsubst(hbuf,s,sizeof hbuf,*charsubst);
	    fprintf(tmpfp,"%s%s\n",indstr,hbuf);
#else
	    fprintf(tmpfp,"%s%s\n",indstr,s);
#endif
	    if (t)
		*t = '\0';
	}
	fprintf(tmpfp,"\n");
	wrapped_nl = WRAPPED_NL;
    }
    fclose(tmpfp);
    follow_it_up();
    art = oldart;
}

int
invoke(cmd,dir)
char* cmd;
char* dir;
{
    char oldmode = mode;
    int ret = -1;

#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE)
	nntp_finishbody(FB_SILENT);
#endif
#ifdef DEBUG
    if (debug)
	printf("\nInvoking command: %s\n",cmd);
#endif
    if (dir) {
	if (chdir(dir)) {
	    printf(nocd,dir) FLUSH;
	    return ret;
	}
    }
    set_mode(gmode,'x');
    termlib_reset();
    resetty();			/* make terminal well-behaved */
    ret = doshell(sh,cmd);	/* do the command */
    noecho();			/* set no echo */
    crmode();			/* and cbreak mode */
    termlib_init();
    set_mode(gmode,oldmode);
    if (dir)
	chdir_newsdir();
    return ret;
}

/*
** cut_line() determines if a line is meant as a "cut here" marker.
** Some examples that we understand:
**
**  BEGIN--cut here--cut here
**
**  ------------------ tear at this line ------------------
**
**  #----cut here-----cut here-----cut here-----cut here----#
*/
#if 0
static bool
cut_line(str)
char* str;
{
    char* cp;
    char got_flag;
    char word[80];
    int  dash_cnt, equal_cnt, other_cnt;

    /* Disallow any single-/double-quoted, parenthetical or c-commented
    ** string lines.  Make sure it has the cut-phrase and at least six
    ** '-'s or '='s.  If only four '-'s are present, check for a duplicate
    ** of the cut phrase.  If over 20 unknown characters are encountered,
    ** assume it isn't a cut line.  If we succeed, return TRUE.
    */
    for (cp = str, dash_cnt = equal_cnt = other_cnt = 0; *cp; cp++) {
	switch (*cp) {
	case '-':
	    dash_cnt++;
	    break;
	case '=':
	    equal_cnt++;
	    break;
	case '/':
	    if (*(cp+1) != '*')
		break;
	case '"':
	case '\'':
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	    return FALSE;
	default:
	    other_cnt++;
	    break;
	}
    }
    if (dash_cnt < 4 && equal_cnt < 6)
	return FALSE;

    got_flag = 0;

    for (*(cp = word) = '\0'; *str; str++) {
	if (islower(*str))
	    *cp++ = *str;
	else if (isupper(*str))
	    *cp++ = tolower(*str);
	else {
	    if (*word) {
		*cp = '\0';
		switch (got_flag) {
		case 2:
		    if (strEQ(word, "line")
		     || strEQ(word, "here"))
			if ((other_cnt -= 4) <= 20)
			    return TRUE;
		    break;
		case 1:
		    if (strEQ(word, "this")) {
			got_flag = 2;
			other_cnt -= 4;
		    }
		    else if (strEQ(word, "here")) {
			other_cnt -= 4;
			if ((dash_cnt >= 6 || equal_cnt >= 6)
			 && other_cnt <= 20)
			    return TRUE;
			dash_cnt = 6;
			got_flag = 0;
		    }
		    break;
		case 0:
		    if (strEQ(word, "cut")
		     || strEQ(word, "snip")
		     || strEQ(word, "tear")) {
			got_flag = 1;
			other_cnt -= strlen(word);
		    }
		    break;
		}
		*(cp = word) = '\0';
	    }
	}
    } /* for *str */

    return FALSE;
}
#endif
