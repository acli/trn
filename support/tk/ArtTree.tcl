class ArtTree
#Note that Widget is also inherited by TrnModule
ArtTree inherit TrnModule

#Instance variables
#f		frame or toplevel containing articletree
#XXX document the rest later

#If there are *no* options at all, the <SECT>_unknown handlers do not work

#Give these options to the contained Tree, not to the TrnModule
#XXX deal with cget later...
ArtTree option -width {0} verify {
    instvar tree_options
    lappend tree_options -width $width
}
ArtTree option -height {0} verify {
    instvar tree_options
    lappend tree_options -height $height
}
ArtTree option -enabled {1}
ArtTree option -zoomfactor {0.75}
ArtTree option -update_spin {10}

ArtTree method init { args } {
    instvar f enable_state enable_button tree_options

#puts "ArtTree init: begin"
    set tree_options {}
    eval $self conf_verify $args

    #set up our inheritance tree...
    set f [next]

    #do the main setup for the object
    $self setup
}

ArtTree method setup { args } {
    instvar tree enabled enable_button tree_options top_msgid \
	cur_ap changes_ap cursor_ap update_count click_ap idlabel

#puts "ArtTree setup: begin"
    set top_msgid ""
    set cur_ap [trn_article]
    set changes_ap [trn_article]
    set cursor_ap [trn_article]
    set click_ap [trn_article]
    set update_count 0

    frame $self.bf1
    set enable_button [button $self.bf1.enable_button \
	-command "$self button"]
    $self button_text
    button $self.bf1.zout -text "<<" -command "$self do_soon zoom_out"
    button $self.bf1.zreset -text "1.0" -command "$self do_soon zoom_reset"
    button $self.bf1.zin -text ">>" -command "$self do_soon zoom_in"
    pack $enable_button \
	$self.bf1.zout $self.bf1.zreset $self.bf1.zin \
	-side left -fill x -expand yes
    frame $self.lf2
    set idlabel [label $self.lf2.idlabel -width 30 -text "<>"]
    pack $self.lf2.idlabel -fill x
    lappend tree_options -entercommand "$self enternode" \
	-leavecommand "$self leavenode" \
	-clickcommand "$self clicknode"
#dbg "Tree options: $tree_options"
    if {$tree_options != {}} {
	set tree [eval Tree $self.tree $tree_options]
    } else {
	set tree [Tree $self.tree]
    }
    pack $self.bf1 -side top -fill x
    pack $self.lf2 -side top -fill x
    pack $tree -side top -expand yes -fill both

    $self modulepack
}

ArtTree method nodesize {ap} {
    set n 0
    if {![$ap flag parentsubject]} {
	incr n 1
    }
    return $n
}

ArtTree method erase {} {
    instvar tree draw_background top_msgid id2vn vn2id draw_interrupt

    $self hidecursor
    set draw_background 0
    set draw_interrupt 0
    set top_msgid ""
    catch {unset id2vn}
    catch {unset vn2id}
    $tree erase
}

#draw article tree starting with article with message-ID msgid
ArtTree method draw {msgid} {
    instvar enabled curx cury cur_ap draw_done pstackptr \
	draw_background top_msgid tree draw_interrupt

    if {(!$enabled) || ($msgid=="")} {
	return
    }
    if {$top_msgid==$msgid} {
	ttk_idle_add "$self draw_continue"
	$self treechanges
	$self showcursor
	return
    }

    $self erase

    set curx 0
    set cury 0
    set draw_done 0
    set pstackptr 0
    set draw_interrupt 0

    if {[$cur_ap setid $msgid]} {
	set draw_background 1
	set top_msgid $msgid
	$cur_ap treeprepare
	ttk_idle_add "$self draw_continue"
    } else {
	dbg "ArtTree draw: invalid message-id"
    }
}

ArtTree method draw_done {} {
    instvar tree draw_done draw_background draw_interrupt

    $self showcursor
    $tree setscrollregion
#Scroll tree to show current plotted article?
    set draw_done 1
    set draw_background 0
    set draw_interrupt 0
}

#Later consider separating out some of these decisions (like colors)
ArtTree method draw_node {} {
    instvar tree curx cury cur_ap id2vn vn2id

    set shape rectangle
    set fillcol white

    if {![$cur_ap flag exists]} {
	set fillcol gray75
    }
    if {![$cur_ap flag unread]} {
	set shape oval
    }
#XXX testing--icky colors. 
    set score [$cur_ap scorequick]
    if {$score <= -3} {
	set fillcol LightBlue
    }
    if {$score <= -6} {
	set fillcol DarkBlue
    }
    if {$score >= 4} {
	set fillcol LightPink
    }
    if {$score >= 8} {
	set fillcol HotPink
    }

    set letter [$cur_ap subjectletter]

    set vn [$tree makenode $curx $cury $shape $fillcol $letter]
    if {![$cur_ap flag parentsubject]} {
	$tree draw_text $vn [$cur_ap header subject]
    }
    set msgid [$cur_ap header msgid]
    set id2vn($msgid) $vn
    set vn2id($vn) $msgid
    return $vn
}

ArtTree method redraw_node {ap vn} {
    instvar tree

#    dbg "redraw_node: updating node ap=$ap, vn=$vn"
    set shape rectangle

    if {![$ap flag unread]} {
	set shape oval
    }
    $tree changeshape $vn $shape
}


#Useful for debugging
ArtTree method update_spin {} {
    instvar update_count update_spin tree

    incr update_count
    if {($update_count%$update_spin)==0} {
	$tree setscrollregion
	update
	return 1
    }
    return 0
}


