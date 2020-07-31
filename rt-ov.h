/* rt-ov.h
*/
/* This software is copyrighted as detailed in the LICENSE file. */


/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

bool ov_init _((void));
int ov_num _((char*,char*));
bool ov_data _((ART_NUM,ART_NUM,bool_int));
void ov_close _((void));
char* ov_fieldname _((int));
char* ov_field _((ARTICLE*,int));
