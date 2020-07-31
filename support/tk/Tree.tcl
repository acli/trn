#todo:
# * Make scrollbars optional?
# * consider separating tree and virtual node info?
# for outlines, allow 0 setting (or small enough scale?) to make no outline
# Need to add margins to size
# consider connecting line sizes?  Should they be scaled?

#Note: the Tree class does *not* pack the returned widget

class Tree
Tree inherit Widget

Tree option {-width} {200} configure {
    instvar c
    $c config -width $width
}

Tree option {-height} {200} configure {
    instvar c
    $c config -height $height
}

#Scale for zoom/unzoom operations: 1.0 is normal
Tree option {-scale} {1.0} configure {
    $self setscale
}

Tree option {-xgridsize} {30}
Tree option {-ygridsize} {30}
Tree option {-xshapesize} {9}
Tree option {-yshapesize} {9}
Tree option {-outlinewidth} {3}
Tree option {-outline_normal_color} {black}
Tree option {-outline_highlight_color} {red}
Tree option {-entercommand} {}
Tree option {-leavecommand} {}
Tree option {-clickcommand} {}


Tree method init { args } {
    instvar f c vnode vcnt

#puts "Tree init: begin, args are |$args|"
    eval {$self conf_verify} $args

    #go up the inheritance tree and get our widget...
    set f [next]

    $self setup
    return $f
}

Tree method setup { args } {
    instvar height width
    instvar f c vnode c2v vcnt \
	xgridsize ygridsize xshapesize yshapesize xgridoffset ygridoffset \
	cursor_status cursor_items

    set vcnt 0
    set cursor_status 0
    set cursor_items {}

#make more configurable later?
#make scrolling command more flexible?
#Reasonable defaults:
    set c [canvas $f.c -height $height -width $width \
	-xscrollcommand "$f.sbh set" -yscrollcommand "$f.sbv set"]
#    set c [canvas $f.c -height $height -width $width \
#	-xscrollcommand "$f.sbh set" -yscrollcommand "$self yscrollset"]
    $c configure -relief sunken -borderwidth 2
    scrollbar $f.sbh -orient horizontal -command "$c xview"
    scrollbar $f.sbv -orient vertical -command "$c yview"
#    scrollbar $f.sbv -orient vertical -command "$self yview"

    pack $f.sbh -side bottom -fill x
    pack $f.sbv -side right -fill y
    pack $c -side top -expand yes -fill both
    $self setbindings
#A reasonably safe scrollregion setting:
    $c configure -scrollregion {0 0 1 1}
#Scale needs to be set late so that the canvas will have been created
    $self setscale
}

#Scrolling commands for debugging
Tree method yscrollset {args} {
    instvar f

    puts "Yscroll: $args"
    eval $f.sbv set $args
}

Tree method yview {args} {
    instvar c

    puts "PreYview: $args |[$c yview]|"
    eval $c yview $args
    puts "PostYview: |[$c yview]|"
}

Tree method setbindings {} {
    instvar c vnode c2v

    bind $c <2> "$c scan mark %x %y"
    bind $c <B2-Motion> "$c scan dragto %x %y"
    $c bind node <Enter> "$self node_enter"
    $c bind node <Leave> "$self node_leave"
    $c bind node <1> "$self node_click"	
}

#consider whether highlight changing should be in command, and what should
#be passed upwards as the argument...
#XXX: try catching error conditions...
Tree method node_enter {} {
    instvar c c2v vnode outline_highlight_color entercommand

    set item [$c find withtag current]
    if {$item == ""} {
	break
    }
    set vn $c2v($item)
    $c itemconfigure $vnode($vn,shape) -outline $outline_highlight_color
    if {$entercommand != {}} {
	eval $entercommand $vn
    }
}

Tree method node_leave {} {
    instvar c c2v vnode outline_normal_color leavecommand

    set item [$c find withtag current]
    if {$item == ""} {
	break
    }
    set vn $c2v($item)
    $c itemconfigure $vnode($vn,shape) -outline $outline_normal_color
    if {$leavecommand != {}} {
	eval $leavecommand $vn
    }
}

