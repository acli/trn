#ScoreBar.tcl

#Note that there is only one global score_curvar variable, so some of
#this object-stuff is overkill...

class ScoreBar
ScoreBar inherit TrnModule

#Option is not used yet, but things stop working with no options
ArtTree option -enabled {1}

ScoreBar method init { args } {
    instvar f
    eval $self conf_verify $args

    set f [next]
    $self setup
}

#Consider adding the label back in if we are *not* a toplevel
ScoreBar method setup {} {
    instvar f
    global score_curvar

    $self configure -title "Current Score"
    scale $f.s \
	-length 10c -orient horizontal -from -50 -to 50 -tickinterval 10 \
	-variable score_curvar
    set score_curvar 0
    pack $f.s
}
