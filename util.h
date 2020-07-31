/* util.h
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "utf.h"

EXT bool waiting INIT(FALSE);  	/* waiting for subprocess (in doshell)? */
EXT bool nowait_fork INIT(FALSE);
EXT bool export_nntp_fds INIT(FALSE);

/* the strlen and the buffer length of "some_buf" after a call to:
 *     some_buf = get_a_line(bufptr,bufsize,realloc,fp); */
EXT int len_last_line_got INIT(0);
EXT MEM_SIZE buflen_last_line_got INIT(0);

#define AT_GREY_SPACE(s) !at_norm_char(s)
#define AT_NORM_CHAR(s)  at_norm_char(s)

/* is the string for makedir a directory name or a filename? */

#define MD_DIR 	0
#define MD_FILE 1

/* a template for parsing an ini file */

struct ini_words {
    int checksum;
    char* item;
    char* help_str;
};

#define INI_LEN(words)         (words)[0].checksum
#define INI_VALUES(words)      ((char**)(words)[0].help_str)
#define INI_VALUE(words,num)   INI_VALUES(words)[num]

#define safefree(ptr)  if (!ptr) ; else free((char*)(ptr))
#define safefree0(ptr)  if (!ptr) ; else free((char*)(ptr)), (ptr)=0

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void util_init _((void));
int doshell _((char*,char*));
#ifndef USE_DEBUGGING_MALLOC
char* safemalloc _((MEM_SIZE));
char* saferealloc _((char*,MEM_SIZE));
#endif
char* safecat _((char*,char*,int));
#ifdef SETUIDGID
int eaccess _((char*,int));
#endif
char* trn_getwd _((char*,int));
char* get_a_line _((char*,int,bool_int,FILE*));
int makedir _((char*,int));
void notincl _((char*));
void growstr _((char**,int*,int));
void setdef _((char*,char*));
#ifndef NO_FILELINKS
void safelink _((char*,char*));
#endif
#ifndef HAS_STRSTR
char* trn_strstr _((char*,char*));
#endif
void verify_sig _((void));
double current_time _((void));
time_t text2secs _((char*,time_t));
char* secs2text _((time_t));
char* temp_filename _((void));
#ifdef SUPPORT_NNTP
char* get_auth_user _((void));
char* get_auth_pass _((void));
#endif
#if defined(USE_GENAUTH) && defined(SUPPORT_NNTP)
char* get_auth_command _((void));
#endif
char** prep_ini_words _((INI_WORDS*));
void unprep_ini_words _((INI_WORDS*));
void prep_ini_data _((char*,char*));
bool parse_string _((char**,char**));
char* next_ini_section _((char*,char**,char**));
char* parse_ini_section _((char*,INI_WORDS*));
bool check_ini_cond _((char*));
char menu_get_char _((void));
int edit_file _((char*));
