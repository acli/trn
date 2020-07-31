case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting makedepend (with variable substitutions)"
if test "X$src" = "X." ; then
	vincludes=''
else
	vincludes="-I. -I$src"
fi
$spitshell >makedepend <<!GROK!THIS!
$startsh
# makedepend.SH
# 
# This software is copyrighted as detailed in the LICENSE file.
#
export PATH || (echo "OOPS, this isn't sh.  Desperation time.  I will feed myself to sh."; sh \$0; kill \$\$)

srcdir=$src
objdir=`pwd`
objext='.\\$\(o\)'

cc_cmd="$cppstdin -I\$objdir -I$usrinc $vincludes $tkinc $cppminus"
ignorefiles="$usrinc|typedef\.h|/lib/gcc-lib/"

test=${test-test}
sed=${sed-sed}
echo=${echo-echo}
cat=${cat-cat}
egrep=${egrep-egrep}
rm=${rm-rm}
mv=${mv-mv}
uniq=${uniq-uniq}
sort=${sort-sort}
basename=${basename-basename}
!GROK!THIS!
$spitshell >>makedepend <<'!NO!SUBS!'

deptmp=$objdir/.deptmp
$cat /dev/null >$deptmp
$echo "(Note: this is going to take a while.)"
cd $srcdir
for file in *.[cy]; do
    case "$file" in
    *.c) filebase=`$basename $file .c`;;
    *.y) filebase=`$basename $file .y`;;
    esac
    $echo "Finding dependencies for $filebase.o."
    $sed -n <$file \
	-e '/^#/{' \
	-e 's|/\*.*$||' \
	-e 's/\\[ 	]*$//' \
	-e p \
	-e '}' | $cc_cmd | $sed \
	-e '/^# *line/s/line//' \
	-e '/^# *[0-9]*[	 ]*"/!d' \
	-e 's/^.*"\(.*\)".*$/'$filebase$objext': \1/' \
	-e 's|: \./|: |' \
	-e "s|$objdir/||" \
	-e 's/: *$/: '$file'/' \
	-e 's/: .*\.c/: '$file'/' | \
	$uniq | $sort | $uniq >>$deptmp
done

for file in *.SH; do
    case "$file" in
    Makefile.SH) ;;
    newsnews.SH)
	$echo `$basename $file .SH | $sed -e 's/_/./'`: $file \; /bin/sh "\$(srcdir)/$file" >>$deptmp
	;;
    environment.SH)
	$echo HelpFiles/config/environment: $file config.sh makedir \; /bin/sh "\$(srcdir)/$file" >>$deptmp
	;;
    *)
	$echo `$basename $file .SH | $sed -e 's/_/./'`: $file config.sh \; /bin/sh "\$(srcdir)/$file" >>$deptmp
	;;
    esac
done

cd $objdir

$sed <Makefile >Makefile.new -e '1,/^# AUTOMATICALLY/!d'

if $test -s $deptmp; then
    $echo "Updating Makefile..."
    $sed -e 's/\$[^(]/$$/g' $deptmp | $egrep -v "$ignorefiles" >dependencies
else
    $echo "You don't seem to have a proper C preprocessor.  Using egrep instead."
    cd $srcdir
    $egrep '^#include ' *.[cyh] ?.[cyh] >$deptmp
    cd $objdir
    $echo "Updating Makefile..."
    <$deptmp $sed -n 's|c:#include "\(.*\)".*$$|o: \1|p' > dependencies
    <$deptmp $sed -n 's|y:#include "\(.*\)".*$$|o: \1|p' >> dependencies
    <$deptmp $sed -n 's|h:#include "\(.*\)".*$$|h: \1|p' >> dependencies
fi
$echo "# WARNING: Put nothing here or make depend will gobble it up!" >> dependencies
$mv Makefile Makefile.old
$mv Makefile.new Makefile
$cat dependencies >>Makefile
$rm $deptmp
!NO!SUBS!
$eunicefix makedepend
chmod 755 makedepend
