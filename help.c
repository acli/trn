/* help.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "ngdata.h"
#include "term.h"
#include "INTERN.h"
#include "help.h"

void
help_init()
{
    ;
}

int
help_page()
{
    int cmd;
#ifdef PAGERHELP
    doshell(sh,filexp(PAGERHELP));
#else
    page_start();
    if ((cmd = print_lines("\
Paging commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
SP	Display the next page.\n\
x	Change the current article filter (between ROT-13 and WWW viewer).\n\
d	Display half a page more.\n\
CR	Display one more line.\n\
^R,v,^X	Restart the current article (v=verbose header, ^X=current filter).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
b	Back up one page.\n\
^E	Display the last page of the article.\n\
^L,X	Refresh the screen (X=rot13).\n\
_C      Switch characterset conversion.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
t	Display the entire article tree and all its subjects.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
g pat	Go to (search forward within article for) pattern.\n\
G	Search again for current pattern within article.\n\
^G	Search for next line beginning with \"Subject:\".\n\
TAB	Search for next line beginning with a different character.\n\
q	Quit the pager, go to end of article.  Leave article read or unread.\n\
j	Junk this article (mark it read).  Goes to end of article.\n\
h, ?	Enter online help browser.\n\
H	This help.\n\
\n\
",NOMARKING)) ||
    (cmd = print_lines("\
The following commands skip the rest of the current article, then behave\n\
just as if typed to the 'What next?' prompt at the end of the article:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
n	Scan forward for next unread article.\n\
N	Go to next article.\n\
^N	Scan forward for next unread article with same title.\n\
p,P,^P	Same as n,N,^N, only going backwards.\n\
-	Go to previously displayed article.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
<, >	Browse the previous/next selected thread.  If no threads are selected,\n\
	all threads that had unread news upon entry to the group are considered\n\
	selected for browsing.  Entering an empty group browses all threads.\n\
[, ]	Go to article's parent/child (try left-/right-arrow also).\n\
(, )	Go to article's previous/next sibling (try up-/down-arrow also).\n\
{, }	Go to tree's root/leaf.\n\
\n\
",NOMARKING)) ||
#ifdef SCAN
    (cmd = print_lines("\
;	Enter article-scan mode.\n\
",NOMARKING)) ||
#endif
#ifdef SCORE
    (cmd = print_lines("\
Scoring commands are enabled.\n\
(Type h at end of article for description of Scoring commands.)\n\
",NOMARKING)) ||
#endif
    (cmd = print_lines("\
The following commands also take you to the end of the article.\n\
Type h at end of article for a description of these commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\
	# $ & / = ? c C f F k K ^K J , m M number e r R ^R s S u U v w W Y ^ |\n\
\n\
(To return to the middle of the article after one of these commands, type ^L.)\n\
",NOMARKING)) )
	return cmd;
#endif
    return 0;
}

int
help_art()
{
    int cmd;
#ifdef ARTHELP
    doshell(sh,filexp(ARTHELP));
#else
    page_start();
    if ((cmd = print_lines("\
Article Selection commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
n,SP	Find next unread article (follows discussion-tree in threaded groups).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
N	Go to next article.\n\
^N	Scan forward for next unread article with same subject in date order.\n\
p,P,^P	Same as n,N,^N, only going backwards.\n\
_N,_P	Go to the next/previous article numerically.\n\
-	Go to previously displayed article.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
<, >	Browse the previous/next selected thread.  If no threads are selected,\n\
	all threads that had unread news upon entry to the group are considered\n\
	selected for browsing.  Entering an empty group browses all threads.\n\
[, ]	Go to article's parent/child (try left-/right-arrow also).\n\
(, )	Go to article's previous/next sibling (try up-/down-arrow also).\n\
{, }	Go to tree's root/leaf.\n\
t	Display the entire article tree and all its subjects.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
number	Go to specified article.\n\
range{,range}:command{:command}\n\
	Apply one or more commands to one or more ranges of articles.\n\
	Ranges are of the form: number | number-number.  You may use . for\n\
	the current article, and $ for the last article.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
 	Valid commands are: a, e, j, m, M, s, S, t, T, |, +, ++, -, and --.\n\
:cmd	Perform a command on all the selected articles.\n\
::cmd	Perform a command on all non-selected articles.\n\
:.cmd	Perform a command on the current thread or its selected articles.\n\
::.cmd	Perform a command on the unselected articles in the current thread.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
/pattern/modifiers\n\
	Scan forward for article containing pattern in the subject line.\n\
	(Use ?pat? to scan backwards; append f to scan from lines, h to scan\n\
	whole headers, a to scan entire articles, r to scan read articles, c\n\
	to make case-sensitive, t to scan from the top of the group.)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
/pattern/modifiers:command{:command}\n\
	Apply one or more commands to the set of articles matching pattern.\n\
	Use a K modifier to save entire command to the KILL file for this\n\
	newsgroup.  If the m command is first, it implies an r modifier.\n\
 	Valid commands are the same as for the range command.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
f,F	Submit a followup article (F = include this article).\n\
r,R	Reply through net mail (R = include this article).\n\
^F	Forward article through net mail.\n\
e dir{|command}\n\
	Extract a mime, uue, or shar file to \"dir\" (using command).\n\
s ...	Save to file or pipe via sh.\n\
S ...	Save via preferred shell.\n\
w,W	Like s and S but save without the header.\n\
| ...	Same as s|...\n\
a	View article attachments.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
C	Cancel this article, if yours.\n\
^R,v	Restart article (v=verbose).\n\
x	Change the current article filter (between ROT-13 and WWW viewer).\n\
^X	View article using current filter (ROT-13 (default) or WWW viewer).\n\
_C      Switch characterset conversion.\n\
c	Catch up (mark all articles as read).\n\
b	Back up one page.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
^E	Display the last page of the article.\n\
^L	Refresh the screen.  You can get back to the pager with this.\n\
X	Refresh screen in rot13 mode.\n\
^	Go to first unread article.  Disables subject search mode.\n\
$	Go to end of newsgroup.  Disables subject search mode.\n\
",NOMARKING)) ||
    (cmd = print_lines("#       Print last article number.\n\
&	Start the option selector.\n\
&switch {switch}\n\
	Set or unset more switches.\n\
&&	Print current macro definitions.\n\
&&def	Define a new macro.\n\
j	Junk this article (mark it read).  Stays at end of article.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
m	Mark article as still unread.\n\
M	Mark article as read but to-return on group exit or Y command.\n\
Y	Yank back articles marked as to-return via the M command.\n\
k	Kill current subject (mark articles as read).\n\
,	Mark current article and its replies as read.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
J	Junk entire thread (mark all subjects as read in this thread).\n\
A	Add current subject to memorized commands (selection or killing).\n\
T	Add current (sub)thread to memorized commands (selection or killing).\n\
K	Mark current subject as read, and save command in KILL file.\n\
^K	Edit local KILL file (the one for this newsgroup).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
=	List subjects of unread articles.\n\
+	Start the selector in whatever mode it was last in.\n\
_a	Start the article selector.\n\
_s	Start the subject selector.\n\
_t	Start the thread selector.\n\
",NOMARKING)) ||
#ifdef SCAN
    (cmd = print_lines("\
;	Enter article-scan mode.\n\
",NOMARKING)) ||
#endif
    (cmd = print_lines("\
_T	Start the thread selector if threaded, else the subject selector.\n\
U	Unread some news -- prompts for thread, subthread, all, or select.\n\
u	Unsubscribe from this newsgroup.\n\
q	Quit this newsgroup for now.\n\
Q	Quit newsgroup, staying at current newsgroup.\n\
",NOMARKING))
#ifdef SCORE
    || (cmd = print_lines("\
Scoring commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\
\"\" <text>\n\
	Adds <text> to the local (this newsgroup's) scorefile.\n\
\"* <text>\n\
	Adds <text> to the global scorefile.\n\
\"C <text>\n\
	Adds <text> to the scorefile represented by the abbreviation C.\n\
\"?	List all scorefile abbreviations and the filenames they represent.\n\
\"! <text>\n\
	Adds <text> to the internal list of scoring rules as if it was read\n\
	from a file.  Articles are rescored after this command.\n\
'd	Debugging dump.  (Will go away in later versions.)\n\
'e <abbreviation OR filename>\n\
	Edits the scorefile named by the following file abbreviation\n\
	character or full filename.  If no arguments are given, edits the\n\
	local scorefile (the one for the current newsgroup).\n\
'f	Continue scoring articles until interrupted.\n\
'r	Discard old scoring data and rescore from score files.\n\
's	Show scoring rules which contributed to this article's score.\n\
't	Test command: may do varying things.  (will usually be disabled)\n\
",NOMARKING))
#endif /* SCORE */
    )
	return cmd;
