case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting norm.saver (with variable substitutions)"
$spitshell >norm.saver <<!GROK!THIS!
$startsh
# norm.saver
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
#
export PATH || (echo "OOPS, this isn't sh.  Desperation time.  I will feed myself to sh."; sh \$0; kill \$\$)

( case "\$5" in
  0) $echo "Article: \$4 of \$6" ;;
  esac
  $tail +\$5c \$1
  $echo ""
  $echo "" ) >> \$7
!GROK!THIS!
$eunicefix norm.saver
chmod 755 norm.saver
