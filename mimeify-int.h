/* mimeify-int.h
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef MIMEIFY_INT_H_INCLUDED
#define MIMEIFY_INT_H_INCLUDED

typedef struct mimeify_inner_status {
    bool has8bit;
    int maxbytelen;
} mimeify_inner_status_t;

typedef struct mimeify_status {
    mimeify_inner_status_t head;
    mimeify_inner_status_t body;
} mimeify_status_t;

int mimeify_scan_input(FILE *, FILE *, mimeify_status_t *);

#endif
