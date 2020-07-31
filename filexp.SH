case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting filexp (with variable substitutions)"
$spitshell >filexp <<!GROK!THIS!
$startsh
sed=${sed-sed}
expr=${expr-expr}
basename=${basename-basename}
test=${test-test}
prefix=${prefix-invalid}
!GROK!THIS!
$spitshell >>filexp <<'!NO!SUBS!'
: expand filename
case "$1" in
 ~/*|~)
	echo $1 | $sed "s|~|${HOME-$LOGDIR}|"
	;;
 ~~/*|~~)
	echo $1 | $sed "s|~~|${TRNPREFIX-$prefix}|"
	;;
 ~*)
	if $test -f /bin/csh; then
		/bin/csh -f -c "glob $1"
		failed=$?
		echo ""
		exit $failed
	else
		name=`$expr x$1 : '..\([^/]*\)'`
		dir=`$sed -n -e "/^${name}:/{s/^[^:]*:[^:]*:[^:]*:[^:]*:[^:]*:\([^:]*\).*"'$'"/\1/" -e p -e q -e '}' </etc/passwd`
		if $test ! -d "$dir"; then
			me=`$basename $0`
			echo "$me: can't locate home directory for: $name" >&2
			exit 1
		fi
		case "$1" in
		*/*)
			echo $dir/`$expr x$1 : '..[^/]*/\(.*\)'`
			;;
		*)
			echo $dir
			;;
		esac
	fi
	;;
*)
	echo $1
	;;
esac
!NO!SUBS!
chmod +x filexp
$eunicefix filexp
