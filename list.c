/* list.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "INTERN.h"
#include "list.h"
#include "list.ih"

void
list_init()
{
    ;
}


/* Create the header for a dynamic list of items.
*/
LIST*
new_list(low, high, item_size, items_per_node, flags, init_node)
long low;
long high;
int item_size;
int items_per_node;
int flags;
void (*init_node) _((LIST*,LISTNODE*));
{
    LIST* list = (LIST*)safemalloc(sizeof (LIST));
    list->first = list->recent = NULL;
    list->init_node = init_node? init_node : def_init_node;
    list->low = low;
    list->high = high;
    list->item_size = item_size;
    list->items_per_node = items_per_node;
    list->flags = flags;

    return list;
}

/* The default way to initialize a node */
static void
def_init_node(list, node)
LIST* list;
LISTNODE* node;
{
    if (list->flags & LF_ZERO_MEM)
	bzero(node->data, list->items_per_node * list->item_size);
}

/* Take the number of a list element and return its pointer.  This
** will allocate new list items as needed, keeping the list->high
** value up to date.
*/
char*
listnum2listitem(list, num)
LIST* list;
long num;
{
    LISTNODE* node = list->recent;
    LISTNODE* prevnode = NULL;

    if (node && num < node->low)
	node = list->first;
    for (;;) {
	if (!node || num < node->low) {
	    node = (LISTNODE*)safemalloc(list->items_per_node*list->item_size
					+ sizeof (LISTNODE) - 1);
	    if (list->flags & LF_SPARSE)
		node->low = ((num - list->low) / list->items_per_node)
			* list->items_per_node + list->low;
	    else
		node->low = num;
	    node->high = node->low + list->items_per_node - 1;
	    node->data_high = node->data
			    + (list->items_per_node - 1) * list->item_size;
	    if (node->high > list->high)
		list->high = node->high;
	    if (prevnode) {
		node->next = prevnode->next;
		prevnode->next = node;
	    }
	    else {
		node->next = list->first;
		list->first = node;
	    }
	    /*node->mid = $$;*/
	    list->init_node(list, node);
	    break;
	}
	if (num <= node->high)
	    break;
	prevnode = node;
	node = node->next;
    }
    list->recent = node;
    return node->data + (num - node->low) * list->item_size;
}

/* Take the pointer of a list element and return its number.  The item
** must already exist or this will infinite loop.
*/
long
listitem2listnum(list, ptr)
LIST* list;
char* ptr;
{
    LISTNODE* node;
    char* cp;
    int i;
    int item_size = list->item_size;

    for (node = list->recent; ; node = node->next) {
	if (!node)
	    node = list->first;
	i = node->high - node->low + 1;
	for (cp = node->data; i--; cp += item_size) {
	    if (ptr == cp) {
		list->recent = node;
		return (ptr - node->data) / list->item_size + node->low;
	    }
	}
    }
    return -1;
}

/* Execute the indicated callback function on every item in the list.
*/
bool
walk_list(list, callback, arg)
LIST* list;
bool (*callback) _((char*,int));
int arg;
{
    LISTNODE* node;
    char* cp;
    int i;
    int item_size = list->item_size;

    for (node = list->first; node; node = node->next) {
	i = node->high - node->low + 1;
	for (cp = node->data; i--; cp += item_size)
	    if (callback(cp, arg))
		return 1;
    }
    return 0;
}

/* Since the list can be sparsely allocated, find the nearest number
** that is already allocated, rounding in the indicated direction from
** the initial list number.
*/
long
existing_listnum(list, num, direction)
LIST* list;
long num;
int direction;
{
    register LISTNODE* node = list->recent;
    LISTNODE* prevnode = NULL;

    if (node && num < node->low)
	node = list->first;
    while (node) {
	if (num <= node->high) {
	    if (direction > 0) {
		if (num < node->low)
		    num = node->low;
	    }
	    else if (direction == 0) {
		if (num < node->low)
		    num = 0;
	    }
	    else if (num < node->low) {
		if (!prevnode)
		    break;
		list->recent = prevnode;
		return prevnode->high;
	    }
	    list->recent = node;
	    return num;
	}
	prevnode = node;
	node = node->next;
    }
    if (!direction)
	return 0;
    if (direction > 0)
	return list->high + 1;
    return list->low - 1;
}

/* Increment the item pointer to the next allocated item.
** Returns NULL if ptr is the last one.
*/
char*
next_listitem(list, ptr)
LIST* list;
char* ptr;
{
    register LISTNODE* node = list->recent;

    if (ptr == node->data_high) {
	node = node->next;
	if (!node)
	    return NULL;
	list->recent = node;
	return node->data;
    }
#if 0
    if (node->high > list->high) {
	if ((ptr - node->data) / list->item_size + node->low >= list->high)
	    return NULL;
    }
#endif
    return ptr += list->item_size;
}

/* Decrement the item pointer to the prev allocated item.
** Returns NULL if ptr is the first one.
*/
char*
prev_listitem(list, ptr)
LIST* list;
char* ptr;
{
    register LISTNODE* node = list->recent;

    if (ptr == node->data) {
	LISTNODE* prev = list->first;
	if (prev == node)
	    return NULL;
	while (prev->next != node)
	    prev = prev->next;
	list->recent = prev;
	return prev->data_high;
    }
    return ptr -= list->item_size;
}

/* Delete the list and all its allocated nodes.  If you need to cleanup
** the individual nodes, call walk_list() with a cleanup function before
** calling this.
*/
void
delete_list(list)
LIST* list;
{
    LISTNODE* node = list->first;
    LISTNODE* prevnode = NULL;

    while (node) {
	prevnode = node;
	node = node->next;
	free((char*)prevnode);
    }
    free((char*)list);
}
