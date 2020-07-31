case $CONFIG in
'') . ./config.sh ;;
esac
echo "Extracting config.h (with variable substitutions)"
sed <<!GROK!THIS! >config.h -e 's!^#undef\(.*\)/\*!/\*#define\1\*//\*!' -e 's!^#un-def!#undef!'
/*
 * This file was produced by running the config_h.SH script, which
 * gets its values from config.sh, which is generally produced by
 * running Configure.
 *
 * Feel free to modify any of this as the need arises.  Note, however,
 * that running config_h.SH again will wipe out any changes you've made.
 * For a more permanent change edit config.sh and rerun config_h.SH.
 *
 * \$Id: Config_h.U,v 3.0.1.5 1997/02/28 14:57:43 ram Exp $
 */

/*
 * Package name      : $package
 * Source directory  : $src
 * Configuration time: $cf_time
 * Configured by     : $cf_by
 * Target system     : $myuname
 */

#ifndef _config_h_
#define _config_h_

/* EUNICE:
 *	This symbol, if defined, indicates that the program is being compiled
 *	under the EUNICE package under VMS.  The program will need to handle
 *	things like files that don't go away the first time you unlink them,
 *	due to version numbering.  It will also need to compensate for lack
 *	of a respectable link() command.
 */
#$d_eunice EUNICE		/**/

/* HASATTRIBUTE:
 *	This symbol indicates the C compiler can check for function attributes,
 *	such as printf formats. This is normally only supported by GNU cc.
 */
#$d_attribut HASATTRIBUTE 	/**/
#ifndef HASATTRIBUTE
#define __attribute__(_arg_)
#endif

/* HAS_GETPWENT:
 *	This symbol, if defined, indicates that the getpwent routine is
 *	available for sequential access of the passwd database.
 *	If this is not available, the older getpw() function may be available.
 */
#$d_getpwent HAS_GETPWENT		/**/

/* INTERNET:
 *	This symbol, if defined, indicates that there is a mailer available
 *	which supports internet-style addresses (user@site.domain).
 */
#$d_internet	INTERNET	/**/

/* HAS_MEMCMP:
 *	This symbol, if defined, indicates that the memcmp routine is available
 *	to compare blocks of memory.
 */
#$d_memcmp HAS_MEMCMP	/**/

/* HAS_MEMCPY:
 *	This symbol, if defined, indicates that the memcpy routine is available
 *	to copy blocks of memory.
 */
#$d_memcpy HAS_MEMCPY	/**/

/* HAS_MEMSET:
 *	This symbol, if defined, indicates that the memset routine is available
 *	to set blocks of memory.
 */
#$d_memset HAS_MEMSET	/**/

/* HAS_MKDIR:
 *	This symbol, if defined, indicates that the mkdir routine is available
 *	to create directories.  Otherwise you should fork off a new process to
 *	exec /bin/mkdir.
 */
#$d_mkdir HAS_MKDIR		/**/

/* NEWS_ADMIN:
 *	This symbol, if defined, contains the login name of the news
 *	administrator.
 */
#$d_newsadm NEWS_ADMIN "$newsadmin"		/**/

/* NOLINEBUF:
 *	This symbol, if defined, indicates that stdout is not buffered, so that
 *	the program can call setbuf() or setlinebuf() for efficiency.
 */
#$d_nolnbuf	NOLINEBUF	/**/

/* NORMSIG:
 *	This symbol, if defined, indicates that normal signal handling routines
 *	should be used, as opposed to the ones in 4.1bsd (sigset, etc.).
 */
#$d_normsig NORMSIG		/**/

/* HAS_RDCHK:
 *	This symbol, if defined, indicates that the rdchk routine is available
 *	to find out if there is input pending on an IO channel.  Generally
 *	the routine is used only if FIONREAD and O_NDELAY aren't available.
 */
#$d_rdchk HAS_RDCHK		/**/

/* HAS_SIGBLOCK:
 *	This symbol, if defined, indicates that the sigblock routine is
 *	available to block signal reception.
 */
