/* color.h - Color handling for trn 4.0.
 */
/* This software is copyrighted as detailed in the LICENSE file, and
 * this file is also Copyright 1995 by Gran Larsson <hoh@approve.se>. */

/*
** Object numbers.
*/

#define COLOR_DEFAULT	 0
#define COLOR_NGNAME	 1	/* NG name in thread selector	*/
#define COLOR_PLUS	 2	/* + in thread selector		*/
#define COLOR_MINUS	 3	/* - in thread selector		*/
#define COLOR_STAR	 4	/* * in thread selector		*/
#define COLOR_HEADER	 5	/* headers in article display	*/
#define COLOR_SUBJECT	 6	/* subject in article display	*/
#define COLOR_TREE	 7	/* tree in article display	*/
#define COLOR_TREE_MARK	 8	/* tree in article display, marked */
#define COLOR_MORE	 9	/* the more prompt		*/
#define COLOR_HEADING	10	/* any heading			*/
#define COLOR_CMD	11	/* the command prompt		*/
#define COLOR_MOUSE	12	/* the mouse bar		*/
#define COLOR_NOTICE	13	/* notices, e.g. kill handling	*/
#define COLOR_SCORE	14	/* score objects		*/
#define COLOR_ARTLINE1	15	/* the article heading		*/
#define COLOR_MIMESEP	16	/* the multipart mime separator	*/
#define COLOR_MIMEDESC	17	/* the multipart mime description line */
#define COLOR_CITEDTEXT	18	/* cited text color		*/
#define COLOR_BODYTEXT	19	/* regular body text		*/
#define MAX_COLORS	20

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void color_init _((void));
void color_rc_attribute _((char*,char*));
void color_object _((int,bool_int));
void color_pop _((void));
void color_string _((int,char*));
void color_default _((void));