#endif
    return 0;
}

int
help_ng()
{
    int cmd;
#ifdef NGHELP
    doshell(sh,filexp(NGHELP));
#else
    page_start();
    if ((cmd = print_lines("\
Newsgroup Selection commands:\n\
",STANDOUT)) != 0)
	return cmd;
    if (ngptr) {
	if ((cmd = print_lines("\
\n\
y	Do this newsgroup now.\n\
SP	Do this newsgroup, executing the default command listed in []'s.\n\
.cmd	Do this newsgroup, executing cmd as first command.\n\
+	Enter this newsgroup through the selector (like typing .+<CR>).\n\
=	Start this newsgroup, but list subjects before reading articles.\n\
U	Enter this newsgroup by way of the \"Set unread?\" prompt.\n\
",NOMARKING)) ||
#ifdef SCAN
    (cmd = print_lines("\
;	Enter article-scan mode for this newsgroup.\n\
",NOMARKING)) ||
#endif
    (cmd = print_lines("\
u	Unsubscribe from this newsgroup.\n\
^N	Switch to next news source (the numbered GROUPs in ~/.trn/access).\n\
^P	Switch to the previous news source.\n\
",NOMARKING)) )
	    return cmd;
    }
    if ((cmd = print_lines("\
t	Toggle the newsgroup between threaded and unthreaded reading.\n\
c	Catch up (mark all articles as read).\n\
A	Abandon read/unread changes to this newsgroup since you started trn.\n\
n	Go to the next newsgroup with unread news.\n\
N	Go to the next newsgroup.\n\
p	Go to the previous newsgroup with unread news.\n\
P	Go to the previous newsgroup.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
-	Go to the previously displayed newsgroup.\n\
1	Go to the first newsgroup.\n\
^	Go to the first newsgroup with unread news.\n\
$	Go to the end of newsgroups.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
g name	Go to the named newsgroup.  Subscribe to new newsgroups this way too.\n\
/pat	Search forward for newsgroup matching pattern.\n\
?pat	Search backward for newsgroup matching pattern.\n\
	(Use * and ? style patterns.  Append r to include read newsgroups.)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
l pat	List unsubscribed newsgroups containing pattern.\n\
m name	Move named newsgroup elsewhere (no name moves current newsgroup).\n\
o pat	Only display newsgroups matching pattern.  Omit pat to unrestrict.\n\
O pat	Like o, but skip empty groups.\n\
a pat	Like o, but also scans for unsubscribed newsgroups matching pattern.\n\
L	List current .newsrc.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
&	Start the option selector.\n\
&switch {switch}\n\
	Set (or unset) more command-line switches.\n\
&&	Print current macro definitions.\n\
&&def	Define a new macro.\n\
!cmd	Shell escape.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
`	switch to the newsgroup selector.\n\
q	Quit trn.\n\
x	Quit, restoring .newsrc to its state at startup of trn.\n\
^K	Edit the global KILL file.  Use commands like /pattern/j to suppress\n\
	pattern in every newsgroup.\n\
v	Print version and the address for reporting bugs.\n\
",NOMARKING)) )
	return cmd;
#endif
    if ((cmd = get_anything()) != 0)
	return cmd;
    show_macros();
    return 0;
}

int
help_ngsel()
{
    int cmd;
    page_start();
    if ((cmd = print_lines("\
The Newsgroup Selector:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
a-z,0-9	Select/deselect the indicated group by its letter or number.  Many of\n\
	the alpha letters are omitted for the following commands.\n\
SP	Perform the default command (usually > or Z).\n\
CR	Enter newsgroup(s).  Selects the current group if nothing is selected.\n\
.	Toggle the current group's selection.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
k, ','	Mark the current group to kill all unread articles.\n\
-	Set a range, as in d - f.  Repeats the last marking action.\n\
@	Toggle the selection of all visible groups.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
E	Toggle exclusion of non-selected groups from the selection list.\n\
n, ]	Move down to the next group (try down-arrow also).\n\
p, [	Move up to the previous group (try up-arrow also).\n\
<, >	Go to previous/next page (try left-/right-arrow also).\n\
^, $	Go to first/last page.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
O	Set the selector's order.\n\
R	Reverse the current sort order.\n\
U	Switch between selecting empty/non-empty newsgroups.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
c	Catch up a group, marking the articles as read without chasing xrefs.\n\
=	Refresh the article counts in the selector (will re-grab active).\n\
^N	Switch to next news source (the numbered GROUPs in ~/.trn/access).\n\
^P	Switch to the previous news source.\n\
^K	Edit the global KILL file.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
\\	Escape next char as a command. A selector command is tried first,\n\
	then a newsgroup-list command (force the latter with another \\).\n\
	E.g. \\a will add groups, \\\\h gets you newsgroup-list help.\n\
:cmd	Perform a command on all the selected groups.\n\
::cmd	Perform a command on all non-selected groups.\n\
:.cmd	Perform a command on the current group.\n\
::.cmd	Perform a command on the all but the current.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
/pattern/modifiers:command{:command}\n\
	Apply one or more commands to the set of groups matching pattern.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
&	Start the option selector or set command-line switches.\n\
&&	View or set macro definitions.\n\
!cmd	Escape to a subshell.\n\
H	This help message.\n\
h, ?	Enter online help browser.\n\
`, ESC	Leave the selector and return to the rn-style group selector.\n\
q	Quit trn.\n\
Q	Quit the selector and return to the rn-style group selector.\n\
",NOMARKING)) )
	return cmd;
    return 0;
}

int
help_addsel()
{
    int cmd;
    page_start();
    if ((cmd = print_lines("\
The Add-Newsgroups Selector:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
a-z,0-9	Select/deselect the newsgroup by its letter or number.  Many of\n\
	the alpha letters are omitted for the following commands.\n\
SP	Perform the default command (usually > or Z).\n\
Z,CR	Add the selected groups to the end of your newsrc.\n\
.	Toggle the current group's selection.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
k, ','	Mark the current group as undesirable for subscribing.\n\
-	Set a range, as in d - f.  Repeats the last marking action.\n\
@	Toggle the selection of all visible groups.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
E	Toggle exclusion of non-selected groups from the selection list.\n\
n, ]	Move down to the next group (try down-arrow also).\n\
p, [	Move up to the previous group (try up-arrow also).\n\
<, >	Go to previous/next page (try left-/right-arrow also).\n\
^, $	Go to first/last page.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
O	Set the selector's order (NOT WORKING YET).\n\
R	Reverse the current sort order (NOT WORKING YET).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
/pattern/modifiers:commands\n\
	Scan all newsgroups for a name matching the pattern.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
&	Start the option selector or set command-line switches.\n\
&&	View or set macro definitions.\n\
!cmd	Escape to a subshell.\n\
H	This help message.\n\
h, ?	Enter online help browser.\n\
q	Quit the selector.\n\
",NOMARKING)) )
	return cmd;
    return 0;
}

#ifdef ESCSUBS
int
help_subs()
{
    int cmd;
#ifdef SUBSHELP
    doshell(sh,filexp(SUBSHELP));
#else
    page_start();
    if ((cmd = print_lines("\
Valid substitutions are:\n\
",STANDOUT)) ||
    (cmd = print_lines("\
\n\
a	Current article number\n\
A	Full name of current article (%P/%c/%a)\n\
b	Destination of last save command, often a mailbox\n\
B	Bytes to ignore at beginning of last saved article\n\
",NOMARKING)) ||
    (cmd = print_lines("\
c	Current newsgroup, directory form\n\
C	Current newsgroup, dot form\n\
d	Full name of newsgroup directory (%P/%c)\n\
D	Distribution line from current article\n\
e	The last command executed to extract data from an article\n\
E	The last extraction directory\n\
",NOMARKING)) ||
    (cmd = print_lines("\
f	Who the current article is from\n\
F	Newsgroups to followup to (from Newsgroups and Followup-To)\n\
h	(This help message)\n\
H	Host name (yours)\n\
i	Message-I.D. line from current article, with <>\n\
I	Reference indicator mark (see -F switch)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
l	News administrator's login name, if any\n\
L	Login name (yours)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
m	Current mode, first letter of (init,newsgroup,thread,article,pager,\n\
		unread,Add,Catchup,Delete-bogus,Mailbox,Resubscribe)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
M	Number of article marked with M\n\
n	Newsgroups from current article\n\
N	Full name (yours)\n\
o	Organization (yours)\n\
O	Original working directory (where you ran trn from)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
p	Your private news directory (from -d)\n\
P	Public news spool directory\n\
r	Last reference (parent article id)\n\
R	References list for followup article\n\
",NOMARKING)) ||
    (cmd = print_lines("\
s	Subject, with all Re's and (nf)'s stripped off\n\
S	Subject, with one Re stripped off\n\
t	New To line derived from From and Reply-To (Internet format)\n\
T	New To line derived from Path\n\
u	Number of unread articles\n\
",NOMARKING)) ||
    (cmd = print_lines("\
U	Number of unread articles not counting the current article (when\n\
	threads are selected, the count only reflects selected articles)\n\
v	The number of extra (unselected) articles, not counting the current\n\
	one if it is unselected\n\
W	Where thread files are saved\n\
x	News library directory\n\
X	Trn library directory\n\
",NOMARKING)) ||
#ifdef SCORE
    (cmd = print_lines("\
y	From line with the domain abbreviated.\n\
	(foo@x.y.bar.edu (Foo's Bar) -> foo@*.bar.edu (Foo's Bar))\n\
",NOMARKING)) ||
#endif
    (cmd = print_lines("\
Y	The current tmp dir (usually /tmp)\n\
z	Length of current article in bytes\n\
Z	Number of selected threads\n\
",NOMARKING)) ||
    (cmd = print_lines("\
~	Your home directory\n\
.	Directory containing the \"dot\" files, such as .newsrc\n\
+	Your trn config file directory (usually %./.trn)\n\
#	A counter in multi-article saves\n\
$	Current process number\n\
/	Last search string\n\
ESC	Run preceding command through % interpretation\n\
",NOMARKING)) ||
    (cmd = print_lines("\
Put ^ in the middle to capitalize the first letter: %^C = Rec.humor\n\
Put _ in the middle to capitalize the last component: %_c = rec/Humor\n\
Put \\ in the middle to quote regexp and % characters in the resulting string\n\
Put :FMT in the middle to format the result printf-style:  %:-30.30t\n\
",NOMARKING)) )
	return cmd;
#endif
    return 0;
}
#endif

int
help_artsel()
{
    int cmd;
    page_start();
    if ((cmd = print_lines("\
Thread/Subject/Article Selector Commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
a-z,0-9	Select/deselect the indicated item by its letter or number.  Many of\n\
	the alpha letters are omitted for the following commands.\n\
SP	Perform the default command (usually > or Z).\n\
CR	Start reading.  Selects the current item if nothing is selected.\n\
Z,TAB	Start reading.  If nothing is selected, read all unread articles.\n\
.	Toggle the current item's selection.\n\
*	Same as '.' except that it affects all items with the same subject.\n\
",NOMARKING)) ||
#ifdef SCAN
    (cmd = print_lines("\
;	Enter article-scan mode.  If articles/subjects/threads were selected\n\
	in the trn selector, article-scan will show only those articles.\n\
",NOMARKING)) ||
#endif
    (cmd = print_lines("\
#	Read the current item only, temporarily ignoring all other selections.\n\
k, ','	Mark the current item as killed.\n\
m, |	Unmark the current item.\n\
-	Set a range, as in d - f.  Repeats the last marking action.\n\
@	Toggle the selection of all visible items.\n\
M	Mark the current item's article(s) as to-return and kill the item.\n\
Y	Yank back and select articles marked to return via M.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
E	Toggle exclusion of non-selected items from the selection list.\n\
n, ]	Move down to the next item (try down-arrow also).\n\
p, [	Move up to the previous item (try up-arrow also).\n\
<, >	Go to previous/next page (try left-/right-arrow also).\n\
^, $	Go to first/last page.\n\
S	Set what the selector displays:  threads, subjects, or articles.\n\
	If the group is unthreaded, choosing threads will thread it.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
=	Toggle between the article and thread/subject selector.\n\
O	Set the selector's order.  A separate default is kept for the\n\
	article and subject/thread selector.\n\
R	Reverse the current sort order.\n\
L	Switch the display between a short style without authors and a\n\
	medium or long style with authors.\n\
U	Switch between selecting unread/read articles.\n\
X	Mark all unselected articles as read and start reading.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
D	Mark unselected articles on the current page as read.  Start\n\
	reading if articles were selected, else go to next page.\n\
J	Junk all selected articles (mark them as read).\n\
c	Catch up -- marks ALL articles as read without chasing xrefs.\n\
A	Add current subject to memorized commands (selection or killing).\n\
T	Add current thread to memorized commands (selection or killing).\n\
^K	Edit local KILL file (the one for this newsgroup).\n\
N	Leave this group as-is and go on to the next one.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
P	Leave this group as-is and go on to the previous one.\n\
:cmd	Perform a command on all the selected articles (use :p to post).\n\
::cmd	Perform a command on all non-selected articles.\n\
:.cmd	Perform a command on the current thread or its selected articles.\n\
::.cmd	Perform a command on the unselected articles in the current thread.\n\
/pattern/modifiers\n\
	Scan all articles for a subject containing pattern.\n\
	(Append f to scan the from line, h to scan whole headers, a to scan\n\
",NOMARKING)) ||
    (cmd = print_lines("\
	entire articles, c to make it case-sensitive, r to scan read articles\n\
	(assumed when you are selecting read articles to set unread.)\n\
/pattern/modifiers:command{:command}\n\
	Apply one or more commands to the set of articles matching pattern.\n\
	Use a K modifier to save entire command to the KILL file for this\n\
	newsgroup.  Commands m and M, if first, imply an r modifier.\n\
 	Valid commands are: e, E, j, m, M, s, S, t, T, !, =, ',' and the\n\
	article/thread (de)selection commands: +/++ (-/--).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
&	Start the option selector or set command-line switches.\n\
&&	View or set macro definitions.\n\
!cmd	Escape to a subshell.\n\
H	This help message.\n\
h, ?	Enter online help browser.\n\
ESC, +	Leave the selector but stay in the group (at last visited article).\n\
q	Quit the selector and the group.\n\
Q	Quit group and return to news group selection prompt for this group.\n\
",NOMARKING)) )
	return cmd;
    return 0;
}

int
help_multirc()
{
    int cmd;
    page_start();
    if ((cmd = print_lines("\
The Newsrc Selector:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
a-z,0-9	Select/deselect the indicated item by its letter or number.  Many of\n\
	the alpha letters are omitted for the following commands.\n\
SP	Perform the default command (usually > or Z).\n\
CR	Open the newsrc(s).  Selects the current item if nothing is selected.\n\
.	Toggle the current item's selection.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
-	Set a range, as in d - f.  Repeats the last marking action.\n\
@	Toggle the selection of all visible items.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
E	Toggle exclusion of non-selected items from the selection list.\n\
n, ]	Move down to the next item (try down-arrow also).\n\
p, [	Move up to the previous item (try up-arrow also).\n\
<, >	Go to previous/next page (try left-/right-arrow also).\n\
^, $	Go to first/last page.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
R	Reverse the current sort order.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
&	Start the option selector or set command-line switches.\n\
&&	View or set macro definitions.\n\
!cmd	Escape to a subshell.\n\
H	This help message.\n\
h, ?	Enter online help browser.\n\
q	Quit trn.\n\
",NOMARKING)) )
	return cmd;
    return 0;
}

int
help_options()
{
    int cmd;
    page_start();
    if ((cmd = print_lines("\
The Option Selector:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
a-z,0-9	Select/deselect the indicated item by its letter or number.  Many of\n\
	the alpha letters are omitted for the following commands.\n\
SP	Perform the default command (usually > or Z).\n\
CR	Accept changes.  Changes the current item if nothing is selected.\n\
Z,TAB	Accept changes.\n\
.	Toggle the current item's selection.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
-	Set a range, as in d - f.  Repeats the last marking action.\n\
@	Toggle the selection of all visible items.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
E	Toggle exclusion of non-selected items from the selection list.\n\
n, ]	Move down to the next item (try down-arrow also).\n\
p, [	Move up to the previous item (try up-arrow also).\n\
<, >	Go to previous/next page (try left-/right-arrow also).\n\
^, $	Go to first/last page.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
R	Reverse the current sort order.\n\
&	Start the option selector or set command-line switches.\n\
&&	View or set macro definitions.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
!cmd	Escape to a subshell.\n\
H	This help message.\n\
h, ?	Enter online help browser.\n\
q	Quit the option selector, abandoning changes.\n\
\n\
",NOMARKING)) ||
    (cmd = print_lines("\
	The options are as follows:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
Use Universal Selector........ yes/no\n\
Universal Selector Order...... natural/points\n\
Universal Selector art. follow yes/no\n\
Universal Selector Commands... [Last-page-cmd][Other-page-cmd]\n\
Use Newsrc Selector........... yes/no\n\
Newsrc Selector Commands...... [Last-page-cmd][Other-page-cmd]\n\
Use Addgroup Selector......... yes/no\n\
Addgroup Selector Commands.... [Last-page-cmd][Other-page-cmd]\n\
Use Newsgroup Selector........ yes/no\n\
Newsgroup Selector Order...... natural/group/count\n\
Newsgroup Selector Commands... [Last-page-cmd][Other-page-cmd]\n\
Use News Selector............. yes/no/[# articles]\n\
",NOMARKING)) ||
    (cmd = print_lines("\
News Selector Mode............ threads/subjects/articles\n\
News Selector Order........... date/subject/author/groups/count/points\n\
News Selector Commands........ [Last-page-cmd][Other-page-cmd]\n\
News Selector Display Styles.. [e.g. lms=long/medium/short]\n\
Option Selector Commands...... [Last-page-cmd][Other-page-cmd]\n\
Use Selector Numbers.......... yes/no\n\
Selector Number Auto-Goto..... yes/no\n\
Use Threads................... yes/no\n\
Cited Text String............. [e.g. '>']\n\
Select My Postings............ parent/subthread/no\n\
",NOMARKING)) ||
    (cmd = print_lines("\
Read Breadth First............ yes/no\n\
Join Subject Lines............ no/[# chars]\n\
Terse Output.................. yes/no\n\
Check for New Groups.......... yes/no\n\
Initial Group List............ [# groups]\n\
Background Threading.......... yes/no\n\
Background Spinner............ yes/no\n\
Compress Subjects............. yes/no\n\
",NOMARKING)) ||
    (cmd = print_lines("\
Auto Savename................. yes/no\n\
Default Savefile Type......... norm/mail/ask\n\
Fuzzy Newsgroup Names......... yes/no\n\
Auto-Grow Groups.............. yes/no\n\
Ignore THRU on Select......... yes/no\n\
Auto Arrow Macros............. yes/no\n\
Save Dir...................... [directory path]\n\
Pager Line-Marking............ standout/underline/no\n\
",NOMARKING)) ||
    (cmd = print_lines("\
Erase Screen.................. yes/no\n\
Erase Each Line............... yes/no\n\
Muck Up Clear................. yes/no\n\
Novice Delays................. yes/no\n\
Header Magic.................. [[!]header,...]\n\
Header Hiding................. [[!]header,...]\n\
Initial Article Lines......... no/[# lines]\n\
Article Tree Lines............ no/[# lines]\n\
Append Unsubscribed Groups.... yes/no\n\
",NOMARKING)) ||
    (cmd = print_lines("\
Line Num for Goto............. [# line (1-n)]\n\
Verify Input.................. yes/no\n\
Eat Type-Ahead................ yes/no\n\
Charset....................... [e.g. patm]\n\
Filter Control Characters..... yes/no\n\
Scan Mode Count............... no/[# articles]\n\
Old Mthreads Database......... yes/no\n\
Checkpoint Newsrc Frequency... [# articles]\n\
Active File Refetch Mins...... no/[# mins]\n\
",NOMARKING)) )
	return cmd;
    return 0;
}

#ifdef SCAN
int
help_scanart()
{
    int cmd;
#ifdef UNDEF
/* implement this later? */
    doshell(sh,filexp(SAHELP));
#else
    page_start();
    if ((cmd = print_lines("\
Article scan commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\
SP,CR	Display article and enter the Article Level.\n\
	Note: Marked articles (see 'm') will be displayed first.\n\
r	Read the pointed-to article first, then read marked articles.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
[0-9]	Jump to item number.  Enter a second digit or a command to jump\n\
	to the item.\n\
n, ]	Move down to the next item (try down-arrow also).\n\
p, [	Move up to the previous item (try up-arrow also).\n\
^N	Move down to next article with the  same subject.\n\
^P	Move up to previous article with the same subject.\n\
>	Display next page of articles.\n\
<	Display previous page of articles.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
t	Move to top of current page.\n\
b	Move to bottom of current page.\n\
T,^	Move to top of all articles.\n\
B,$	Move to bottom of all articles.\n\
N	Go to next newsgroup.\n\
P	Go to previous newsgroup.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
^L,^R	Redraw screen.\n\
j	Junk the article pointed to.\n\
k	Junk all articles (after pointer) with same subject.\n\
	(much the same as 'k' at the Article Selection Level.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
m	Mark or unmark article for reading or a later command.\n\
M	Toggle mark on following articles with same subject.\n\
	(This is the same as typing 'm' on each of them.)\n\
X	Junk (mark as read) all unmarked articles.\n\
D	Junk unmarked articles on the same page. (similar to 'X')\n\
	['X' and 'D' act like the TRN thread selector options.]\n\
J	Junk marked articles on page.  (opposite of 'D')\n\
G	Goto an article by number.\n\
e	Extract marked articles (in mime, shar, or uuencoded format) to a\n\
	directory.  Multipart files may be extracted in any order.\n\
E	End/Examine the last extracted file.  A command can be entered to\n\
	examine/view/play the extracted file.  If the file was incomplete,\n\
	it can be removed.\n\
	[The e and E commands are disabled at the moment.]\n\
",NOMARKING)) ||
    (cmd = print_lines("\
s	Select (for zoom) this article.\n\
S	Select (for zoom) all following articles with same subject.\n\
z	Enter/exit \"zoom\" mode--only selected articles are displayed.\n\
	(Zoom mode is automatically entered when article-scan mode is\n\
	entered from the TRN thread selector.)\n\
Z	Clear (Zero) the zoom selections\n\
	[Note: when in zoom mode commands which delete articles will\n\
	only delete selected articles.  This means, for instance, that\n\
	'X' will only delete *selected* unmarked articles.  (Useful for\n\
	reading only a few articles from a thread.)]\n\
",NOMARKING)) ||
    (cmd = print_lines("\
+	Enter thread selection mode (only if group is threaded).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
!cmd	Escape to a subshell and execute cmd.\n\
&	Print current values of command-line switches.\n\
&switch {switch}\n\
	Set or unset more switches.\n\
&&	Print current macro definitions.\n\
&&def	Define a new macro.\n\
h	Print this help.\n\
H	Enter help scan mode (an online documentation browser).\n\
q	Quit reading this newsgroup and go to the next one.\n\
Q	Leave article scan mode and begin reading articles, starting\n\
	with the article pointed to.  You will not return to\n\
	article-scan mode unless you type ';' again.\n\
c	Catch up on all articles in the current newsgroup.\n\
	(All articles are marked as read.)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
/text	Search article descriptions forwards for text.\n\
?text	Search article descriptions backwards for text.\n\
gtext   Search article descriptions forwards for text.  If no matching\n\
	descriptions are found, continue the search from the first\n\
	article description.  (wrap-around search)\n\
	Searches are for case-insensitive substring of article\n\
	descriptions matching text.  Leading spaces in text are ignored\n\
	(which means you can type \"g John Smith\" or \"gJohn Smith\")\n\
",NOMARKING)) ||
    (cmd = print_lines("\
f	Toggle subject following mode.  When enabled article threads\n\
	will be followed normally.  When disabled typing space or\n\
	return at end of article will return to article-scan mode.\n\
	Typing a command explicitly (like ^n) will still work.\n\
	(If an explicit command is typed, follow mode will be temporarily\n\
	turned on for the remainder of the thread.)\n\
F	Toggle subject Folding mode.  (show only first article with a\n\
	subject.  Note that \"first\" depends on ordering...)\n\
",NOMARKING)) ||
    (cmd = print_lines("\
U	Toggle reading eligibility: read / unread+read\n\
TAB	Toggle display of \"threadcount\" (# of articles following this one\n\
	with the same subject)\n\
a	Toggle display of authors.\n\
%%	Toggle display of article numbers.\n\
",NOMARKING))
#ifdef SCORE
    || (cmd = print_lines("\
Scoring commands:\n\
",STANDOUT)) ||
    (cmd = print_lines("\
K(num)	Kill articles with scores equal to or below a number (num).\n\
	Type the threshold and press return.  (no spaces: type K-12)\n\
^E	Edit the score file for the current group.\n\
o	Toggle article ordering: arrival or score-sorted.\n\
O	Toggle article ordering between older-first and newer-first\n\
	when in score-sorted order.  (For two articles with same\n\
	score, toggle between the older one first or the newer one first.)\n\
R	Rescores all articles.  (same as 'r at article level).\n\
",NOMARKING)) ||
    (cmd = print_lines("\
[Note: type ' or \" then RETURN for menus of scoring commands.]\n\
\"\" <text>\n\
	Adds <text> to the local (this newsgroup's) scorefile.\n\
\"* <text>\n\
	Adds <text> to the global scorefile.\n\
\"C <text>\n\
	Adds <text> to the scorefile represented by the abbreviation C.\n\
\"?	List all scorefile abbreviations and the filenames they represent.\n\
\"! <text>\n\
	Adds <text> to the internal list of scoring rules as if it was read\n\
	from a file.  Articles are rescored after this command.\n\
'd	Debugging dump.  (Will go away in later versions.)\n\
'e <abbreviation OR filename>\n\
	Edits the scorefile named by the following file abbreviation\n\
	character or full filename.  If no arguments are given, edits the\n\
	local scorefile (the one for the current newsgroup).\n\
'f	Continue scoring articles until interrupted.\n\
'r	Discard old scoring data and rescore from score files.\n\
's	Show scoring rules which contributed to this article's score.\n\
't	Test command: may do varying things.  (will usually be disabled)\n\
",NOMARKING))
#endif /* SCORE */
    )
	return cmd;
#endif /* !UNDEF (later the !doshell part) */
    return 0;
}
#endif /* SCAN */

int
help_univ()
{
    int cmd;
    page_start();
    if ((cmd = print_lines("\
The Universal Selector:\n\
",STANDOUT)) ||
    (cmd = print_lines("\n\
a-z,0-9	Select/deselect the indicated item by its letter or number.  Many of\n\
	the alpha letters are omitted for the following commands.\n\
SP	Perform the default command (usually > or Z).\n\
CR	Enter item(s).  Selects the current item if nothing is selected.\n\
TAB	Start reading selected items.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
-	Set a range, as in d - f.  Repeats the last marking action.\n\
@	Toggle the selection of all visible items.\n\
.	Toggle the current item's selection.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
E	Toggle exclusion of non-selected items from the selection list.\n\
n, ]	Move down to the next item (try down-arrow also).\n\
p, [	Move up to the previous item (try up-arrow also).\n\
<, >	Go to previous/next page (try left-/right-arrow also).\n\
^, $	Go to first/last page.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
O	Set the selector's order.\n\
R	Reverse the current sort order.\n\
U	Switch between selecting empty/non-empty items.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
^E	Edit the current universal configuration file.\n\
",NOMARKING)) ||
    (cmd = print_lines("\
&	Start the option selector or set command-line switches.\n\
&&	View or set macro definitions.\n\
!cmd	Escape to a subshell.\n\
H	This help message.\n\
h, ?	Enter online help browser.\n\
q	Quit current level.  At top level, leave trn.\n\
Q	Quit the current level.  At top level, go to the next selector.\n\
",NOMARKING)) )
	return cmd;
    return 0;
}

int
univ_key_help(where)
int where;
{
    switch (where) {
      case UHELP_PAGE:
	return help_page();
      case UHELP_ART:
	return help_art();
      case UHELP_NG:
	return help_ng();
      case UHELP_NGSEL:
	return help_ngsel();
      case UHELP_ADDSEL:
	return help_addsel();
#ifdef ESCSUBS
      case UHELP_SUBS:
	return help_subs();
#endif
      case UHELP_ARTSEL:
	return help_artsel();
      case UHELP_MULTIRC:
	return help_multirc();
      case UHELP_OPTIONS:
	return help_options();
#ifdef SCAN
      case UHELP_SCANART:
	return help_scanart();
#endif
      case UHELP_UNIV:
	return help_univ();
      default:
	return 0;
    }
}