Tree method node_click {} {
    instvar c c2v vnode clickcommand

    set item [$c find withtag current]
    if {$item == ""} {
	break
    }
    set vn $c2v($item)
    if {$clickcommand != {}} {
	eval $clickcommand $vn
    }
}

#XXX later add margins...
Tree method setscrollregion {} {
    instvar c
    $c configure -scrollregion [$c bbox all]
}

#Scale grid, shape, and outline size to current dimensions:
Tree method setscale {} {
    instvar c scale xgridsize ygridsize xgridoffset ygridoffset \
	xshapesize yshapesize outlinewidth
    instvar xgridscale ygridscale xshapescale yshapescale outlinewidthscale

#puts "Tree setscale: scale is $scale"
    set xgridscale [expr $xgridsize * $scale]
    set ygridscale [expr $xgridsize * $scale]
    set xgridoffset [expr $xgridscale/2]
    set ygridoffset [expr $ygridscale/2]
    set xshapescale [expr $xshapesize * $scale]
    set yshapescale [expr $yshapesize * $scale]
    set outlinewidthscale [expr $outlinewidth * $scale]
    $c configure -xscrollincrement $xgridscale -yscrollincrement $ygridscale
}

Tree method zoomfactor {zf} {
    instvar scale
    if {$zf=="reset"} {
	set scale 1.0
    } else {
	set scale [expr $scale*$zf]
    }
    $self setscale
}


#returns virtual node number
Tree method makenode {gx gy shape bcolor label} {
    instvar c vcnt vnode c2v
    instvar xgridoffset ygridoffset outline_normal_color scale
    instvar xgridscale ygridscale xshapescale yshapescale outlinewidthscale

    set vn [incr vcnt]
    set vnode($vn,gx) $gx
    set vnode($vn,gy) $gy

    set x [expr ($gx * $xgridscale) + $xgridoffset]
    set y [expr ($gy * $ygridscale) + $ygridoffset]

#soon add other options
#Later allow custom shape routines
    set t1 [$c create $shape [expr $x-$xshapescale] [expr $y-$yshapescale] \
	[expr $x+$xshapescale] [expr $y+$yshapescale] \
	-outline $outline_normal_color -width $outlinewidthscale \
	-fill $bcolor -tags node]
    set c2v($t1) $vn
    set vnode($vn,shape) $t1
#XXX do not make label if scale too small  (needs to be changable)
    if {$scale>=0.5} {
	set t1 [$c create text $x $y -text $label -fill black -tags node]
	set c2v($t1) $vn
	set vnode($vn,label) $t1
    }
    return $vn
}

#vn is the virtual node
#shape is the new shape
Tree method changeshape {vn shape} {
    instvar c vnode c2v
    instvar xgridoffset ygridoffset outline_normal_color
    instvar xgridscale ygridscale xshapescale yshapescale outlinewidthscale

    set gx $vnode($vn,gx)
    set gy $vnode($vn,gy)

    set x [expr ($gx * $xgridscale) + $xgridoffset]
    set y [expr ($gy * $ygridscale) + $ygridoffset]

    set oldshape $vnode($vn,shape)
    set bcolor [$c itemcget $oldshape -fill]
    $c delete $oldshape
    unset c2v($oldshape)

    set t1 [$c create $shape [expr $x-$xshapescale] [expr $y-$yshapescale] \
	[expr $x+$xshapescale] [expr $y+$yshapescale] \
	-outline $outline_normal_color -width $outlinewidthscale \
	-fill $bcolor -tags node]
    #make the shape appear below the label (if it exists)
    if {[info exists vnode($vn,label)]} {
	$c lower $t1 $vnode($vn,label)
    }
    set c2v($t1) $vn
    set vnode($vn,shape) $t1
}

#Write out text to the right of the virtual node
#Text is given as an arbitrary number of arguments
#XXX font choice later, possible skip when nodes too small
#XXX text color
#Consider different tag?
#Consider implementation that obeys grid? (one line per y-grid)
Tree method draw_text {vn txt} {
    instvar c vnode c2v
    instvar xgridscale ygridscale xshapescale yshapescale outlinewidthscale \
	xgridoffset ygridoffset scale

#XXX hack: do not draw text if too small a scale
    if {$scale<0.5} {
	return
    }
    set gx [expr $vnode($vn,gx) + 1]
    set gy $vnode($vn,gy)

    set x [expr ($gx * $xgridscale) + $xgridoffset - $xshapescale]
    set y [expr ($gy * $ygridscale) + $ygridoffset - $yshapescale]

    set item [$c create text $x $y -text $txt -fill black -anchor nw \
	-tags node]
    set c2v($item) $vn
    set vnode($vn,txt) $item
}


