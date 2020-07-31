case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting mkpro (with variable substitutions)"
$spitshell >mkpro <<!GROK!THIS!
#!$perlpath
#
# mkpro - make prototypes, appending them to the .h and .ih files
#

chdir "$src";
!GROK!THIS!
$spitshell >>mkpro <<'!NO!SUBS!'

$dont_line="/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */\n";

$changes = 0;

# We're looking for the following limited format:
#
# void
# function(arg1, arg2)
# char* arg1;
# int (*arg2) _((void));
# {
#
# Once we find the '{' on a line by itself, we scan backward, parsing
# the arguments until we find the function line, at which point we snag
# the return value and check for a preceding #if line.
#
# Public prototypes for foo.c are written to foo.h, while the private
# (i.e. static) ones are written to foo.ih.  When updating the files,
# we generate foo.nh and foo.nih and discard any file that is not
# different from the original.  If a file is different, the original
# is moved into the directory "sanity", and the new file renamed.

foreach $file (<*.c>) {
    next if $file eq 'ndir.c' || $file eq 'parsedate.c';
    next unless $file =~ s/\.c$//;
    open(IN_C, "$file.c") || die "Can't open $file.c for input: $!\n";

    # Copy everything in foo.h up to the "DON'T EDIT" line to foo.nh
    if (!open(IN, "$file.h")) {
	#print STDERR "Can't open $file.h -- skipping $file.c.\n";
	next;
    }
    open(OUT_H, ">$file.nh") || die "Can't open $file.nh for output: $!\n";
    while (<IN>) {
	last if /DON'T EDIT BELOW/;
	print OUT_H $_;
    }
    print OUT_H $dont_line . "\n";
    close IN;

    # Copy everything in foo.ih up to the "DON'T EDIT" line to foo.nih
    open(OUT_I, ">$file.nih") || die "Can't open $file.nih for output: $!\n";
    if (open(IN, "$file.ih")) {
	while (<IN>) {
	    last if /DON'T EDIT BELOW/;
	    print OUT_I $_;
	}
	close IN;
	$need_I = 1;
	$new_I = '';
    } else {
	# If foo.ih doesn't yet exist, create a foo.nih anyway
	print OUT_I "/* $file.ih\n */\n\n";
	$need_I = 0;
	$new_I = '  **NEW**';
    }
    print OUT_I $dont_line . "\n";

    $cond_H = 'none';
    $cond_I = 'none';
    while (<IN_C>) {
	# Limit the lines stack to 20 entries
	shift @lines if $#lines >= 20;
	push(@lines, $_);
	next unless /^{\s*$/;
	pop @lines;
	undef @args;
	undef $func;
	# The default is to use the .h file unless we see a "static"
	$out = OUT_H;
	$cond = $cond_H;
	while ($_ = pop @lines) {
	    s|\s*/\*.*\*/||;		# trim any comment
	    if (/^(\S*).*;/) {
		# A line with ';' is an argument
		$arg = $1;
		# Discard "register" prefix
		if ($arg eq "register") {
		    /^register\s*(\S*)\s/;
		    $arg = $1;
		}
		# If the arg line has _((...)), remember that bit as well.
		if (/(_\(\(.*\)\))/) {
		    $arg .= "(*) $1";
		    push(@args,$arg);
		}
		else {
		    # Append an extra '*' for arg[]
		    $arg .= '*' if /]/;
		    push(@args,$arg);
		    # We allow only one duplicate, e.g. int arg1, arg2
		    if (/,/) {
			push(@args,$arg);
		    }
		}
	    } elsif (/^(.*)\(/) {
		# A line with '(' but without ';' is the function name.
		$func = $1;
	    } elsif (/,/) {
		# Any remaining line with a comma is a continuation-line
		# of the function's arg-list, e.g. "  arg8,arg9,arg10)"
	    } else {
		chop;
		$ret = $_;
		if (/^static\s/) {
		    # A static function goes in .ih, not .h
		    $out = OUT_I;
		    $cond = $cond_I;
		    $need_I = 1;
		}
		# Pop the previous line (and another if empty)
		$_ = pop @lines;
		$_ = pop @lines if /^$/;
		if (/^\#\s*if([^\/]*)/) {
		    # Remember conditional preceding function
		    $newcond = $1;
		    $newcond =~ s/\s*$//;
		} elsif (/^\#\s*else/) {
		    # Discard a function after an #else -- it's a duplicate
		    last;
		} else {
		    # No conditional -- turn it off
		    $newcond = 'none';
		}
		# Output conditional, but only if it's different.
		if ($cond ne $newcond) {
		    # Out with the old...
		    if ($cond ne 'none') {
			print $out "#endif\n";
		    }
		    # In with the new...
		    if ($newcond ne 'none') {
			print $out "#if$newcond\n";
		    }
		}
		# At last, output the actual prototype.
		print $out "$ret $func _((";
		if ($_ = pop @args) {
		    print $out $_;
		    while ($_ = pop @args) {
			print $out ",$_";
		    }
		} else {
		    print $out "void";
		}
		# Some special processing for non-returning "finalize"
		if ($func eq "finalize") {
		    print $out "))
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR >= 5)
  __attribute__((noreturn))
#endif
  ;\n";
		} else {
		    print $out "));\n";
		}
		# Remember the conditional state of the current .h|.ih file.
		if ($ret =~ /^static\s/) {
		    $cond_I = $newcond;
		} else {
		    $cond_H = $newcond;
		}
		last;
	    }
	}
    }
    close IN_C;

    # Close any dangling conditionals
    if ($cond_H ne 'none') {
	print OUT_H "#endif\n";
    }
    close OUT_H;

    if ($cond_I ne 'none') {
	print OUT_I "#endif\n";
    }
    close OUT_I;

    # Check for any differences and update if changed.
    if (system "diff $file.h $file.nh >/dev/null") {
	print "Updating $file.h\n";
	mkdir("sanity",0755) unless -d "sanity";
	rename("$file.h","sanity/$file.h");
	rename("$file.nh","$file.h");
	$changes++;
    } else {
	unlink "$file.nh";
    }

    # The .ih file is a little different, in that it might not be needed.
    if ($need_I) {
	if ($new_I ne '') {
	    system "touch $file.ih";
	}
	if (system "diff $file.ih $file.nih >/dev/null") {
	    print "Updating $file.ih$new_I\n";
	    mkdir("sanity",0755) unless -d "sanity";
	    rename("$file.ih","sanity/$file.ih");
	    rename("$file.nih","$file.ih");
	    $changes++;
	} else {
	    unlink "$file.nih";
	}
    } else {
	unlink "$file.nih";
    }
}

print "No changes to prototypes.\n" if !$changes;
!NO!SUBS!
$eunicefix mkpro
chmod 755 mkpro
