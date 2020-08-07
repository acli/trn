case $CONFIG in
    '') . ./config.sh ;;
esac
patchlevel=patchlevel.h
test -f patchlevel2.h && patchlevel=patchlevel2.h
trnversion=`sed -e 's/.*"\(.*\)\"/\1/' $src/$patchlevel 2>/dev/null`
echo "Extracting Pnews (with variable substitutions)"
$spitshell >Pnews <<!GROK!THIS!
$startsh
# 
# This software is copyrighted as detailed in the LICENSE file.
#
# syntax: Pnews -h headerfile			or
#	  Pnews -h headerfile oldarticle	or
#         Pnews newsgroup title			or just
#         Pnews

export PATH || (echo "OOPS, this isn't sh.  Desperation time.  I will feed myself to sh."; sh \$0; kill \$\$)

# trn version
pnewsversion="trn$trnversion"

# Current output charset
charset=utf-8
if [ -n "$LC_CTYPE" ]; then
	case "$LC_CTYPE" in
	*.*)	charset="`echo "$LC_CTYPE" | cut -d. -f2`"
	esac
elif [ -n "$LC_ALL" ]; then
	case "$LC_ALL" in
	*.*)	charset="`echo "$LC_ALL" | cut -d. -f2`"
	esac
elif [ -n "$LANG" ]; then
	case "$LANG" in
	*.*)	charset="`echo "$LANG" | cut -d. -f2`"
	esac
fi

# System dependencies

mailer="\${RNMAILER-${mailer-/bin/mail}}"
# if you change this to something that does signatures, take out signature code

