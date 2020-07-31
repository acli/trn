/* list.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */

struct listnode {
    LISTNODE* next;
    /*LISTNODE* mid;*/
    long low;
    long high;
    char* data_high;
    char data[1];  /* this is actually longer */
};

struct list {
    LISTNODE* first;
    LISTNODE* recent;
    void (*init_node) _((LIST*,LISTNODE*));
    long low;
    long high;
    int item_size;
    int items_per_node;
    int flags;
};

#define LF_ZERO_MEM	0x0001
#define LF_SPARSE	0x0002

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void list_init _((void));
LIST* new_list _((long,long,int,int,int,void(*) _((LIST*,LISTNODE*))));
char* listnum2listitem _((LIST*,long));
long listitem2listnum _((LIST*,char*));
bool walk_list _((LIST*,bool(*) _((char*,int)),int));
long existing_listnum _((LIST*,long,int));
char* next_listitem _((LIST*,char*));
char* prev_listitem _((LIST*,char*));
void delete_list _((LIST*));
