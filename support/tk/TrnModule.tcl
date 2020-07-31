#TrnModule: provides frames or toplevels to TrnModules--will later provide
#           common functionality of all modules.

class TrnModule
TrnModule inherit Widget

#toplevel config is an error
TrnModule option {-toplevel} {0} verify {
    if {$toplevel} {
	set toplevel 1
    } else {
	set toplevel 0
    }
}

TrnModule option {-width} {120} configure {
    instvar f
    $f config -width $width
}

TrnModule option {-height} {100} configure {
    instvar f
    $f config -height $height
}

#XXX clean this up, allow for initial setting
TrnModule option {-title} {} configure {
    instvar f toplevel
    if {$toplevel} {
	wm title $f $title
    }
}

TrnModule method init { args } {
    instvar f toplevel height width

    eval $self conf_verify $args

    if {$toplevel} {
	set f [next toplevel -width $width -height $height]
    } else {
	set f [next -width $width -height $height]
    }
    $self setsize
    return $f
}

#If not a toplevel, pack the frame
TrnModule method modulepack { args } {
    instvar f toplevel
    if {!$toplevel} {
	catch {pack $f -expand yes -fill both}
    }
}

#Later: save sizes of internal frames?
#            (might need to know resizing command, or make standard)

#Set size if there is a default (and if we are a toplevel)
#If there is no default, make the window saved later

#Note: This code used to set the size and position immediately.
#      Unfortunately, the window would (usually) not be mapped, so the
#      window manager did not adjust for the decorations.

TrnModule method setsize {} {
    instvar toplevel f
    global _TrnModule_winpos _TrnModule_winlist

    if {$toplevel} {
	set _TrnModule_winlist($f) 1
    }
}

#consider special check if no data
#Only currently mapped windows are saved.
proc TrnModule_savesizes {fname} {
    global _TrnModule_winpos _TrnModule_winlist

    foreach w [array names _TrnModule_winlist] {
	catch {
	    if {[winfo ismapped $w]} {
		set _TrnModule_winpos($w) [winfo geometry $w]
	    }
	}
    }
    set data [array get _TrnModule_winpos]

    set file [open $fname "w"]
    puts $file $data
    close $file
}

proc TrnModule_loadsizes {fname} {
    global _TrnModule_winpos
    catch {
	set file [open $fname "r"]
	gets $file data
	close $file
	array set _TrnModule_winpos $data
    }
}

#Reset all windows to their saved sizes.
proc TrnModule_setsizes {} {
    global _TrnModule_winpos _TrnModule_winlist
    foreach w [array names _TrnModule_winlist] {
	catch {wm geom $w $_TrnModule_winpos($w)}
    }
}

#Reset all windows to their natural sizes.
proc TrnModule_naturalsizes {} {
    global _TrnModule_winpos _TrnModule_winlist
    foreach w [array names _TrnModule_winlist] {
	catch {wm geom $w $_TrnModule_winpos($w)}
    }
}