#XXX line color
#later do fancier things...
#Arguments are the current (child) virtual node and the parent virtual node
Tree method draw_parentline {vn pvn} {
    instvar c vnode c2v
    instvar xgridscale ygridscale xshapescale yshapescale outlinewidthscale \
	xgridoffset ygridoffset

    set gx $vnode($vn,gx)
    set gy $vnode($vn,gy)
    set x [expr ($gx * $xgridscale) + $xgridoffset]
    set y [expr ($gy * $ygridscale) + $ygridoffset]

    set pgx $vnode($pvn,gx)
    set pgy $vnode($pvn,gy)
    set px [expr ($pgx * $xgridscale) + $xgridoffset]
    set py [expr ($pgy * $ygridscale) + $ygridoffset]

    set xmid [expr $px + (($x-$px)/2)]

    set line [$c create line $x $y $xmid $y -tags pline]
    set c2v($line) $vn
    set vnode($vn,pline) $line
    $c lower $line
    if {$py!=$y} {
#Later: draw dot at parent end of line if dots are enabled
    }
}

#XXX line color
#later do fancier things...
#Arguments are the current (parent) virtual node and the child virtual node
#The child virtual node should be the *last* child.
Tree method draw_childline {vn cvn} {
    instvar c vcnt vnode c2v
    instvar xgridscale ygridscale xshapescale yshapescale outlinewidthscale \
	xgridoffset ygridoffset

    set gx $vnode($vn,gx)
    set gy $vnode($vn,gy)
    set x [expr ($gx * $xgridscale) + $xgridoffset]
    set y [expr ($gy * $ygridscale) + $ygridoffset]

    set cgx $vnode($cvn,gx)
    set cgy $vnode($cvn,gy)
    set cx [expr ($cgx * $xgridscale) + $xgridoffset]
    set cy [expr ($cgy * $ygridscale) + $ygridoffset]

    set xmid [expr $x + (($cx-$x)/2)]

    if {$cgy==$gy} {
	set line [$c create line $x $y $xmid $y -tags cline]
    } else {
	set line [$c create line $x $y $xmid $y $xmid $cy -tags cline]
    }

    set c2v($line) $vn
    set vnode($vn,cline) $line
    $c lower $line
}

#XXX cursor color
#consider not plotting below a certain size?
Tree method plot_cursor_loc {gx gy} {
    instvar c cursor_items cursor_status xgridscale ygridscale

    if {$cursor_status} {
	$self unplot_cursor
    }
    set cursor_status 1
    set x [expr $gx * $xgridscale]
    set y [expr $gy * $ygridscale]
#can place arbitrary number of items in list, for now, just one rectangle.
    lappend cursor_items [$c create rectangle $x $y \
	[expr $x+$xgridscale] [expr $y+$ygridscale] -fill lightgreen]
#later reverse list for lowering?
    foreach item $cursor_items {
	$c lower $item
    }
}

#Plot cursor on vnode vn
Tree method plot_cursor {vn} {
    instvar vnode

    set gx $vnode($vn,gx)
    set gy $vnode($vn,gy)

    $self plot_cursor_loc $gx $gy
}

Tree method unplot_cursor {} {
    instvar c cursor_items cursor_status

    foreach item $cursor_items {
	$c delete $item
    }
    set cursor_items {}
    set cursor_status 0
}

Tree method erase {} {
    instvar c vnode vcnt c2v

    $self unplot_cursor
    $c delete all
    catch {unset vnode}
    catch {unset c2v}
#Should we reset the vnode counter?
#   set vcnt 0
#A reasonably safe scrollregion setting:
    $c configure -scrollregion {0 0 1 1}
}

#For compatability...
Tree method wipetree {} {
    $self erase
}

