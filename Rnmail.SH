case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting Rnmail (with variable substitutions)"
$spitshell >Rnmail <<!GROK!THIS!
$startsh
# Rnmail
# 
# This software is copyrighted as detailed in the LICENSE file.
#
# syntax: Rnmail -h headerfile [oldart]		or
#         Rnmail destination-list 		or just
#         Rnmail

export PATH || (echo "OOPS, this isn't sh.  Desperation time.  I will feed myself to sh."; sh \$0; kill \$\$)

# System dependencies

mailer="\${RNMAILER-${mailer-/bin/mail}}"
# if you change this to something that does signatures, take out signature code

# your organization name
orgname="$orgname"
# what pager you use--if you have kernal paging use cat
pager="\${PAGER-$pager}"
# how you derive full names, bsd, usg, or other
nametype="$nametype"
# default editor
defeditor="$defeditor"
# where the non-publics are
case $d_portable in
define)  rnlib=\`$filexp $privlib\` ;;
undef)   rnlib="$privlib" ;;
esac
# how not to do a newline with echo
n="$n"
c="$c"

test=${test-test}
sed=${sed-sed}
echo=${echo-echo}
cat=${cat-cat}
grep=${grep-grep}
egrep=${egrep-egrep}
cp=${cp-cp}
rm=${rm-rm}
sleep=${sleep-sleep}
who=${who-who}
pgp=$pgp

!GROK!THIS!
case "$d_ignoreorg" in
define) $spitshell >>Rnmail <<'!NO!SUBS!'
orgname=${NEWSORG-$orgname}
!NO!SUBS!
	;;
*)	$spitshell >>Rnmail <<'!NO!SUBS!'
orgname=${NEWSORG-${ORGANIZATION-$orgname}}
!NO!SUBS!
	;;
esac
$spitshell >>Rnmail <<'!NO!SUBS!'
dotdir=${DOTDIR-${HOME-$LOGDIR}}
tmpartcopy=$dotdir/.letter
tmpart=$tmpartcopy.$$
speller=$rnlib/Speller
mail_sig=${MAILSIGNATURE-$dotdir/.mail_sig}

# see if PGP exists
pgp=${PGPCOMMAND-$pgp}
$test -n "$pgp" && pgpprompt=" PGP,"
pgpsearch='^-----BEGIN PGP (SIGNED|MESSAGE)'

deadfile=${HOME-$LOGDIR}/dead.letter
if test -d $deadfile ; then
	deadfile=${deadfile}2
fi

headerfile=""
case $# in
0) ;;
*)  case $1 in
    -h)
	headerfile="$2"
	case $# in
	3) oldart=$3 ;;
	esac
	;;
    esac
    ;;
esac

case $headerfile in
'')
    case $# in
    0)
	to=h
	while $test "X$to" = Xh ; do
	    $echo ""
	    $echo $n "To: $c"
	    read to || exit 0
	    case $to in
	    h)
		$cat <<'EOH'

Type the net address of those people to whom you wish the message sent.
Additional recipients may be added on the Cc: line when you edit.

Separate multiple addresses with spaces.

EOH
		;;
	    esac
	done
	;;
    *)
	to="$*"
	;;
    esac
    to=`$echo "$to" | $sed 's/  */ /g'`

    title=h
    while $test "X$title" = Xh ; do
	$echo ""
	$echo $n "Title/Subject: $c"
	read title || exit 0
	case $title in
	h)
	    $cat <<'EOH'

Type the title for your message.  
EOH
	    ;;
	esac
    done

# now build a file with a header for them to edit
    
    case $orgname in
    /*) orgname=`$cat $orgname` ;;
    esac

    $sed -e '/^Reply-To: $/d' > $tmpart <<EOHeader
To: $to
Subject: $title
Organization: $orgname
Reply-To: $REPLYTO
Cc:
Bcc:

EOHeader

    ;;
*)
    $cat < $headerfile  > $tmpart
    ;;
esac

# check for addresses in the .invalid TLD
if $egrep -i '^(To|Cc|Bcc): .*\.invalid([][()<>@,;:\\". ]|$)' $tmpart >/dev/null 2>&1; then
    $cat <<'EOM'

At least one of the addresses you are about to mail is marked as an invalid
address, so you will not be able to send mail to it directly.  You should
look for clues in the previous article to find an address where the poster
can receive mail.
EOM
fi

case "$FAST_RNMAIL" in
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

If you have already produced the body of your message, type the filename
for it here.  If you just want to proceed directly to the editor, type a
RETURN.  In any event, you will be allowed to edit as many times as you
want before you send off the message.
EOH
	;;
    '')
	$echo "" >> $tmpart
	state=edit
	;;
    *)
	$cat $file >>$tmpart
	state=pgptest
	;;
    esac
done

$echo ""

if $test -r $mail_sig; then
    $echo "-- " >> $tmpart
    $cat $mail_sig >> $tmpart
fi

rescue="$sleep 1; $cat $tmpart >>$deadfile ; $echo Message appended to $deadfile ; exit 0"
trap "$cp $tmpart $tmpartcopy ; $rm -f $tmpart*" 0
trap "trap : 1; $rescue" 1
trap "$rescue" 2

while : ; do
    case $state in
    edit)
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
	    state=pgptest
	fi
	;;
    ask)
	$echo ""
	$echo $n "Check spelling,$pgpprompt Send, Edit, List, or Abort? $c"
	read ans || state=rescue	# exits on EOF

	case $ans in
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

Type c to check the message's spelling, s to send the message, a to abort
and append the message to dead.letter, e to edit the message again, or l
to list the message with your pager.

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
	    $echo $n "Sign, Encrypt, Both, Un-pgp, Test, or <Return>? $c"
	    read ans
	fi
	case "$ans" in
	[sS]*)
	    state=pgpsign
	    ;;
	[eE]*)
	    state=pgpencrypt pgpopt='-e'
	    ;;
	[bB]*)
	    state=pgpboth pgpopt='-es'
	    ;;
	[uU]*)
	    state=pgpundo
	    ;;
	[tT]*)
	    state=pgptest
	    ;;
	[rRaA]*|'')
	    state=ask
	    ;;
	[hH]*)
	    $cat <<'EOH'

Type s to PGP-sign the message, e to encrypt the message, b to both sign
and encrypt the message, t to test the state the current state of the
message, or a to abort this prompt.  You can also specify the action from
the previous prompt by using ps to PGP-sign, pe to encrypt, etc.
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
	state=ask
	if $test -n "$pgpprompt"; then
	    ans=`$egrep "$pgpsearch" $tmpart`
	    case "$ans" in
	    *SIGNED*)
		$pgp +batchmode +verbose=0 -o $tmpart.tmp $tmpart
 		$rm -f $tmpart.tmp
		;;
	    *MESSAGE*)
		$echo ""
		$echo "Message is PGP encoded."
		;;
	    *)
		$echo ""
		$echo "Message is not PGP-signed."
		;;
	    esac
	fi
	;;
    pgpsign)
	$echo ""
	ans=`$egrep "$pgpsearch" $tmpart`
	if $test "X$ans" != X; then
	    $echo "Article is already signed."
	else
	    $echo "Preparing PGP signature:"
	    $sed -ne '1,/^[	 ]*$/ p' $tmpart > $tmpart.head
	    $sed -e '1,/^[	 ]*$/ d' $tmpart | $pgp +verbose=0 -fast >$tmpart.tmp
	    if $test -s $tmpart.tmp; then
		$cat $tmpart.head $tmpart.tmp >$tmpart
		$echo ""
		$echo "Article is now PGP-signed."
	    fi
	    $rm -f $tmpart.tmp
	fi
	state=ask
	;;
    pgpencrypt|pgpboth)
	$echo ""
	ans=`$egrep "$pgpsearch" $tmpart`
	if $test "X$ans" != X; then
	    $echo "Article is already signed or encoded."
	else
	    $sed -ne '1,/^[	 ]*$/ p' $tmpart > $tmpart.head
	    cc="`$egrep -i '^((Bcc)|(Cc)|(To)):' $tmpart.head | $sed -e 's/^Bcc://' -e 's/^Cc://' -e 's/^To://' -e 's/,/ /g'`"
	    $echo "Processing message with pgp (to $cc):"
	    $sed -e '1,/^[	 ]*$/ d' $tmpart | $pgp +verbose=0 -fat $pgpopt $cc >$tmpart.tmp
	    if $test -s $tmpart.tmp; then
		$cat $tmpart.head $tmpart.tmp >$tmpart
		$echo ""
		$echo "Article has been processed."
	    fi
	    $rm -f $tmpart.tmp
	fi
	state=ask
        ;;
    pgpundo)
	$echo ""
	ans=`$egrep "$pgpsearch" $tmpart`
	if $test "X$ans" != X; then
	    $sed -ne '1,/^[	 ]*$/ p' $tmpart > $tmpart.head
	    case "$ans" in
	    *SIGNED*)
		$sed -e '1,/^[	 ]*$/ d' $tmpart | $pgp +verbose=0 -f >$tmpart.tmp 2>/dev/null
		;;
	    *MESSAGE*)
		$sed -e '1,/^[	 ]*$/ d' $tmpart | $pgp +verbose=0 -f >$tmpart.tmp
		;;
	    esac
	    $echo ""
	    if $test -s $tmpart.tmp; then
		$cat $tmpart.head $tmpart.tmp >$tmpart
		$echo "PGP signature removed from article."
	    else
		$echo "Article was not changed."
		state=ask
	    fi
	    $rm -f $tmpart.tmp
	else
	    $echo "Article is not PGP-processed."
	fi
	state=ask
	;;
    send)
	if $test -f $dotdir/.signature; then
	    $echo $n "Append .signature file? [y] $c"
	    read ans
	    case $ans in
	    ''|y*)
		$echo "-- " >> $tmpart
		cat $dotdir/.signature >> $tmpart
		;;
	    esac
	fi
	case $mailer in
	*sendmail|*premail)
	    $mailer -t <$tmpart
	    ;;
	*qmail-inject)
	    $mailer <$tmpart
	    ;;
# but recmail does not know about Bcc, alas
	*recmail)
	    $mailer <$tmpart
	    ;;
	*)
	    set X `$sed <$tmpart -n -e '/^To:/{' -e 's/To: *//p' -e q -e '}'`
	    shift
	    set X "$@" `$sed <$tmpart -n -e '/^Cc:/{' -e 's/Cc: *//p' -e q -e '}'`
	    shift
	    set X "$@" `$sed <$tmpart -n -e '/^Bcc:/{' -e 's/Bcc: *//p' -e q -e '}'`
	    shift
	    $grep -v "^Bcc:"  <$tmpart | $mailer "$@"
	    ;;
	esac
	case $? in
	0)
	    state=cleanup
	    ;;
	*)
	    state=rescue
	    ;;
	esac
	;;
    rescue)
	$cat $tmpart >>$deadfile
	$echo "Message appended to $deadfile"
	$echo "A copy may be temporarily found in $tmpartcopy"
	exit
	;;
    cleanup)
	case "${MAILRECORD-none}" in
	none)
	    ;;
	*)
	    set X ${USER-${LOGNAME-`$who am i`}} unknown
	    shift
	    ${REPLYSAVER-$rnlib/mbox.saver} $tmpart "." "." 0 0 Pnews $MAILRECORD "From $1 `LANG= date`"
	    if $test $? -eq 0 ; then
		$echo "Message appended to $MAILRECORD"
	    else
		$echo "Cannot append to $MAILRECORD"
	    fi
	    ;;
	esac
	exit
	;;
    esac
done
!NO!SUBS!
$eunicefix Rnmail
chmod 755 Rnmail
