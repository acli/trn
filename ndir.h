/* ndir.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#if defined(I_DIRENT) && !defined(EMULATE_NDIR)
#include <dirent.h>
#else
#ifdef I_NDIR
#include <ndir.h>
#else
#ifdef I_SYS_NDIR
#include <sys/ndir.h>
#else
#ifdef I_SYS_DIR
#include <sys/dir.h>
#else

#ifndef DEV_BSIZE
#define	DEV_BSIZE	512
#endif
#define DIRBLKSIZ	DEV_BSIZE
#define	MAXNAMLEN	255

Direntry_t {
	long	d_ino;			/* inode number of entry */
	short	d_reclen;		/* length of this record */
	short	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name must be no longer than this */
};

/*
 * The DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in Direntry_t
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#define DIRSIZ(dp) ((sizeof(Direntry_t)-(MAXNAMLEN+1))+(((dp)->d_namlen+1+3)&~3))

/*
 * Definitions for library routines operating on directories.
 */
typedef struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	char	dd_buf[DIRBLKSIZ];
} DIR;
#ifndef NULL
#define NULL 0
#endif
DIR* opendir _((char*));
Direntry_t* readdir _((DIR*));
long telldir _((DIR*));
void seekdir _((DIR*));
#define rewinddir(dirp)	seekdir((dirp), (long)0)
void closedir _((DIR*));

#endif
#endif
#endif
#endif
