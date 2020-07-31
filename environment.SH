case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting HelpFiles/config/environment (with variable substitutions)"
if test "X$src" != "X." ; then
    for ff in `$grep '^HelpFiles/' $src/MANIFEST | sed -e 's/[	 ].*//'`; do
	./makedir $ff 1
	cp $src/$ff $ff
    done
fi
$spitshell >HelpFiles/config/environment <<!GROK!THIS!
In this list, variables marked with (%) are subject to percent interpolation,
and variables marked with (~) are subject to both percent and home-directory
interpolation.

Valid substitutions for strings marked with % are:

      %a      Current article number
      %A      Full name of current article (%P/%c/%a)
      %b      Destination of a save command, a mailbox or command
      %B      The byte offset to the beginning of the article for saves
              with or without the header.  The 's' and 'S' commands set
              it to 0, and the 'w' and 'W' commands set it to the byte
              offset of the body of the article.
      %c      Current newsgroup, directory form
      %C      Current newsgroup, dot form
      %d      Full name of newsgroup directory (%P/%c)
      %D      Distribution: line from the current article
      %e      Extract program
      %E      Extract destination directory
      %f      From: line or Reply-To: line from the current article
      %F      Newsgroups to followup to from Newsgroups: and Followup-To:
      %h      Name of header file to pass to mail or news poster
      %H      Host name (yours)
      %i      Message-I.D.: line from current article, with <>
      %j      The terminal speed (e.g. 9600)
      %I      Inclusion indicator
      %l      News administrator login name
      %L      Login name (yours)
      %m      The current mode of trn.
                  i    Initializing.
                  n    Newsgroup-list level.
                  f    End (finis) of newsgroup-list level.
                  t    The thread/subject/article selector.
                  c    Newsrc selector.
                  w    Newsgroup selector.
                  j    Addgroup selector.
                  l    Option selector.
                  a    Article level ("What next?").
                  e    End of the article level.
                  p    Pager level ("MORE" prompt).
                  u    Unkill prompt.
                  d    Selector mode prompt.
                  o    Selector order prompt.
                  m    Memorize thread command prompt.
                  r    Memorize subject command prompt.
                  z    Option edit prompt.
                  k    Processing memorized (KILL-file) commands.
                  A    Add this newsgroup?
                  B    Abandon confirmation.
                  C    Catchup confirmation.
                  D    Delete bogus newsgroups?
                  F    Is follow-up a new topic?
                  M    Use mailbox format?
                  R    Resubscribe to this newsgroup?
                  K    Press any key prompt.
      %M      Number of articles marked with M
      %n      Newsgroups from source article
      %N      Full name (yours)
      %o      Organization (yours)
      %O      Original working directory (where you ran trn from)
      %p      Your private news directory (-d switch)
      %P      Public news spool directory
      %r      Last reference (parent article id)
      %q      The last quoted input (via %").
      %R      References list for a new article
      %s      Subject, with all Re's and (nf)'s stripped off
      %S      Subject, with one Re stripped off
      %t      New To: line derived from From: and Reply-To (Internet always)
      %T      New To: line derived from Path:
      %u      Number of unread articles
      %U      Number of unread articles disregarding current article
      %v      Number of unselected articles disregarding current article
      %W      The thread directory root
      %x      News library directory, usually /usr/lib/news
      %X      Trn's private library directory, usually %x/trn
      %y      From line with domain shortening (name@*.domain.nam)
      %Y      The tmp directory to use
      %z      Size of current article in bytes.
      %Z      Number of selected threads.
      %~      Home directory (yours)
      %.      Directory containing dotfiles, usually %~
      %+      Directory containing a user's init files, usually %./.trn
      %#      count of articles saved in current command (from 1 to n)
      %^#     ever-increasing number (from 1 to n)
      %\$     current process number
      %{name} Environment variable "name".  %{name-default} form allowed.
      %<name> Like %{name}, but variable "name" will itself be subject to
              interpolation before being used.
      %[name] Header line beginning with "Name: ", without "Name: "
      %"prompt"
              Print prompt and insert what is typed.
      %\`command\`
              Insert output of command.
      %(test_text=pattern?if_text:else_text)
              Substitute if_text if test_text matches pattern, otherwise
              substitute else_text.  Use != for negated match.
              % substitutions are done on test_text, if_text, and else_text.
      %digit  Substitute the text matched by the nth bracket in the last
              pattern that had brackets.  %0 matches the last bracket
              matched, in case you had alternatives.
      %?      Insert a space unless the entire result is > 79 chars, in
              which case the space becomes a newline.

      Put ^ in the middle to capitalize the first letter: %^C = Rec.humor
      Put _ in the middle to capitalize last component: %_c = net/Jokes
      Put \ in the middle to quote regexp and % characters in the result
      Put > in the middle to return the address portion of a name.
      Put ) in the middle to return the comment portion of a name.
      Put ' in the middle to protect "'"s in arguments you've put in "'"s.
      Put :FMT in the middle to format the result: %:-30.30t

      ~ interpretation in filename expansion happens after % expansion, so
      you could put ~%{NEWSLOGNAME-news} and it will expand correctly.


The variables:

 ATTRIBUTION (%)
      Gives the format of the attribution line in front of the quoted
      article included by an F (followup) command.

      Default: In article %i,%?%)f <%>f> wrote:

 AUTHORCOPY
      If defined, contains the name of a file to which Pnews will append
      finished articles.

      Default: Article is not saved.

 AUTOSUBSCRIBE
      When trn is checking for new newsgroups and finds one matching one
      of the patterns in AUTOSUBSCRIBE, the new group is automatically
      added to the end of the .newsrc, subscribed.   Newsgroups not
      matching this  or AUTOUNSUBSCRIBE, below, are offered to the user.

      AUTOSUBSCRIBE is a comma separated list of newsgroup patterns ala
      'o', '/', etc.  It can also include "but not" entries preceded by
      '!'.  "a,b,!c,d" is read as "matching a or b, unless it also matches
      c; matching d regardless".  Another way to look at it is "(((a or b)
      and not c) or d)".  To automatically subscribe to all local groups
      but be choosy about non-local groups, one might say "*,!*.*".

      Default: (none)

 AUTOUNSUBSCRIBE
      AUTOUNSUBSCRIBE is very similar to AUTOSUBSCRIBE, above, but new
      newsgroups matching it are automatically added to the end of the
      .newsrc file, unsubscribed.  If a newsgroup matches AUTOSUBSCRIBE,
      AUTOUNSUBSCRIBE is not consulted.

      Default: (none)

 CANCEL (~)
      The shell command used to cancel an article.

      Default: inews -h < %h

 CANCELHEADER (%)
      The format of the file to pass to the CANCEL command in order to
      cancel an article.

      Default:

      Newsgroups: %n
      Subject: cancel
      %(%{FROM}=^$?:From: %{FROM}
      )Control: cancel %i
      Distribution: %D

      %i was cancelled from within trn.

 DOTDIR (%)
      Where to find your dot files, if they aren't in your home directory.

      Default: \$HOME

 EDITOR (~)
      The name of your editor, if VISUAL is undefined.

      Default: $defeditor

 EXSAVER (%)
      The shell command to execute in order to extract data to either
      /bin/sh or a user-specified command.

      Default: tail +%Bc %A | %e

 FIRSTLINE (%)
      Controls the format of the line displayed at the top of an article.
      Warning: this may go away.

      The default (ignoring the Marked to return display in unthreaded
      groups) is approximately:

      %C #%a%(%Z=^0\$?%(%U!=^0\$?  (%U  more\)):  (%U  +  %v more\))

 FORWARDHEADER (%)
      The format of the header file for forwarding messages.  If these
      headers contain a Content-Type header with a multipart/* type,
      Rnmail will attempt to forward the message MIME-encapsulated.
      This may fail if you are using a non-standard FORWARDPOSTER
      which does not pass on MIME headers properly.  See also
      FORWARDPOSTER.

      Default:

      To: %"\n\nTo: "
      Subject: %(%i=^\$?:%[subject] (fwd\\)
      %(%{FROM}=^\$?:From: %{FROM}
      )%(%{REPLYTO}=^\$?:Reply-To: %{REPLYTO}
      )X-Newsgroups: %n
      In-Reply-To: %i)
      %(%[references]=^\$?:References: %[references]
      )Organization: %o
      Mime-Version: 1.0
      Content-Type: multipart/mixed; boundary=\"=%\$%^#=--\"
      Cc: 
      Bcc: \n\n

 FORWARDMSG (%)
      The line added to the message body when forwarding a message,
      signifying the beginning of the forwarded message.  This is
      only added if the message is not being forwarded as a MIME
      attachment (see FORWARDHEADER).

      Default: ------- start of forwarded message -------

 FORWARDMSGEND (%)
      The line added to the message body when forwarding a message,
      signifying the end of the forwarded message.  This is only
      added if the message is not being forwarded as a MIME 
      attachment (see FORWARDHEADER).

      Default: ------- end of forwarded message -------

 FORWARDPOSTER (~)
      The shell command to be used by the forward command (^F) in
      order to allow you to edit and deliver the file.  trn will not
      itself call upon an editor for replies -- this is a function
      of the program referenced by FORWARDPOSTER.  See also
      FORWARDHEADER and MAILPOSTER.

      Default: Rnmail -h %h

 FROM (%)
      What to put in the From: header of your posts, email replies,
      and email forwards, instead of whatever the default name and
      address are for your system.  This will only work if you use
      the default setings for the NEWSHEADER, MAILHEADER, and
      FORWARDHEADER variables, or if your custom ones use FROM to
      set the From: header.

      Regardless of the settings of NEWSHEADER, MAILHEADER, and
      FORWARDHEADER, the setting of FROM is used to determine which
      articles may be cancelled or superseded.

      Default: undefined

 HELPPAGER
      The pager used by the online help system, if you want to use
      something different from the default or the content of the
      PAGER variable (see PAGER).

      Default: \$PAGER or 'more'.

 HIDELINE
      If defined, contains a regular expression which matches article
      lines to be hidden, in order, for instance, to suppress quoted
      material.  A recommended string for this purpose is "^>...",
      which doesn't hide lines with only '>', to give some indication
      that quoted material is being skipped.  If you want to hide more
      than one pattern, you can use "|" to separate the alternatives.
      You can view the hidden lines by restarting the article with the
      'v' command.

      There is some overhead involved in matching each line of the
      article against a regular expression.  You might wish to use a
      baud-rate modifier to enable this feature only at low baud rates.

      Default: undefined

 HOME
      Your home directory.  Affects ~ interpretation, and the location 
      of your dot files if DOTDIR is not defined.

      Default: \$LOGDIR

 KILLGLOBAL (~)
      Where to find the KILL file to apply to every newsgroup.  See the
      '^K' command at the newsgroup-selection level.

      Default: %p/KILL

 KILLLOCAL (~)
      Where to find the KILL file for the current newsgroup.  See the
      commands 'K' and '^K' at the article selection level, and the
      search modifier 'K'.

      Default: %p/%c/KILL

 LOGDIR
      Your home directory if HOME is undefined.  Affects ~ interpretation,
      and the location of your dot files if DOTDIR is not defined.

      Default: none.

      Explanation: you must have either \$HOME or \$LOGDIR.

 LOGNAME
      Your login name, if USER is undefined.  May be interpolated
      using "%L".

      Default: value of getlogin().

 LOCALTIMEFMT
      The format used by strftime() to print the local time.  The Date
      line is only displayed in local time if the group is threaded (see
      the -H option for more information on Date).

      Default: %a %b %e %X %Z %Y

      which is the same format as the date(1) command.

 MAILCALL (~)
      What to say when there is new mail.

      Default: (Mail)

 MAILCAPS
      A colon-separated search path for mailcap files.

      Default: %./.mimecap:~/.mailcap:$mimecap

 MAILFILE (~)
      Where to check for mail.

      Default: $mailfile

 MAILHEADER (%)
      The format of the header file for mail replies.  See also
      MAILPOSTER.

      Default:

      To: %t
      Subject: %(%i=^\$?:Re: %S
      X-Newsgroups: %n
      In-Reply-To: %i)
      %(%{FROM}=^\$?:From: %{FROM}
      )%(%{REPLYTO}=^\$?:Reply-To: %{REPLYTO}
      )%(%[references]=^\$?:References: %[references]
      )Organization: %o
      Cc: 
      Bcc: \n\n

 MAILPOSTER (~)
      The shell command to be used by the reply commands (r and R) in
      order to allow you to enter and deliver the response.  trn will
      not itself call upon an editor for replies -- this is a function
      of the program referenced by MAILPOSTER.  See also MAILHEADER.

      Default: Rnmail -h %h

 MBOXSAVER (~)
      The shell command to save an article in mailbox format.

      Default: %X/mbox.saver %A %P %c %a %B %C "%b" "From %t %\`date\`"

      Explanation: the first seven arguments are the same as for
      NORMSAVER.  The eighth argument to the shell script is the new
      From line for the article, including the posting date, derived
      either directly from the Posted: line, or not-so-directly from
      the Date: line.  Header munging at its finest.

 MODSTRING
      The string to insert in the group summary line, which heads each
      article, for a moderated group.  See also NOPOSTRING.

      Default: " (moderated)"

 NAME
      Your full name.  May be interpolated using "%N".

      Default: name from /etc/passwd, or ~/.fullname.

 NEWSHEADER (%)
      The format of the header file for follow-ups.  See also NEWSPOSTER.

      Default:

      %(%[followup-to]=^\$?:%(%[followup-to]=^%n\$?:X-ORIGINAL-NEWSGROUPS: %n
      ))Newsgroups: %(%F=^\$?%C:%F)
      Subject: %(%S=^\$?%\"\n\nSubject: \":Re: %S)
      Summary: 
      Expires: 
      %(%R=^\$?:References: %R
      )Sender: 
      Followup-To: 
      %(%{FROM}=^\$?:From: %{FROM}
      )%(%{REPLYTO}=^\$?:Reply-To: %{REPLYTO}
      )Distribution: %(%i=^\$?%\"Distribution: \":%D)
      Organization: %o
      Keywords: %[keywords]
      Cc: %(%F=poster?%t:%(%F!=@?:%F))\n\n

 NEWSORG
      Either the name of your organization, or the name of a file containing
      the name of your organization.  (For use at sites where the
      ORGANIZATION environmental variable is already in use.  NEWSORG will
      override ORGANIZATION if both are present.)  May be interpolated using
      "%o".

      Default: $orgname

 NEWSPOSTER (~)
      The shell command to be used by the follow-up commands (f and F) in
      order to allow you to enter and post a follow-up news article.  If
      not set, trn handles  the whole process and calls inews directly.
      See also NEWSHEADER.

      Default: Pnews -h %h

 NEWSSIGNATURE
      A signature file used by Pnews when generating news articles.
      Note that your inews may use a .signature file; you probably don't
      want to have both (it will result in two signatures being added to
      your articles).  Using NEWSSIGNATURE will cause your signature to
      appear in your editor when composing an article, while .signature
      will not.

      Default: \$DOTDIR/.news_sig.

 NNTPSERVER
      The hostname of your news server.  [This does not apply unless you
      are running the NNTP version of trn.]

      Default: $servername

 NO_ORIGINATOR
      If set, instructs inews not to add its Originator header.  The value
      is unimportant.

      Default: not set

 NOPOSTRING
      The string to insert in the group summary line, which heads each
      article, for a group to which local posting is not allowed.  See
      also MODSTRING.

      Default: " (no posting)"

 NORMSAVER (~)
      The shell command to save an article in the normal (non-mailbox)
      format.

      Default: %X/norm.saver %A %P %c %a %B %C "%b"

 ORGANIZATION
      Either the name of your organization, or the name of a file
      containing the name of your organization.  (If NEWSORG is set, it
      will override ORGANIZATION.)  May be interpolated using "%o".

      Default: $orgname

 PAGER
      The pager to be used by the internal help system.  See also
      HELPPAGER.

      Default: $pager

 PAGESTOP
      If defined, contains a regular expression which matches article
      lines to be treated as form-feeds.  There are at least two things
      you might want to do with this.  To cause page breaks between
      articles in a digest, you might define it as "^--------".   To
      force a page break before a signature, you could define it as
      "^-- \$".  (Then, when you see  "--" at the bottom of the page, you
      can skip the signature if you so desire by typing 'n' instead of
      space.)  To do both, you could use "^--".  If you want to break on
      more than one pattern, you can use "|" to separate the alternatives.

      There is some overhead involved in matching each line of the article
      against a regular expression.  You might wish to use a baud-rate
      modifier to enable this feature only at low baud rates.

      Default: undefined

 PIPESAVER (%)
      The shell command to execute in order to accomplish a save to a pipe
      ("s | command" or "w | command").  The command typed by the user is
      substituted in as %b.

      Default: %(%B=^0\$?<%A:tail +%Bc %A |) %b

      Explanation: if %B is 0, the command is "<%A  %b", otherwise the
      command is "tail +%Bc %A | %b".

 REPLYTO
      The value of the "Reply-To:" header, if needed.

 RNINIT
      This variable is used when initializing trn in rn-compatibility mode
      (see the -x switch) or when the TRNINIT variable isn't defined.  See
      the TRNINIT variable for a description.

 RNMACRO (~)
      The name of the file containing macros and key mappings when running
      trn as rn.  See also the TRNMACRO variable.

      Default: %./.rnmac

 SAVEDIR (~)
      The name of the directory to save to, if the save command does not
      specify a directory name.

      Default:
        If -/ is set: %p/%c
        If +/ is set: %p

 SAVENAME (%)
      The name of the file to save to, if the save command contains only a
      directory name.

      Default:
        If -/ is set: %a
        If +/ is set: %^C

 SAVESCOREFILE (~)
      A file where trn saves article scores, read when trn starts and
      written when trn exits.  If you do not want to save scores, you
      can use "savescores off" in a local or global scorefile.

      Default: %+/savedscores

 SCOREDIR (~)
      The directory where scorefiles are kept.

      Default: %+/scores

 SELECTCHARS
      The characters used by the thread selector to select the associated
      thread of discussion.  You can specify up to 64 visible characters,
      including upper-  and lower-case letters, numbers, and many
      punctuation characters.  Selection characters override command
      characters in the selector, but are not excluded from macro expansion,
      so be careful.

      Default: abdefgijlorstuvwxyz1234567890BCFGHIKMVW

      (You'll notice various characters are omitted to allow them to be
      typed as commands in the selector.)

 SHELL
      The name of your preferred shell.  It will be used by the '!', 'S'
      and 'W' commands.

      Default: $prefshell

 SUBJLINE (%)
      Controls the format of the lines displayed by the '=' command at the
      article selection level.

      Default: %s

 SUPERSEDEHEADER (%)
      The format of the header file for a supersede article.

      Default:

      Newsgroups: %n
      Subject: %[subject]
      %(%{FROM}=^$?:From: %{FROM}
      Summary: %[summary]
      Expires: %[expires]
      References: %[references]
      From: %[from]
      Reply-To: %[reply-to]
      Supersedes: %i
      Sender: %[sender]
      Followup-To: %[followup-to]
      Distribution: %D
      Organization: %o
      Keywords: %[keywords]\n\n

 TERM
      Determines which termcap entry to use, unless TERMCAP contains the
      entry.

 TERMCAP
      Holds either the name of your termcap file, or a termcap entry.

      Default: /etc/termcap, normally.

 TRNINIT
      Default values for switches may be passed to trn by placing them in
      the TRNINIT variable (or RNINIT if you're starting trn in
      rn-compatibility mode).  Any switch that is set in this way may be
      overruled on the command line, or via the '&' command from within trn.
      Binary-valued switches that are set with "-switch" may be unset using
      "+switch".

      If TRNINIT begins with a '/' it is assumed to be the name of a file
      containing switches.  You can put comments in this file by preceding
      them with a '#' as long as this is the first character on a line or
      it follows some white-space (which delimits the switches in the file).
      If you want to set many environment variables but don't want to keep 
      them all in your environment, or if the use of any of these variables
      conflicts with other programs, you can use this feature along with the
      -E switch to set the environment variables upon startup.

      Default: " ".

 TRNMACRO (~)
      The name of the file containing macros and key  mappings.  If the file
      is not found, the RNMACRO variable is used to look for your rn macros.
      For information on what to put into this file, see the CUSTOM MACROS
      section of the documentation.

      Default: %+/macros

 UNSHAR (~)
      The shell command to execute in order to accomplish the unshar'ing of
      a shell archive.

      Default: /bin/sh

 USER
      Your login name.  May be interpolated using "%L".

      Default: \$LOGNAME

 VISUAL (~)
      The name of your editor.

      Default: \$EDITOR

 XTERMMOUSE
      If you set this variable to 'y' (yes), trn will enable the use of the
      xterm mouse in the selector if you are using an xterm.  Once enabled,
      left-clicking on an item selects it while middle-clicking an item will
      move to that item.  If you click the top (header) line of the selector
      it moves up a page.  If you click the bottom (footer) line of the
      selector it executes the default command for the page (left click) or
      goes down a page (middle click).  You can also use the right mouse
      button to move up or down a page by clicking in the upper-half or
      lower-half of the screen, respectively.

 YOUSAID (%)
      Gives the format of the attribution line in front of the quoted article
      included by an R (reply-by-mail) command.

      Default: In article %i you write:

!GROK!THIS!