#$d_sigblock HAS_SIGBLOCK	/**/

/* HAS_SIGHOLD:
 *	This symbol, if defined, indicates that the sighold routine is
 *	available to hold signals.
 */
#$d_sighold HAS_SIGHOLD	/**/

/* HAS_STRCASECMP:
 *	This symbol, if defined, indicates that the strcasecmp() routine is
 *	available for case-insensitive string compares.
 */
#$d_strccmp HAS_STRCASECMP	/**/

/* UNION_WAIT:
 *	This symbol if defined indicates to the C program that the argument
 *	for the wait() system call should be declared as 'union wait status'
 *	instead of 'int status'. You probably need to include <sys/wait.h>
 *	in the former case (see I_SYSWAIT).
 */
#$d_uwait UNION_WAIT		/**/

/* HAS_VFORK:
 *	This symbol, if defined, indicates that vfork() exists.
 */
#$d_vfork HAS_VFORK	/**/

/* Signal_t:
 *	This symbol's value is either "void" or "int", corresponding to the
 *	appropriate return type of a signal handler.  Thus, you can declare
 *	a signal handler using "Signal_t (*handler)()", and define the
 *	handler using "Signal_t handler(sig)".
 */
#define Signal_t $signal_t	/* Signal handler's return type */

/* USE_WIFSTAT:
 *	This symbol, if defined, indicates to the C program that the argument
 *	for the WIFxxx set of macros such as WIFSIGNALED or WIFEXITED can
 *	be of the same kind as the one used to hold the wait() status. Should
 *	only matter on HP-UX, where the macros are incorrectly written and
 *	therefore cause programs using them with an 'union wait' variable
 *	to not compile properly. See also UNION_WAIT.
 */
#$d_wifstat USE_WIFSTAT		/**/

/* DEFEDITOR:
 *	This symbol contains the full pathname of the default editor.
 */
#define DEFEDITOR "$defeditor"		/**/

/* I_DIRENT:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <dirent.h>. Using this symbol also triggers the definition
 *	of the Direntry_t define which ends up being 'struct dirent' or
 *	'struct direct' depending on the availability of <dirent.h>.
 */
/* Direntry_t:
 *	This symbol is set to 'struct direct' or 'struct dirent' depending on
 *	whether dirent is available or not. You should use this pseudo type to
 *	portably declare your directory entries.
 */
#$i_dirent I_DIRENT		/**/
#define Direntry_t $direntrytype

/* I_STDLIB:
 *	This symbol, if defined, indicates that <stdlib.h> exists and should
 *	be included.
 */
#$i_stdlib I_STDLIB		/**/

/* I_STRING:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <string.h> (USG systems) instead of <strings.h> (BSD systems).
 */
#$i_string I_STRING		/**/

/* I_SYS_DIR:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <sys/dir.h>.
 */
#$i_sysdir I_SYS_DIR		/**/

/* I_SYS_IOCTL:
 *	This symbol, if defined, indicates that <sys/ioctl.h> exists and should
 *	be included. Otherwise, include <sgtty.h> or <termio.h>.
 */
/* I_SYS_FILIO:
 *	This symbol, if defined, indicates that <sys/filio.h> exists and
 *	should be included instead of <sys/ioctl.h>.
 */
#$i_sysioctl	I_SYS_IOCTL		/**/
#$i_sysfilio	I_SYS_FILIO		/**/

/* I_SYS_NDIR:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <sys/ndir.h>.
 */
#$i_sysndir I_SYS_NDIR	/**/

/* I_SYS_WAIT:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <sys/wait.h>.
 */
#$i_syswait I_SYS_WAIT	/**/

/* I_TERMIO:
 *	This symbol, if defined, indicates that the program should include
 *	<termio.h> rather than <sgtty.h>.  There are also differences in
 *	the ioctl() calls that depend on the value of this symbol.
 */
