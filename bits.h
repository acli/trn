/* bits.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT int dmcount INIT(0);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void bits_init _((void));
void rc_to_bits _((void));
bool set_firstart _((char*));
void bits_to_rc _((void));
void find_existing_articles _((void));
void onemore _((ARTICLE*));
void oneless _((ARTICLE*));
void oneless_artnum _((ART_NUM));
void onemissing _((ARTICLE*));
void unmark_as_read _((ARTICLE*));
void set_read _((ARTICLE*));
void delay_unmark _((ARTICLE*));
void mark_as_read _((ARTICLE*));
void mark_missing_articles _((void));
void check_first _((ART_NUM));
void yankback _((void));
int chase_xrefs _((bool_int));