#Consider whether some top items should show parent line.
ArtTree method draw_continue {} {
    instvar tree curx cury cur_ap draw_done pstack pstackptr \
	draw_background enabled draw_interrupt

    if {(!$enabled) || ($draw_done) || (!$draw_background)} {
	return
    }

#dbg "draw_continue enter"
update
    while (1) {

	$self update_spin
	if {$draw_interrupt} {
	    set draw_interrupt 0
	    return
	}
	if {([trn_misc pending])} {
	    ttk_idle_add "$self draw_continue"
	    $tree setscrollregion
	    return
	}

#XXX make much more interesting (local method?)
	set vn [$self draw_node]

	if {$pstackptr>0} {
	    set pvn $pstack($pstackptr)
	    $tree draw_parentline $vn $pvn
	    if {![$cur_ap flag nextsibling]} {
		$tree draw_childline $pvn $vn
	    }
	}
	set ns [$self nodesize $cur_ap]
	incr cury $ns
#later also check to see if we will draw the child.
	if {[$cur_ap flag child]} {
	    incr curx
	    incr pstackptr
	    set pstack($pstackptr) $vn
	    $cur_ap move child
	} else {
	    if {$ns==0} {
		incr cury 1
	    }
	    if {[$cur_ap flag nextsibling]} {
		$cur_ap move sibling
	    } else {
		if {$pstackptr==0} {
		    $self draw_done
		    return
		}
		set foundsib 0
		while {(!$foundsib) && ($pstackptr>0)} {
		    incr curx -1
		    incr pstackptr -1
		    $cur_ap move parent
		    if {[$cur_ap flag nextsibling]} {
			$cur_ap move sibling
			set foundsib 1
		    }
		}
		if {$foundsib==0} {
		    $self draw_done
		    return
		}
	    }
	}
    }
}

ArtTree method nodeupdate {ap} {
    instvar id2vn

    set msgid [$ap header msgid]
#    dbg "updating node $ap: $msgid"
    if {[info exists id2vn($msgid)]} {
	$self redraw_node $ap $id2vn($msgid)
    }
}

ArtTree method treechanges {} {
    instvar changes_ap top_msgid
#XXX work on the globals
    global tree_nodechanged

    if {[$changes_ap setid $top_msgid]} {
	set tree_nodechanged "$self nodeupdate"
	$changes_ap treechange
    } else {
	dbg "ArtTree treechanges: error setting id: |$top_msgid|"
    }
}

ArtTree method showcursor {} {
    instvar cursor_ap id2vn tree

    set tmp [$cursor_ap setcurrent]

    if {$tmp>0} {
	set msgid [$cursor_ap header msgid]
	if {[info exists id2vn($msgid)]} {
	    $tree plot_cursor $id2vn($msgid)
	}
    }
}

ArtTree method hidecursor {} {
    instvar tree

    $tree unplot_cursor
}

#What to do when the button is pressed  (arg is button path)
ArtTree method button { } {
    instvar enabled enable_button

    $enable_button flash
    if {$enabled} {
	set enabled 0
	$self erase
    } else {
	set enabled 1
    }
    $self button_text
}

ArtTree method button_text {} {
    instvar enabled enable_button

    if {$enabled} {
	$enable_button configure -text "Tree is ON"
    } else {
	$enable_button configure -text "Tree is OFF"
    }
}

ArtTree method redraw {} {
    instvar top_msgid

    set safeid $top_msgid
    $self erase
    $self draw $safeid
}

ArtTree method zoom_in {} {
    instvar tree zoomfactor

    $tree zoomfactor [expr 1/$zoomfactor]
    $self redraw
}

ArtTree method zoom_out {} {
    instvar tree zoomfactor

    $tree zoomfactor $zoomfactor
    $self redraw
}

ArtTree method zoom_reset {} {
    instvar tree zoomfactor

    $tree zoomfactor reset
    $self redraw
}

#can interrupt drawing...
ArtTree method do_soon {cmd} {
    instvar draw_interrupt

    ttk_idle_add "$self $cmd"
    set draw_interrupt 1
}

ArtTree method enternode {vn} {
    instvar vn2id idlabel

    if {[info exists vn2id($vn)]} {
	$idlabel configure -text $vn2id($vn)
    }
}

ArtTree method leavenode {vn} {
    instvar idlabel

    $idlabel configure -text "<>"
}

#Within the current group, attempt to go to an article
#later handle 's' (scan mode), 't' thread selector in appropriate ways...
#(if the article is<=0, do nothing)
ArtTree method goto_article {anum} {
    global ttk_keys
    if {$anum>0} {
	#if the article number is good...
	set trn_mode [trn_misc mode]
	if {(($trn_mode == "t"))} {
	    append ttk_keys "+"
	}
	if {(($trn_mode == "p") || ($trn_mode == "a") || ($trn_mode == "e")
	    || ($trn_mode == "t"))} {
	    append ttk_keys "$anum\n"
	} elseif {$trn_mode == "s"} {
	    append ttk_keys "G$anum\n"
	}
    }
}


ArtTree method clicknode {vn} {
    instvar click_ap vn2id

    if {[info exists vn2id($vn)]} {
	set msgid $vn2id($vn)
	$click_ap setid $msgid
	set anum [$click_ap artnum]
    }
    $self goto_article $anum
}



################# Unknown options/methods
#Unknown options and methods are redirected to the contained article tree

ArtTree sys_method verify_unknown { opt args } {
    instvar tree tree_options
#puts "ArtTree verify_unknown: $opt $args"
    lappend tree_options $opt $args
}

ArtTree sys_method configure_unknown { opt args } {
    instvar tree
    eval $tree configure $opt $args
}

ArtTree sys_method cget_unknown { opt } {
    instvar tree
    $tree cget $opt
}

ArtTree method unknown { args } {
    instvar tree
    eval {$tree $method} $args
}
