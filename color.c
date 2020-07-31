/* color.c -  Color handling for trn 4.0.
 */
/* This software is copyrighted as detailed in the LICENSE file, and
 * this file is also Copyright 1995 by Gran Larsson <hoh@approve.se>. */

/*
** The color handling is implemented as an attribute stack containing
** foreground color, background color, and video attribute for an object.
** Objects are screen features like "thread tree", "header lines",
** "subject line", and "command prompt". The intended use is something
** like this:
**
**	color_object(COLOR_HEADER, 1);
**	fputs(header_string, stdout);
**	color_pop();
**
** The color_pop function will take care of restoring all colors and
** attribute to the state before the color_object function was called.
**
** Colors and attributes are parsed from the [attribute] section
** in the trnrc file. Escape sequences for the colors are picked up
** from term.c by calling the function tc_color_capability.
**
** If colors were specified in the [attribute] section, then colors
** are used, otherwise only normal monochrome video attributes.
*/

#include "EXTERN.h"
#include "common.h"
#include "term.h"
#include "util.h"
#include "final.h"
#include "INTERN.h"
#include "color.h"
#include "color.ih"

/*
** Object properties.
**
** Give default attributes that are used if the trnrc file has no,
** or just a few, lines in the [attribute] section.
*/

static COLOR_OBJ objects[MAX_COLORS] = {
    { "default",	nullstr, nullstr, NOMARKING	},
    { "ngname",		nullstr, nullstr, STANDOUT	},
    { "plus",		nullstr, nullstr, LASTMARKING	},
    { "minus",		nullstr, nullstr, LASTMARKING	},
    { "star",		nullstr, nullstr, LASTMARKING	},
    { "header",		nullstr, nullstr, LASTMARKING	},
    { "subject",	nullstr, nullstr, UNDERLINE	},
    { "tree",		nullstr, nullstr, LASTMARKING	},
    { "tree marker",	nullstr, nullstr, STANDOUT	},
    { "more",		nullstr, nullstr, STANDOUT	},
    { "heading",	nullstr, nullstr, STANDOUT	},
    { "command",	nullstr, nullstr, STANDOUT	},
    { "mouse bar",	nullstr, nullstr, STANDOUT	},
    { "notice",		nullstr, nullstr, STANDOUT	},
    { "score",		nullstr, nullstr, STANDOUT	},
    { "art heading",	nullstr, nullstr, LASTMARKING	},
    { "mime separator",	nullstr, nullstr, STANDOUT	},
    { "mime description",nullstr,nullstr, UNDERLINE	},
    { "cited text",	nullstr, nullstr, LASTMARKING	},
    { "body text",	nullstr, nullstr, NOMARKING	},
};

/* The attribute stack.  The 0th element is always the "normal" object. */
static struct {
    COLOR_OBJ object;
} color_stack[STACK_SIZE];

static int stack_pointer = 0;

/* Initialize color support after trnrc is read. */
void
color_init()
{
    if (use_colors) {
	char* fg;
	char* bg;
	int i;

	/* Get default capabilities. */
	if ((fg = tc_color_capability("fg default")) == NULL) {
	    fprintf(stderr,"trn: you need a 'fg default' definition in the [termcap] section.\n");
	    finalize(1);
	}
	if ((bg = tc_color_capability("bg default")) == NULL) {
	    fprintf(stderr,"trn: you need a 'bg default' definition in the [termcap] section.\n");
	    finalize(1);
	}
	if (strEQ(fg, bg))
	    bg = nullstr;
	for (i = 0; i < MAX_COLORS; i++) {
	    if (objects[i].fg == nullstr)
		objects[i].fg = fg;
	    if (objects[i].bg == nullstr)
		objects[i].bg = bg;
	}
    }

    if (objects[COLOR_DEFAULT].attr == LASTMARKING)
	objects[COLOR_DEFAULT].attr = NOMARKING;

    /* Set color to default. */
    color_default();
}