/* I_TERMIOS:
 *	This symbol, if defined, indicates that the program should include
 *	the POSIX termios.h rather than sgtty.h or termio.h.
 *	There are also differences in the ioctl() calls that depend on the
 *	value of this symbol.
 */
/* I_SGTTY:
 *	This symbol, if defined, indicates that the program should include
 *	<sgtty.h> rather than <termio.h>.  There are also differences in
 *	the ioctl() calls that depend on the value of this symbol.
 */
#$i_termio I_TERMIO		/**/
#$i_termios I_TERMIOS		/**/
#$i_sgtty I_SGTTY		/**/

/* I_UNISTD:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <unistd.h>.
 */
#$i_unistd I_UNISTD		/**/

/* I_UTIME:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <utime.h>.
 */
#$i_utime I_UTIME		/**/

/* I_VFORK:
 *	This symbol, if defined, indicates to the C program that it should
 *	include vfork.h.
 */
#$i_vfork I_VFORK	/**/

/* MAILFILE:
 *	This symbol contains the interpretable name of the mail spool file
 *	for the current user.  The program must be prepared to substitute
 *	the HOME directory for %~, and the login id for %L.
 */
#define MAILFILE "$mailfile"		/**/

/* MBOXCHAR:
 *	This symbol contains a character which will match the beginning
 *	of a mailbox file.
 */
#define MBOXCHAR '$mboxchar'		/**/

/* PASSNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file.
 */
/* BERKNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file in Berkeley format (name first thing, everything
 *	up to first comma, with & replaced by capitalized login id, yuck).
 */
#$d_passnames PASSNAMES /*  (undef to take name from ~/.fullname) */
#$d_berknames BERKNAMES /* (that is, ":name,stuff:") */

/* INSTALLPREFIX:
 *	This symbol contains the name of the install prefix for this package.
 */
#define INSTALLPREFIX "$prefix"		/**/

/* PREFSHELL:
 *	This symbol contains the full name of the preferred user shell on this
 *	system.  Usual values are /bin/csh, /bin/ksh, /bin/sh.
 */
#define PREFSHELL "$prefshell"		/**/

/* ROOTID:
 *	This symbol contains the uid of root, normally 0.
 */
#define ROOTID $rootid		/**/

/* ACTIVE:
 *	The name of the active file for the news system.  This file contains
 *	the list of active newsgroups.  The name may have ~ on the front.
 */
/* ACTIVE_TIMES:
 *	The name of the active.times file for the news system.
 */
#define ACTIVE "$active"		/**/
#$d_acttimes ACTIVE_TIMES "$acttimes"		/**/

/* HAS_FTIME:
 *	This symbol, if defined, indicates that the ftime() routine exists.
 *	It is basically a sub-second accuracy clock, but is less accurate
 *	than gettimeofday(2) anyway. The type "Timeval" should be used to
 *	refer to "struct timeb".
 */
/* HAS_GETTIMEOFDAY:
 *	This symbol, if defined, indicates that the gettimeofday() system
 *	call is available for a sub-second accuracy clock. Usually, the file
 *	<sys/resource.h> needs to be included (see I_SYS_RESOURCE).
 *	The type "Timeval" should be used to refer to "struct timeval".
 */
#$d_ftime HAS_FTIME		/**/
#$d_gettimeod HAS_GETTIMEOFDAY	/**/
#ifdef HAS_GETTIMEOFDAY
#define Timeval struct timeval	/* Structure used by gettimeofday() */
#endif
#if defined(HAS_FTIME) && !defined(Timeval)
#define Timeval struct timeb		/* Structure used by ftime() */
#endif

/* HAS_GETHOSTNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	gethostname() routine to derive the host name.
 */
/* HAS_UNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	uname() routine to derive the host name.
 */
/* PHOSTCMD:
 *	This symbol, if defined, indicates that the C program may use the
 *	contents of PHOSTCMD as a command to feed to the popen() routine
 *	to derive the host name.
 *	Note that the command uses a fully qualified path, so that it is safe
 *	even if used by a process with super-user privileges.
 */