case $d_portable in
define)
    # where recordings and distributions are kept
    lib=\`$filexp $newslib\`
    # where important rn things are kept
    rnlib=\`$filexp $privlib\`
    artcheck=trn-artchk
    nntplist=nntplist
    inews=\`$filexp ${useinews-inews}\`
    ;;
undef)
    # where recordings and distributions are kept
    lib="$newslib"
    # where important rn things are kept
    rnlib="$privlib"
    artcheck=$bin/trn-artchk
    nntplist=$bin/nntplist
    inews=${useinews-inews}
    ;;
esac

# your organization name
orgname="$orgname"
# what pager you use--if you have kernal paging use cat
pager="\${PAGER-$pager}"
# how you derive full names, bsd, usg, or other
nametype="$nametype"
# default editor
defeditor="$defeditor"
# how not to echo with newline
n="$n"
c="$c"

# You should also look at the distribution warnings below marked !DIST!
# to make sure any distribution regions you are a member of are included.
# The following are some prototypical distribution groups.  If you do not
# use them all set the unused ones to a non-null string such as 'none'.
loc="$locdist"
org="$orgdist"
multistate="$multistatedist"
city="$citydist"
state="$statedist"
cntry="$cntrydist"
cont="$contdist"

active=${active-$lib/active}
groupdesc=${groupdesc-$lib/newsgroups}
test=${test-test}
sed=${sed-sed}
echo=${echo-echo}
cat=${cat-cat}
egrep=${egrep-egrep}
grep=${grep-grep}
tr=${tr-tr}
cp=${cp-cp}
sleep=${sleep-sleep}
rm=${rm-rm}
who=${who-who}
mv=${mv-mv}
pgp=$pgp

nidump=${nidump}
ypmatch=${ypmatch}

!GROK!THIS!
case "$d_ignoreorg" in
define) $spitshell >>Pnews <<'!NO!SUBS!'
orgname=${NEWSORG-$orgname}
!NO!SUBS!
	;;
*)	$spitshell >>Pnews <<'!NO!SUBS!'
orgname=${NEWSORG-${ORGANIZATION-$orgname}}
!NO!SUBS!
	;;
esac
$spitshell >>Pnews <<'!NO!SUBS!'
homedir=${HOME-$LOGDIR}
dotdir=${DOTDIR-$homedir}
tmpartcopy=$dotdir/.article
tmpart=$tmpartcopy.$$
speller=$rnlib/Speller
exitval=0

# see if PGP exists
pgp=${PGPCOMMAND-$pgp}
$test -n "$pgp" && pgpprompt=" PGP,"
pgpsearch='^-----BEGIN PGP SIG'

tmp="${TMPDIR-/tmp}"
case "$NEWSACTIVE" in
'') ;;
*) active="$NEWSACTIVE";;
esac
case "$active" in
none|'') active="$tmp/Pnact.$$";;
esac
case "$NEWSDESCRIPTIONS" in
'') ;;
*) groupdesc="$NEWSDESCRIPTIONS";;
esac
case "$groupdesc" in
none|'') groupdesc="$tmp/Pndesc.$$";;
esac
news_sig=${NEWSSIGNATURE-$dotdir/.news_sig}

case "$NETSPEED" in
[0-4]) ;;
[1-9]*|f*)
    if $test ! -f "$groupdesc"; then
	groupdesc="$tmp/Pndesc.$$"
	rmlist="$groupdesc"
	cmdlist="$nntplist -o $groupdesc newsgroups ;"
    fi
    if $test ! -f "$active"; then
	active="$tmp/Pnact.$$"
	rmlist="$rmlist $active"
	cmdlist="$cmdlist $nntplist -o $active active"
    fi
    ;;
esac

if $test -f $dotdir/.pnewsexpert -o "X$FAST_PNEWS" != X ; then
    expertise=expert
else
    $cat <<'EOM'
I see you've never used this version of Pnews before.  I will give you extra
help this first time through, but then you must remember what you learned.
If you don't understand any question, type h and a CR (carriage return) for
help.

If you've never posted an article to the net before, it is HIGHLY recommended
that you read the netiquette document found in news.announce.newusers so
that you'll know to avoid the commonest blunders.  To do that, interrupt
Pnews, get to the top-level prompt of [t]rn, and use the command
"g news.announce.newusers" to go to that group.

EOM
    expertise=beginner
fi

case $cntry in
  can) Stpr=Province ; stpr=province ;;
  *)   Stpr=State ; stpr=state ;;
esac

case $multistate in
  pnw) multistpr="Pacific NorthWest" ;;
  *)   multistpr="Multi-State Area" ;;
esac

headerfile=""
case $# in
0) ;;
*)  case $1 in
    -h)
	headerfile="$2"
	shift
	shift
	case $# in
	0)
	    oldart=""
	    ;;
	*)
	    oldart="$1"
	    shift
	    ;;
	esac
	;;
    esac
    ;;
esac

case $headerfile in
'')
    . $rnlib/Pnews.header
    ;;
*)
    $cp $headerfile $tmpart
    ;;
esac

rescue="$sleep 1; $cat $tmpart >>${HOME-$LOGDIR}/dead.article; $echo Article appended to ${HOME-$LOGDIR}/dead.article; exit 0"
trap "$cp $tmpart $tmpartcopy; $rm -f $rmlist $tmpart*; exit \$exitval" 0
trap "trap : 1; $rescue" 1
trap "$rescue" 2

$echo ""

# extract the newsgroups list and distribution
hdr_newsgroups=`$sed -n -e '/^Newsgroups:/{' -e 's///' -e 's/,/ /g' -e p -e q -e '}' $tmpart`
hdr_distribution=`$sed -n -e '/^Distribution:/{' -e 's///' -e p -e q -e '}' $tmpart`

# check for "poster" magic cookie.  Allow erroneous user@site too.
flag=0
for ng in $hdr_newsgroups ; do
    case "$ng" in
	poster)	flag=1 ;;
	*@*) flag=1 ;;
	*)	;;
    esac
done
case $flag in
1)
    $cat <<'EOM'

The original author has requested that messages be sent back via mail
rather than posting to news.  Do you want to jump out of this and
EOM
    $echo $n "mail your reply instead? [yn] $c"
    read ans
    case $ans in
	n*) $sed '1,/^[	 ]*$/{/^Newsgroups:/d;
		s/^X-ORIGINAL-NEWSGROUPS:/Newsgroups:/;}' $tmpart >$tmpart.tmp
	    $mv $tmpart.tmp $tmpart
	    ;;
	*)  exit 0 ;;
    esac
    $echo " "
    $echo "The Cc: line should have the poster's requested return address."
    ;;
esac
  
# play recorded message
if $test -s ${lib}/recording ; then
     for ng in $hdr_newsgroups ; do
	_rec1=${lib}/`$sed -n "/^$ng/s/^.*	//p" ${lib}/recording`
	_tmp=`$echo $ng |$sed "s/\..*//"`
	_rec2=${lib}/`$cat -s ${lib}/recording|$grep ${_tmp}.all|$sed "s/^.*	//"`
	if $test -f ${_rec1} ; then
	    $cat -s ${_rec1}
	fi
	if $test -f ${_rec2} ; then
	    $cat -s ${_rec2}
	fi
    done
fi

# determine the distribution of this message
set X $hdr_distribution
shift
if $test $# -gt 0 ; then
    dist=$1.whatever
else
    set X $hdr_newsgroups
    shift
    if $test $# -gt 0 ; then
	dist=$1.whatever
    else
	dist=misc.whatever
    fi
fi
case $dist in
*.*)
    ;;
*)
    dist=$dist.whatever
    ;;
esac

case "$FAST_PNEWS" in
y*) ;;
*)
# tell them what we think they are doing... !DIST!
case $dist in
world.*|comp.*|news.*|sci.*|rec.*|misc.*|soc.*|talk.*|alt.*|'')
    $cat <<'EOM'
This program posts news to thousands of machines throughout the entire
civilized world.  Your message will cost the net hundreds if not thousands of
dollars to send everywhere.  Please be sure you know what you are doing.

EOM
    ;;
vmsnet.*)
    $echo 'This program posts news to many machines.'
    ;;
bit.*)
    $echo 'This program posts news to many machines on BITNET.'
    ;;
ddn.*)
    $echo 'This program posts news to many machines throughout the internet.'
    ;;
$cont.*)
    $echo 'This program posts news to many machines throughout the continent.'
    ;;
$cntry.*)
    $echo 'This program posts news to many machines throughout the country.'
    ;;
$multistate.*)
    $echo "This program posts news to many machines throughout the ${multistpr}."
    ;;
$state.*)
    $echo "This program posts news to many machines throughout the ${stpr}."
    ;;
$city.*)
    $echo 'This program posts news to many machines throughout the city.'
    ;;
$org.*)
    $echo 'This program posts news to machines throughout the organization.'
    ;;
$loc.*)
    $echo 'This program posts news to machines throughout the local organization.'
    ;;
*.*)
    $echo 'This program may post news to many machines.'
    ;;
to.*)
    $echo 'This program may post news to a particular machine.'
    ;;
*)
    $echo 'This program posts news to everyone on the machine.'
    ;;
esac
ans=""
while $test "X$ans" = X ; do
    $echo $n "Are you absolutely sure that you want to do this? [ny] $c"
    read ans
    case $ans in
    y*) ;;
    f*) ;;
    h*) $cat <<'EOH'

Type n or CR to exit, y to post.

EOH
	ans="" ;;
    *) exit 0 ;;
    esac
done
;;
esac

# run nntplist in the background, if necessary
if $test -n "$cmdlist"; then
    ( eval $cmdlist ) >/dev/null 2>&1 &
fi

# check for addresses in the .invalid TLD
if $egrep -i '^Cc: .*\.invalid([][()<>@,;:\\". ]|$)' $tmpart >/dev/null 2>&1; then
    $cat <<'EOM'

At least one of the addresses to which you are copying this article is
marked as an invalid address, so you will not be able to send mail to it
directly.  You should look for clues in the previous article to find an
address where the poster can receive mail.
EOM
fi

case "$FAST_PNEWS" in
y*) file=''
    $echo "" >> $tmpart
    state=edit
    ;;
*)  file=h;;
esac
while $test "X$file" = Xh ; do
    $echo ""
    $echo $n "Prepared file to include [none]: $c"
    read file || exit 0
    case $file in
    h)
	$cat <<'EOH'

If you have already produced the body of your article, type the filename
for it here.  If you just want to proceed directly to the editor, type a
RETURN.  In any event, you will be allowed to edit as many times as you
want before you send off the article.
EOH
	;;
    '')
	$echo "" >> $tmpart
	state=edit
	;;
    *)
	$cat $file >>$tmpart
	state=check
	;;
    esac
done

$echo ""

if $test -r $news_sig; then
    $echo "-- " >> $tmpart
    $sed 4q $news_sig >> $tmpart
fi

while : ; do
    case $state in
    edit)
	case $expertise in
	beginner)
	    $cat </dev/null >$dotdir/.pnewsexpert
	    $cat <<'EOMessage'
A temporary file has been created for you to edit.  Be sure to leave at
least one blank line between the header and the body of your message.
(And until a certain bug is fixed all over the net, don't start the body of
your message with any indentation, or it may get eaten.)

Within the header may be fields that you don't understand.  If you don't
understand a field (or even if you do), you can simply leave it blank, and
it will go away when the article is posted.

Type return to get the default editor, or type the name of your favorite
editor.

EOMessage
	    ;;
	esac
	case "${VISUAL-${EDITOR-}}" in
	'')
	    tmp=h
	    ;;
	*)
	    tmp=''
	    ;;
	esac
	while $test "X$tmp" = Xh ; do
	    $echo $n "Editor [${VISUAL-${EDITOR-$defeditor}}]: $c"
	    read tmp || state=rescue	# exits on EOF
	    case $tmp in
	    h)
		$cat <<'EOH'

Type a return to get the default editor, or type the name of the editor you
prefer.  The default editor depends on the VISUAL and EDITOR environment
variables.

EOH
		;;
	    '')
		;;
	    *)
		VISUAL=$tmp
		export VISUAL
		;;
	    esac
	done
	if $test $state = edit ; then
	    trap : 2
	    ${VISUAL-${EDITOR-$defeditor}} $tmpart $oldart
	    trap "$rescue" 2
	    state=check
	fi
	;;
    check)
	# wait for possible background nntplist
	if $test -n "$cmdlist"; then
	    wait
	    cmdlist=''
	fi

	# warn about long lines, malformed headers, misspelled newsgroups
	($artcheck $tmpart 79 $groupdesc $active) 2>/dev/null
	state=pgptest
	;;
    ask)
	$echo ""
	$echo $n "Check spelling,$pgpprompt Send, Edit, List, or Abort? $c"
	read ans || state=rescue	# exits on EOF

	case "$ans" in
	[aAqQ]*)
	    state=rescue
	    ;;
	[eE]*)
	    set $ans
	    case $# in
	    2)  VISUAL="$2" ;;
	    esac
	    state=edit
	    ;;
	[lL]*)
	    $pager $tmpart
	    state=ask
	    ;;
	[cCiI]*|[sS][pP]*)
	    $speller $tmpart
	    state=pgptest
	    ;;
	[pP]*)
	    if $test -n "$pgpprompt"; then
		ans=`$echo $ans | $sed -e 's/^[pP] *//'`
		state=pgpask
	    fi
	    ;;
	[sS]*)
	    state=send
	    ;;
	[hH]*)
	    $cat <<'EOH'

Type c to check the article's spelling, s to send the article, a to abort
and append the article to dead.article, e to edit the article again, or l
to list the article with your pager.

To invoke an alternate editor, type 'e editor'.
EOH
	    ;;
	'')
	    ;;
	*)
	    $echo ""
	    $echo "Unknown command: $ans"
	    ans=''
	    ;;
	esac
	;;
    pgpask)
	if test "X$ans" = X; then
	    $echo ""
	    $echo $n "Sign [user], Unsign, Test, or <Return>? $c"
	    read ans || state=rescue	# exits on EOF
	fi
	case "$ans" in
	[sS]*)
	    ans=`echo $ans | $sed -e 's/^[^ ]* *//'`
	    case "$ans" in
	    '') pgpsignopts='' ;;
	    *) pgpsignopts="-u $ans" ;;
	    esac
	    state=pgpsign
	    ;;
	[uU]*)
	    state=pgpundo
	    ;;
	[tT]*)
	    state=pgptest
	    ;;
	[aArR]*|'')
	    state=ask
	    ;;
	[hH]*)
	    $cat <<'EOH'

Type s to PGP-sign the article, u to unsign it, t to test the
signature, or a to abort from this sub-menu.  You can also specify
the action from the previous prompt by using ps to PGP-sign, pu
to unsign, etc.
EOH
	    ans=''
	    ;;
	*)
	    $echo ""
	    $echo "Unknown command: $ans"
	    ans=''
	    ;;
	esac
	;;
    pgptest)
	if $test -n "$pgpprompt"; then
	    ans=`$grep "$pgpsearch" $tmpart`
	    if $test "X$ans" != X; then
		$pgp +batchmode +verbose=0 -o $tmpart.tmp $tmpart
		$rm -f $tmpart.tmp
	    else
		$echo ""
		$echo "Article is not PGP-signed."
	    fi
	fi
	state=ask
	;;
    pgpsign)
	$echo ""
	ans=`$grep "$pgpsearch" $tmpart`
	if $test "X$ans" != X; then
	    $echo "Article is already signed."
	else
	    $echo "Preparing PGP signature:"
	    $sed -ne '1,/^[	 ]*$/ p' $tmpart > $tmpart.head
	    $sed -e '1,/^[	 ]*$/ d' $tmpart | $pgp $pgpsignopts +verbose=0 -fast >$tmpart.tmp
	    if $test -s $tmpart.tmp; then
		$cat $tmpart.head $tmpart.tmp >$tmpart
		$echo ""
		$echo "Article is now PGP-signed."
	    fi
	    $rm -f $tmpart.tmp
	fi
	state=ask
	;;
    pgpundo)
	$echo ""
	ans=`$grep "$pgpsearch" $tmpart`
	if $test "X$ans" != X; then
	    $sed -ne '1,/^[	 ]*$/ p' $tmpart > $tmpart.head
	    $sed -e '1,/^[	 ]*$/ d' $tmpart | $pgp +verbose=0 -f >$tmpart.tmp 2>/dev/null
	    if $test -s $tmpart.tmp; then
		$cat $tmpart.head $tmpart.tmp >$tmpart
		$echo "PGP signature removed from article."
	    else
		$echo "Article was not changed."
		state=ask
	    fi
	    $rm -f $tmpart.tmp
	else
	    $echo "Article is not PGP-signed."
	fi
	state=ask
	;;
    send)
	set X `$sed < $tmpart -n -e '/^Newsgroups: /{' -e p -e q -e '}'`
	shift
	case $# in
	2)
	    state=success
	    headerstrip='1,/^[	 ]*$/{/^[A-Z][-A-Za-z0-9]*:[	 ]*$/d;
		/^X-ORIGINAL-NEWSGROUPS:/d;
		/^[Cc][Cc]:/d;
		/^Distribution: world/d;
		/^X-Newsreader:/d;
		/^X-Newsposter:/d;
		/^User-Agent:/d;
		/^From:/s/Re:  */Re: /g;
		/^[	 ]*$/i\
X-Newsreader: '"$pnewsversion"'
		/^[	 ]*$/i\
Mime-Version: 1.0
		/^[	 ]*$/i\
Content-Type: text/plain; charset="'"$charset"'"
		/^[	 ]*$/i\
Content-Transfer-Encoding: 8bit
		}'
	    case "$NNTPSERVER" in
	    local|'') share_connection=no;;
	    *) share_connection=$TRN_VERSION;;
	    esac
	    case "$share_connection" in
	    4.*)
		$sed "$headerstrip" $tmpart >$headerfile
		exitval=42
		;;
	    *)
		NNTPFDS=''
		export NNTPFDS
		if $sed "$headerstrip" $tmpart | $inews -h ; then
		    : null
		else
		    state=ask
		fi
		;;
	    esac
	    cc=`$sed -n '1,/^[	 ]*$/{/^[Cc][Cc]:[	 ][^	 ]/p;}' $tmpart|
		$sed 's/^[Cc][Cc]:[	 ][	 ]*//'`
	    if $test "X$cc" != X ; then
		case "$mailer" in
		*recmail)  set X ;;
		*sendmail|*premail) set X -t ;;
		*)	   set X `echo $cc | $sed 's/,/ /g'` ;;
		esac
		shift
		(
		    case "$mailer" in
		    *recmail|*sendmail|*premail) $echo To: $cc ;;
		    esac
		    $sed "$headerstrip" $tmpart |
		       $sed '1,/^[	 ]*$/s/^Newsgroups:/X-Also-Posted-To:/'
		    if $test -f $homedir/.signature; then
			$echo "-- "
			$cat $homedir/.signature
		    fi
		) | $mailer $@
	    fi
	    ;;
	*)
	    $echo ""
	    $echo "Malformed Newsgroups line."
	    $echo ""
	    $sleep 1
	    state=edit
	    ;;
	esac
	;;
    rescue)
	if $test -s $tmpart; then
		$cat $tmpart >> ${HOME-$LOGDIR}/dead.article
		$echo "Article appended to ${HOME-$LOGDIR}/dead.article"
		$echo "A copy may be temporarily found in $tmpartcopy"
	else
		$echo "Null article discarded."
	fi
	exit 0
	;;
    success)
	case "${AUTHORCOPY-none}" in
	none)
	    ;;
	*)
	    set X ${USER-${LOGNAME-`$who am i`}} unknown
	    shift
	    ${POSTSAVER-$rnlib/mbox.saver} $tmpart "." "." 0 0 Pnews $AUTHORCOPY "From $1 `LANG= date`"
	    if $test $? -eq 0 ; then
		$echo "Article appended to $AUTHORCOPY"
	    else
		$echo "Cannot append to $AUTHORCOPY"
	    fi
	    ;;
	esac
	exit $exitval
	;;
    esac
done
!NO!SUBS!
$eunicefix Pnews
chmod 755 Pnews
$spitshell >Pnews.header <<'!NO!SUBS!'
case $# in
0)
    ng=h
    while $test "X$ng" = Xh ; do
	$echo ""
	$echo $n "Newsgroup(s): $c"
	read ng || exit 0
	case $ng in
	h)
	    $cat <<'EOH'

Type the name of one or more newsgroups to which you wish to post an article.
If you want to post to multiple newsgroups, it is better to do them all at
once than to post to each newsgroup individually, which defeats the news
reading programs' strategies of eliminating duplicates.

Separate multiple newsgroup names with commas.
EOH
	    ;;
	esac
    done
    ;;
*)
    ng=$1
    shift
    ;;
esac
case $ng in
*\ *)
    ng=`$echo "$ng" | $sed 's/[, ] */,/g'`
    ;;
esac
case $ng in
ddn.*)
    defdist=inet
    dist=h
    ;;
*.*)
    defdist=''
    dist=h
    ;;
*)
    defdist=''
    dist=''
    ;;
esac

while $test "X$dist" = Xh ; do
    if $test -f $lib/distributions; then
	$echo " "
	$echo "Your local distribution prefixes are:"
	$cat $lib/distributions
	$echo " "
    else
	$egrep -v '[	 ]none$' <<EOM

Your local distribution prefixes are:
    Local organization:	$loc
    Organization:	$org
    City:		$city
    $Stpr:  		$state
    $multistpr:	$multistate
    Country:		$cntry
    Continent:		$cont
    Everywhere:		<null> (not "world")

EOM
    fi
    $echo $n "Distribution ($defdist): $c"
    read dist || exit 0
    case $dist in
    '') dist="$defdist" ;;
    esac
    case "$dist" in
    h)
	$cat <<'EOH'

The Distribution line may be used to limit the distribution of an article
to some subset of the systems that would receive the article based only on
the Newsgroups line.  For example, if you want to sell your car in talk.auto,
and you live in New Jersey, you might want to put "nj" on the Distribution
line to avoid advertising in California, which has enough problems of its own.
The actual area designators to use depend on where you are, of course.
EOH
	;;
    world*|comp*|news*|sci*|rec*|misc*|soc*|talk*|alt*)
	dist=''
	;;
    ''|$loc|$org|$city|$state|$multistate|$cntry|$cont|$defdist)
	;;
    *)  
	if $test -f $lib/distributions && \
	  $egrep "^$dist[ 	]" $lib/distributions >$tmpart && \
	  $test -s $tmpart; then
	    : null
	else
	    $echo "Unrecognized distribution prefix--type h for help, CR to use anyway."
	    defdist=$dist
	    dist=h
	fi
	;;
    esac
done

follow=""

# LCP 16-Oct-91 Subject line is required.  Make it a little more
# difficult to omit.  Added "while : ; do", ... "done", and "if"
# at end of while loop.
while : ; do
  case $# in
  0)
    title=h
    while $test "X$title" = Xh ; do
	$echo ""
	$echo $n "Title/Subject: $c"
	read title || exit 0
	case $title in
	h)
	    $cat <<'EOH'

Type the title for your article.  Please make it as informative as possible
(within reason) so that people who aren't interested won't have to read the
article to find out they aren't interested.  This includes marking movie
spoilers as (spoiler), and rotated jokes as (rot 13).
EOH
	;;
	esac
    done
    ;;
  *)
    title="$*"
    # LCP 16-Oct-91 Added "set" and "shift".  Must insure $# is 0
    # in case the title is all white space and we make another
    # pass thru this loop.
    set X
    shift
    ;;
  esac
  if expr "X$title" : "^X[    ]*$" > /dev/null 2>&1
  then
    $cat <<'EOH'

Articles without a "Subject:" line will not be accepted by the News
system.  Please give a Title/Subject line for your article.
EOH
  else
     break
  fi
done


# now build a file with a header for them to edit

set X ${USER-${LOGNAME-`$who am i`}}
shift
logname=$1
case $logname in
*!*) logname=`expr "$logname" : '!\(.*\)$'` ;;
esac
case ${NAME-$nametype} in
bsd)
	if $test "X$ypmatch" != X; then
		fullname=`$ypmatch $logname passwd 2>/dev/null | $sed -e "s/^[^:]*:[^:]*:[^:]*:[^:]*:\([^,:;]*\).*"'$'"/\1/"`
	elif $test "X$nidump" != X; then
		fullname=`$nidump passwd / | $sed -e "/^$logname:/{s/^[^:]*:[^:]*:[^:]*:[^:]*:\([^,:;]*\).*"'$'"/\1/" -e "q" -e "}" -e "d"`
	fi
	if $test "X$fullname" = X; then
		fullname=`$sed </etc/passwd -e "/^$logname:/{s/^[^:]*:[^:]*:[^:]*:[^:]*:\([^,:;]*\).*"'$'"/\1/" -e "q" -e "}" -e "d"`
	fi
    case $fullname in
    *'&'*) : GACK
	lname=`$echo $logname | $tr 'a-z' 'A-Z'`
	lname=`$echo $lname $logname | $sed 's/^\(.\)[^ ]* ./\1/'`
	fullname=`$echo "$fullname" | $sed "s/&/${lname}/"`
	;;
    esac
    ;;
usg)
    if $test "X$ypmatch" != X; then
	fullname=`$ypmatch $logname passwd 2>/dev/null | $sed -e "s/^[^:]*:[^:]*:[^:]*:[^:]*:\([^(:]*\).*"'$'"/\1/" -e "s/^.*-//" -e "q"`
    fi
    if $test "X$fullname" = X; then
	fullname=`$sed </etc/passwd -e "/^$logname:/{s/^[^:]*:[^:]*:[^:]*:[^:]*:\([^(:]*\).*"'$'"/\1/" -e "s/^.*-//" -e "q" -e "}" -e "d"`
    fi
    ;;
*)
    fullname=${NAME-`$cat $dotdir/.fullname`}
    ;;
esac

case $orgname in
/*) orgname=`$cat $orgname` ;;
esac

$sed -e '/^Reply-To: $/d' > $tmpart <<EOHeader
Newsgroups: $ng
Subject: $title
Summary: 
Reply-To: $REPLYTO
Followup-To: $follow
Distribution: $dist
Organization: $orgname
Keywords: 
Cc: 

EOHeader

!NO!SUBS!
case "$d_nntp" in
define) $sed < Pnews.header -e '/^#NORMAL/d' > Pnews_h.new ;;
*)  $sed < Pnews.header -e '/^#NORMAL/s/^#NORMAL//' > Pnews_h.new ;;
esac
$mv Pnews_h.new Pnews.header
$eunicefix Pnews.header
