case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting mbox.saver (with variable substitutions)"
$spitshell >mbox.saver <<!GROK!THIS!
$startsh
# mbox.saver
# 
# This software is copyrighted as detailed in the LICENSE file.
# 
#	Arguments:
#	1 Full name of article (%A)
#	2 Public news spool directory (%P)
#	3 Directory of current newsgroup (%c)
#	4 Article number (%a)
#	5 Where in article to start (%B)
#	6 Newsgroup name (%C)
#	7 Save destination (%b)
#	8 First line of message, normally From...
#
export PATH || (echo "OOPS, this isn't sh.  Desperation time.  I will feed myself to sh."; sh \$0; kill \$\$)

!GROK!THIS!
case $mboxchar in
"$CTRLA")
    $spitshell >>mbox.saver <<!GROK!THIS!
( $echo "$CTRLA$CTRLA$CTRLA$CTRLA"
  if $test "\$5" = 0 -a ! "\$4" = 0 ; then
    $echo "Article: \$4 of \$6"
  fi
  $tail +\$5c \$1
  $echo ""
  $echo ""
  $echo "$CTRLA$CTRLA$CTRLA$CTRLA" ) >> \$7
!GROK!THIS!
    ;;
*)
    $spitshell >>mbox.saver <<!GROK!THIS!
 ( $echo "\$8"
  if $test "\$5" = 0 -a ! "\$4" = 0 ; then
    $echo "Article: \$4 of \$6"
  fi
  $tail +\$5c \$1 | $sed "s/^[Ff]rom />&/"
  $echo ""
  $echo "" ) >> \$7
!GROK!THIS!
    ;;
esac
$eunicefix mbox.saver
chmod 755 mbox.saver