/* HAS_RES_INIT:
 *	This symbol, if defined, indicates that the C program may use the
 *	res_init() routine to derive the domain.
 */
/* HAS_GETDOMAINNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	getdomainname() routine to derive the domain.
 */
#$d_gethname HAS_GETHOSTNAME	/**/
#$d_uname HAS_UNAME		/**/
#$d_phostcmd PHOSTCMD "$aphostcmd"	/* How to get the host name */
#$d_resinit HAS_RES_INIT	/**/
#$d_getdname HAS_GETDOMAINNAME	/**/

/* HAS_GETWD:
 *	This symbol, if defined, indicates that the getwd routine is
 *	available to get the working directory.
 */
/* HAS_GETCWD:
 *	This symbol, if defined, indicates that the getcwd routine is
 *	available to get the working directory.
 */
#$d_getwd HAS_GETWD		/**/
#$d_getcwd HAS_GETCWD		/**/

/* HAS_TERMLIB:
 *	This symbol, when defined, indicates that termlib-style routines
 *	are available.  There is nothing to include.
 */
#$d_havetlib	HAS_TERMLIB	/**/

/* IGNOREORG:
 *	This symbol, if defined, indicates that the ORGANIZATION environment
 *	variable does not contain an organization name.
 */
#$d_ignoreorg IGNOREORG		/**/

/* SUPPORT_NNTP:
 *	This symbol, if defined, indicates that NNTP support is possible.
 */
/* USE_GENAUTH:
 *	This symbol, if defined, indicates that authinfo generic
 *	authentication is to be supported.
 */
/* SERVER_NAME:
 *	When using NNTP, this symbol indicates the server name or a
 *	file to open to read the server name.
 */
/* SUPPORT_XTHREAD:
 *	This symbol, if defined, indicates that .thread files can be
 *	grabbed via NNTP.
 */
/* HAS_LOCAL_SPOOL:
 *	This symbol, if defined, indicates that there's a local spool
 *	directory configured into trn.
 */
#$d_nntp SUPPORT_NNTP	/**/
#$d_genauth USE_GENAUTH	/**/
#define SERVER_NAME "$servername"  	/**/
#$d_xthread SUPPORT_XTHREAD  	/**/
#$d_local HAS_LOCAL_SPOOL	/**/

/* void:
 *	This symbol is used for void functions.  On implementations which
 *	support void appropriately, its value is "void".  Otherwise, its
 *	value should be set to "int".
 */
#$d_novoid void int	/**/

/* HAS_RENAME:
 *	This symbol, if defined, indicates that the rename routine is available
 *	to rename files.  Otherwise you should do the unlink(), link(), unlink()
 *	trick.
 */
#$d_rename HAS_RENAME	/**/

/* size_t:
 *	This symbol is defined as an int if no size_t definition exists.
 */
#$d_sizet	size_t int		/**/

/* HAS_STRCHR:
 *	This symbol is defined to indicate that the strchr()/strrchr()
 *	functions are available for string searching. If not, try the
 *	index()/rindex() pair.
 */
#$d_strchr HAS_STRCHR	/**/

/* HAS_STRFTIME:
 *	This symbol, if defined, indicates that the strftime routine is
 *	available to format locale-specific times.
 */
#$d_strftime HAS_STRFTIME		/**/

/* HAS_STRSTR:
 *	This symbol, if defined, indicates that the strstr routine is
 *	available to find substrings.
 */
#$d_strstr HAS_STRSTR	/**/

/* EMULATE_NDIR:
 *	This symbol, if defined, indicates that the program should compile
 *	the ndir.c code provided with the package.
 */
/* I_NDIR:
 *	This symbol, if defined, indicates that the program should include the
 *	system's version of ndir.h, rather than the one with this package.
 */
#$d_usendir	EMULATE_NDIR		/**/
#$d_libndir	I_NDIR		/**/

/* I_PTEM:
 *	This symbol, if defined, indicates to the C program that it should
 *	include ptem.h.
 */
