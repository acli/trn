: see end of file for revision information
case $CONFIG in
    '') . ./config.sh ;;
esac
trnversion=`sed -e 's/.*" \(.*\)\"/\1/' $src/patchlevel.h 2>/dev/null`
echo "Extracting newsnews (with version substitution)"
cat >newsnews <<!GROK!THIS!
			*** NEWS NEWS ***

Welcome to trn $trnversion.

Type 'h' at any prompt to get help.  The old (rn-compatible) help
is usually available by typing 'H' as well.

This message will not be displayed again unless it is updated with new
information.

   - The friendly folks at <trn-workers@lists.sourceforge.net>
!GROK!THIS!
#
# This software is copyrighted as detailed in the LICENSE file.