/* Parse a line from the [attribute] section of trnrc. */
void
color_rc_attribute(object, value)
char* object;
char* value;
{
    char* s;
    char* t;
    char* n = NULL;
    int i;

    /* Find the specified object. */
    for (i = 0; i < MAX_COLORS; i++) {
	if (strcaseEQ(object, objects[i].name))
	    break;
    }
    if (i >= MAX_COLORS) {
	fprintf(stderr,"trn: unknown object '%s' in [attribute] section.\n",
		object);
	finalize(1);
    }

    /* Parse the video attribute. */
    if (*value == 's' || *value == 'S')
	objects[i].attr = STANDOUT;
    else if (*value == 'u' || *value == 'U')
	objects[i].attr = UNDERLINE;
    else if (*value == 'n' || *value == 'N')
	objects[i].attr = NOMARKING;
    else if (*value == '-')
	objects[i].attr = LASTMARKING;
    else {
	fprintf(stderr,"trn: bad attribute '%s' for %s in [attribute] section.\n",
		value, object);
	finalize(1);
    }

    /* See if they specified a color */
    for (s = value; *s && !isspace(*s); s++) ;
    while (isspace(*s)) s++;
    if (!*s) {
	objects[i].fg = nullstr;
	objects[i].bg = nullstr;
	return;
    }
    for (t = s; *t && !isspace(*t); t++) ;
    if (*t) {
	*(n = t++) = '\0';
	while (isspace(*t)) t++;
    }

    /* We have both colors and attributes, so turn colors on. */
    use_colors = TRUE;

    /* Parse the foreground color. */
    if (*s == '-')
	objects[i].fg = NULL;
    else {
	sprintf(buf, "fg %s", s);
	objects[i].fg = tc_color_capability(buf);
	if (objects[i].fg == NULL) {
	    fprintf(stderr,"trn: no color '%s' for %s in [attribute] section.\n",
		    buf, object);
	    finalize(1);
	}
    }
    if (n) {
	*n = ' ';
	n = NULL;
    }

    /* Make sure we have one more parameter. */
    for (s = t; *t && !isspace(*t); t++) ;
    if (*t) {
	*(n = t++) = '\0';
	while (isspace(*t)) t++;
    }
    if (!*s || *t) {
	fprintf(stderr,"trn: wrong number of parameters for %s in [attribute] section.\n",
		object);
	finalize(1);
    }

    /* Parse the background color. */
    if (*s == '-')
	objects[i].bg = NULL;
    else {
	sprintf(buf, "bg %s", s);
	objects[i].bg = tc_color_capability(buf);
	if (objects[i].bg == NULL) {
	    fprintf(stderr,"trn: no color '%s' for %s in [attribute] section.\n",
		    buf, object);
	    finalize(1);
	}
    }
    if (n)
	*n = ' ';
}

/* Turn on color attribute for an object. */
void
color_object(object, push)
int object;
bool_int push;
{
    COLOR_OBJ merged;

    /* Merge in the colors/attributes that we are not setting
     * from the current object. */
    merged = color_stack[stack_pointer].object;

    /* Merge in the new colors/attributes. */
    if (objects[object].fg)
	merged.fg = objects[object].fg;
    if (objects[object].bg)
	merged.bg = objects[object].bg;
    if (objects[object].attr != LASTMARKING)
	merged.attr = objects[object].attr;

    /* Push onto stack. */
    if (push && ++stack_pointer >= STACK_SIZE) {
	/* error reporting? $$ */
	stack_pointer = 0;		/* empty stack */
	color_default();		/* and set normal colors */
	return;
    }
    color_stack[stack_pointer].object = merged;

    /* Set colors/attributes. */
    output_color();
}

/* Pop the color/attribute stack. */
void
color_pop()
{
    /* Trying to pop an empty stack? */
    if (--stack_pointer < 0)
	stack_pointer = 0;
    else
	output_color();
}

/* Color a string with the given object's color/attribute. */
void
color_string(object, str)
int object;
char* str;
{
    int len = strlen(str);
    if (str[len-1] == '\n') {
	strcpy(msg, str);
	msg[len-1] = '\0';
	str = msg;
	len = 0;
    }
    if (!use_colors && *tc_UC && objects[object].attr == UNDERLINE)
	underprint(str);	/* hack for stupid terminals */
    else {
	color_object(object, 1);
	fputs(str, stdout);
	color_pop();
    }
    if (!len)
	putchar('\n');
}

/* Turn off color attribute. */
void
color_default()
{
    color_stack[stack_pointer].object = objects[COLOR_DEFAULT];
    output_color();
}

/* Set colors/attribute for an object. */
static void
output_color()
{
    static COLOR_OBJ prior = { nullstr, NULL, NULL, NOMARKING };
    COLOR_OBJ* op = &color_stack[stack_pointer].object;

    /* If no change, just return. */
    if (op->attr == prior.attr && op->fg == prior.fg && op->bg == prior.bg)
	return;

    /* Start by turning off any existing colors and/or attributes. */
    if (use_colors) {
	if (objects[COLOR_DEFAULT].fg != prior.fg
	 || objects[COLOR_DEFAULT].bg != prior.bg) {
	    fputs(prior.fg = objects[COLOR_DEFAULT].fg, stdout);
	    fputs(prior.bg = objects[COLOR_DEFAULT].bg, stdout);
	}
    }
    switch (prior.attr) {
      case NOMARKING:
	break;
      case STANDOUT:
	un_standout();
	break;
      case UNDERLINE:
	un_underline();
	break;
    }

    /* For color terminals we set the foreground and background color. */
    if (use_colors) {
	if (op->fg != prior.fg)
	    fputs(prior.fg = op->fg, stdout);
	if (op->bg != prior.bg)
	    fputs(prior.bg = op->bg, stdout);
    }

    /* For both monochrome and color terminals we set the video attribute. */
    switch (prior.attr = op->attr) {
      case NOMARKING:
	break;
      case STANDOUT:
#ifdef NOFIREWORKS
	no_sofire();
#endif
	standout();
	break;
      case UNDERLINE:
#ifdef NOFIREWORKS
	no_ulfire();
#endif
	underline();
	break;
    }
}