#$i_ptem I_PTEM		/**/

/* I_TIME:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <time.h>.
 */
/* I_SYS_TIME:
 *	This symbol, if defined, indicates to the C program that it should
 *	include <sys/time.h>.
 */
#$i_time I_TIME		/**/
#$i_systime I_SYS_TIME		/**/

/* MIMECAP:
 *	This symbol contains the pathname of mimecap file, which controls
 *	what programs get run when handling mime articles.  It is often
 *	metamail's "mailcap" file, and is the same format.
 */
#define MIMECAP "%./.mimecap:~/.mailcap:$mimecap"		/**/

/* NEWSLIB:
 *	This symbol contains the name of the directory serving as the news
 *	library.  The program must be prepared to do ~ expansion on it.
 */
/* EXTRAINEWS:
 *	Contains the full path and filename of an inews to use for local
 *	postings, or nothing.
 */
/* GROUPDESC:
 *	Contains the full path and filename of the "newsgroups" file for
 *	group descriptions, or nothing.
 */
/* SUBSCRIPTIONS:
 *	Contains the full path and filename of the list of default
 *	subscriptions, or nothing.
 */
#define NEWSLIB "$newslib"		/**/
#define EXTRAINEWS "$extrainews"	/**/
#define GROUPDESC "$groupdesc"	/**/
#define SUBSCRIPTIONS "$subscriptions"	/**/

/* NEWSSPOOL:
 *	This symbol contains the directory name where news articles are
 *	spooled.  The program must be prepared to do ~ expansion on it.
 */
#define NEWSSPOOL "$newsspool"		/**/

/* ORGNAME:
 *	This symbol contains either the organizaton name or the full pathname
 *	of a file containing the organization name, which the program must
 *	be prepared to open and substitute the contents of.
 */
#define ORGNAME "$orgname"		/**/

/* PHOSTNAME:
 *	This symbol contains the posting host's name or a file from which
 *	to read its name.
 */
/* HOSTBITS:
 *	Set to TRUE if we should compare only the domain portion of the
 *	hostname when looking for local articles.
 */
#define PHOSTNAME "$phost"		/**/
#define HOSTBITS $hostbits	/**/

/* PRIVLIB:
 *	This symbol contains the name of the private library for this package.
 *	The library is private in the sense that it needn't be in anyone's
 *	execution path, but it should be accessible by the world.  The program
 *	should be prepared to do ~ expansion.
 */
#define PRIVLIB "$privlib"		/**/

/* SCAN:
 *	This is defined if you want strn's scan mode.
 */
/* SCORE:
 *	This is defined if you want strn's article scoring.
 */
#$strn SCAN		/**/
#$strn SCORE		/**/

/* THREAD_DIR:
 *	This symbol indicates where the thread files go.
 */
/* OVERVIEW_DIR:
 *	This symbol indicates where the overview files go.
 */
/* OVERVIEW_FMT:
 *	The overview.fmt file.
 */
#define THREAD_DIR	"$threaddir"		/**/
#define OVERVIEW_DIR	"$overviewdir"		/**/
#define OVERVIEW_FMT	"$overviewfmt"		/**/

/* USE_TK:
 *	This is defined if you want to link trn with Tk.
 */
#$tk USE_TK		/**/

/* THREAD_INIT:
 *	This symbol indicates we act like trn no matter what our name is.
 */
/* SELECT_INIT:
 *	This symbol indicates we default to the selector for group entry.
 */
/* MSDOS:
 *	This is defined if this is an MSDOS system.
 */
#define THREAD_INIT	$trn_init
#define SELECT_INIT	$trn_select

#define CALL_INEWS "$useinews -h <%h"
#define NEWSPOSTER "$bin/Pnews -h %h"
#define MAILPOSTER "$bin/Rnmail -h %h"
/*#define ANCIENT_NEWS	*//* if your B news system is <= 2.10.1 */

#$d_msdos MSDOS	/**/

#endif
!GROK!THIS!
