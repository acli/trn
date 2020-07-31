#libswanted=`echo $libswanted | sed 's/ x//'`
mailer='/usr/lib/mail/execmail'
xxx=`uname -X 2>/dev/null | sed -n 's/^Release.*=//p`
case "$xxx" in
''|3.2v2) d_rename=undef;;
esac
