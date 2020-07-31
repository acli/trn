case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting mktd (with variable substitutions)"
$spitshell >mktd <<!GROK!THIS!
#!$perlpath
#
# mktd - make typedef.h
#

chdir "$src";
!GROK!THIS!
$spitshell >>mktd <<'!NO!SUBS!'

# We scan the .h and .ih files for definitions of the form:
#
#    struct foo { ... };
# and
#    union bar { ... };
#
# and create typedefs of:
#
#    typedef struct foo FOO;
# and
#    typedef union bar BAR;
#
# in the file typedef.nh.  If it differs from typedef.h, move the
# old version to the directory "sanity" and rename the new one.

open(IN, "<typedef.h") || die "Can't open typedef.h for input\n";
open(OUT, ">typedef.nh") || die "Can't open typedef.nh for output\n";

# Copy everything up to the DON'T EDIT line.
while (<IN>) {
    print OUT $_;
    last if /DON'T EDIT BELOW/;
}

close IN;

# Scan through all the .h and .ih files
foreach $file (sort(<*.h>, <*.ih>)) {
    open(IN, $file) || die "Can't open $file for input\n";
    # The first typedef from this file needs a preceding comment.
    $need_comment = 1;
    while (<IN>) {
	next unless /^(struct|union)\s+([a-z][a-z0-9_]*)\s+{/;
	if ($need_comment) {
	    print OUT "\n/* $file */\n\n";
	    $need_comment = 0;
	}
	$upper = $2;
	$upper =~ tr/a-z/A-Z/;
	print OUT "typedef $1 $2 $upper;\n";
    }
    close IN;
}

close OUT;

# Check for any differences and update if changed.
if (system "diff -q typedef.h typedef.nh >/dev/null") {
	print "Updating typedef.h\n";
	mkdir("sanity",0755) unless -d "sanity";
	rename("typedef.h","sanity/typedef.h");
	rename("typedef.nh","typedef.h");
} else {
	print "No change to typedef.h\n";
	unlink "typedef.nh";
}
!NO!SUBS!
$eunicefix mktd
chmod 755 mktd
