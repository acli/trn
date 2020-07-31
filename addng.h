/* addng.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


struct addgroup {
    ADDGROUP* next;
    ADDGROUP* prev;
    DATASRC* datasrc;
    ART_NUM toread;	/* number of articles to be read (for sorting) */
    NG_NUM num;		/* a possible sort order for this group */
    char flags;
    char name[1];
};

#define AGF_SEL		0x01
#define AGF_DEL		0x02
#define AGF_DELSEL	0x04
#define AGF_INCLUDED	0x10

#define AGF_EXCLUDED	0x20

EXT ADDGROUP* first_addgroup;
EXT ADDGROUP* last_addgroup;

EXT ADDGROUP* sel_page_gp;
EXT ADDGROUP* sel_next_gp;

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void addng_init _((void));
bool find_new_groups _((void));
bool scanactive _((bool_int));
void sort_addgroups _((void));
