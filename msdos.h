/* msdos.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include <io.h>
#include <dos.h>
#include <dir.h>
#include <conio.h>
#include <process.h>

FILE*	popen(char*,char*);
int	pclose(FILE*);

#define FILE_REF(s) (*(s)=='/'?'/':(isalpha(*s)&&(s)[1]==':'?(s)[2]:0))

#define chdir ChDir
#define getenv GetEnv

#define FOPEN_RB "rb"
#define FOPEN_WB "wb"

#define B19200	19200
#define B9600	9600
#define B4800	4800
#define B2400	2400
#define B1800	1800
#define B1200	1200
#define B600	600
#define B300	300
#define B200	200
#define B150	150
#define B134	134
#define B110	110
#define B75	75
#define B50	50

#define LIMITED_FILENAMES
#define RESTORE_ORIGDIR
#define NO_FILELINKS
#define WINSOCK
#define LAX_INEWS
#define mkdir(dir,mode) mkdir(dir)
