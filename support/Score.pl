# Score
#
# Functions for scoring articles externally ala trn4.

require 5;

%scores = ();
$| = 1;

$HOME = $ENV{HOME};
$FILTERDIR = $ENV{FILTERDIR} || "$HOME/News/Filters";
$debug = $ENV{FILTER_DEBUG};

sub next_cmd {

    my ($line, $cmd, $arg);
    my ($dummy);

    chomp ($line = <>);

    $debug && print LOG "recv: `", $line, "'\n";

    ($cmd, $arg) = split (/\s+/, $line, 2);
    ($cmd, $arg);
}

sub article {

    my ($fields) = @_;
    my %art;
    my ($artid, $subj, $from, $date, $msgid, $refs, $bytes, $lines, $xref);

    ($artid, $subj, $from, $date, $msgid, $refs, $bytes, $lines, $xref) =
	split (/\t/, $fields);

    $debug && print STDERR "$artid..." if ($artid % 50 == 0);

    $scores{$artid} = 0;

    %art = (
	"number"     => $artid,
	"subject"    => $subj,
	"from"       => $from,
	"date"       => $date,
	"message-id" => $msgid,
	"references" => $refs,
	"bytes"      => $bytes,
	"lines"      => $lines,
	"xref"       => $xref
    );

    \%art;
}

sub score_art {
    local ($art, $score) = @_;
    $scores{$art->{number}} += $score;
}

sub select_art {
    local ($art) = @_;
    $scores{$art->{number}} = 10000;
}

sub junk_art {
    local ($art) = @_;
    $scores{$art->{number}} = -10000;
}

sub thwack {

    my ($cmd, $arg);
    my ($art);
    my ($CurrentGroup);
    my ($num);

    while (($cmd, $arg) = &next_cmd) {

	if ($cmd eq "bye" || $cmd eq "") {
	    exit;
	}
	elsif ($cmd eq "newsgroup") {
	    &done if defined &done;
	    $CurrentGroup = $arg;
	    undef &local_score;
	    undef @local_hdrs;
	    undef &init;
	    undef &done;
	    do "$FILTERDIR/$CurrentGroup" if -T "$FILTERDIR/$CurrentGroup";
	    if (defined(&global_score) || defined(&local_score)) {
		# No need to sort or weed duplicates
		$_ = 'want';
		$_ .= ' ' . join(' ', @global_hdrs) if defined(@global_hdrs);
		$_ .= ' ' . join(' ', @local_hdrs) if defined(@local_hdrs);
		$_ = 'overview' if $_ eq 'want';
		print $_, "\n";
		$debug && print LOG "\tsent: `$_'\n";
	    }
	    else {
		print "skip\n";
		$debug && print LOG "\tsent: `skip'\n";
	    }
	}
	elsif ($cmd eq "art") {
	    $art = &article ($arg);
	    &global_score ($art) if defined &global_score;
	    &local_score ($art) if defined &local_score;
	}
	elsif ($cmd eq "scores") {
	    $| = 1;
	    foreach $num (keys %scores) {
		print "$num $scores{$num}\n";
		$debug && print LOG "\tsent: `$num $scores{$num}'\n";
	    }
	    undef %scores;
	    print "done\n";
	}
	else {
	    $debug && print LOG "filter: received unknown command `$cmd' from newsreader\n";
	}
    }
}

1;
