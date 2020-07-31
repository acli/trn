/* This file Copyright 1993 by Clifford A. Adams */
/* url.h
 *
 * Routines for handling WWW URL references.
 */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

bool fetch_http _((char*,int,char*,char*));
bool fetch_ftp _((char*,char*,char*));
bool parse_url _((char*));
bool url_get _((char*,char*));
