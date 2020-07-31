#atree.tcl
#
# Simple article tree implementation.
# Begun sometime around July 1995.
# For more information, contact caadams@ist.net
#
# [Jan. 96] This file is no longer being worked on--it is replaced by the
# object-oriented version of the article tree.
#
# Todo:
#	* consistent naming, pref. within a common prefix (ouch)
#	* allow multiple simultaneous trees?
#	* Factor out generic tree-drawing from article stuff?
#
# Consider:
#	* separate canvas->virtual array rather than $vnode(canvid.virt)

#Vague attempt at encapsulation:
set atree_top ".atree_top"

#catch needed in case window position code is not loaded
catch {
    ttk_topwin_register $atree_top
} trash

#default units are pixels
set tree_def(units) ""
set tree_def(xsize) 30
set tree_def(ysize) 30
#default shapesize is the radius of a circle, or half the side of a square...
set tree_def(shapesize) 9
#size (radius) of decorative dots.  0 means no dot
set tree_def(dotsize) 0
#Number of grid units to leave blank for text on right side of tree.
set tree_def(rightblank) 8
proc reset_tree_units {} {
    global tree tree_def
    set tree(units) $tree_def(units)
    set tree(xsize) $tree_def(xsize)
    set tree(ysize) $tree_def(ysize)
    set tree(xoffset) [expr $tree(xsize)/2]
    set tree(yoffset) [expr $tree(ysize)/2]
    set tree(shapesize) $tree_def(shapesize)
    set tree(dotsize) $tree_def(dotsize)
    set tree(rightblank) $tree_def(rightblank)
}
reset_tree_units

#virtual node (vnode) item counter
#Always increases, independent of canvas objects.
#First vnode returned will be 1
set vnode_count 0

proc new_vnode {} {
    global vnode_count
    incr vnode_count
    return $vnode_count
}

toplevel $atree_top
wm title $atree_top "Article Tree"
set atree $atree_top.c

proc atree_scroll_cmd {which a b} {
    $which set $a $b
#dbg "atree_scroll_cmd"
    atree_visible_update
}

canvas $atree -relief sunken -borderwidth 2 \
	-xscrollcommand "atree_scroll_cmd $atree_top.hscroll" \
	-yscrollcommand "atree_scroll_cmd $atree_top.vscroll" \
	-xscrollincrement $tree(xsize) \
	-yscrollincrement $tree(ysize)

proc set_canv_scrollsize {x y} {
    global atree tree
    set sreg {0 0}
    lappend sreg [expr $tree(xsize) * $x] [expr $tree(ysize) * $y]
    $atree configure -scrollregion $sreg
}
set_canv_scrollsize 1 1


###Tree enabling/disabling toggle   (XXX: could be done better)
#default state is on
set atree_enabled 1

proc atree_enabled_text_set {} {
    global atree_enabled atree_enabled_text
    if {$atree_enabled} {
	set atree_enabled_text "Article tree is ON"
    } else {
	set atree_enabled_text "Article tree is OFF"
    }
}
#Set the initial text
atree_enabled_text_set

button $atree_top.enable -text $atree_enabled_text \
	-command { set atree_enabled [expr !$atree_enabled]
	wipetree
	atree_enabled_text_set
	$atree_top.enable configure -text $atree_enabled_text
	}
pack $atree_top.enable -side top -fill x

set msgidlabel_default "<>"
set msgidlabel $msgidlabel_default
label $atree_top.msgid -textvariable msgidlabel
pack $atree_top.msgid -side top -fill x

scrollbar $atree_top.hscroll -orient horiz -command "$atree xview"
pack $atree_top.hscroll -side bottom -fill x
scrollbar $atree_top.vscroll -command "$atree yview"
pack $atree_top.vscroll -side right -fill y

pack $atree -expand yes -fill both
bind $atree <2> "$atree scan mark %x %y"
bind $atree <B2-Motion> {
	$atree scan dragto %x %y
#	atree_visible_update
}

#calculate visible rectangle
set atree_vis_minx -1
set atree_vis_miny -1
set atree_vis_maxx -1
set atree_vis_maxy -1
#old visibility
set atree_vis2_minx -1
set atree_vis2_miny -1
set atree_vis2_maxx -1
set atree_vis2_maxy -1

proc atree_visible_update {} {
    global atree_vis_minx atree_vis_miny atree_vis_maxx atree_vis_maxy
    global atree_vis2_minx atree_vis2_miny atree_vis2_maxx atree_vis2_maxy
    atree_visible_region
    if {($atree_vis_minx != $atree_vis2_minx) || \
	($atree_vis_miny != $atree_vis2_miny) || \
	($atree_vis_maxx != $atree_vis2_maxx) || \
	($atree_vis_maxy != $atree_vis2_maxy)} {
#dbg "vis:($atree_vis_minx,$atree_vis_miny)->($atree_vis_maxx,$atree_vis_maxy)"
#do complex things here
	set atree_vis2_minx $atree_vis_minx
	set atree_vis2_miny $atree_vis_miny
	set atree_vis2_maxx $atree_vis_maxx
	set atree_vis2_maxy $atree_vis_maxy
    }
}

proc atree_visible_region {} {
    global atree tree
    global atree_vis_minx atree_vis_miny atree_vis_maxx atree_vis_maxy
    global ttk_tree_x ttk_tree_y
    set tmp [$atree xview]
    set x0 [lindex $tmp 0]
    set x1 [lindex $tmp 1]
    set xwidth [expr $ttk_tree_x + $tree(rightblank)]
    set atree_vis_minx [expr floor($x0*$xwidth)]
    set atree_vis_maxx [expr ceil($x1*$xwidth)]
    set tmp [$atree yview]
    set y0 [lindex $tmp 0]
    set y1 [lindex $tmp 1]
    set yheight $ttk_tree_y
    set atree_vis_miny [expr floor($y0*$yheight)]
    set atree_vis_maxy [expr ceil($y1*$yheight)]
}

#This doesn't really work, as the visibility bounds were meant
#to find which elements *might* **possibly** be visible.
proc atree_make_visible {x y} {
    global atree tree
    global atree_vis_minx atree_vis_miny atree_vis_maxx atree_vis_maxy

#    dbg "make_vis: enter ($x,$y)"
    atree_visible_update
    while {$x<$atree_vis_minx} {
	$atree xview scroll -1 pages
	atree_visible_update
    }
    while {$x>$atree_vis_maxx} {
	$atree xview scroll 1 pages
	atree_visible_update
    }
    while {$y<$atree_vis_miny} {
	$atree yview scroll -1 pages
	atree_visible_update
    }
    while {$y>$atree_vis_maxy} {
	$atree yview scroll 1 pages
	atree_visible_update
    }
}


#coordinates are graphical.
proc draw_parent_line {ap x y px py} {
    global atree tree
    set xmid [expr $px + (($x-$px)/2)]
    set tmpline [$atree create line $x $y $xmid $y]
    $atree lower $tmpline
    #later: optional dot at joint.
    if {![$ap flag nextsibling]} {
	#If this is the last sibling, draw the attachment
	#to the parent article...
	set tmpline [$atree create line $px $py $xmid $py $xmid $y]
	$atree lower $tmpline
	#later: optional dot at joint.
    }		
}

# draws the "cursor" for the current article.  Might later have other
# effect, or even allow choice between effects.

set atree_cursoritem1 -1
set atree_cursoritem2 -1
set atree_cursorexists 0

#coordinates are grid coords.
proc plot_cursor {gx gy} {
    global atree node_msgid tree atree_cursoritem1 atree_cursorexists
    set sx $tree(xsize)
    set sy $tree(ysize)
    if {$atree_cursorexists} {
	unplot_cursor
    }
    set x [expr $gx * $tree(xsize)]
    set y [expr $gy * $tree(ysize)]
    set atree_cursoritem1 [$atree create rectangle $x $y \
	[expr $x+$sx] [expr $y+$sy] -fill lightgreen]
    $atree lower $atree_cursoritem1
    set atree_cursorexists 1
#XXX Make this auto-scroll optional
#    atree_make_visible $gx $gy
}

proc unplot_cursor {} {
    global atree node_msgid tree
    global atree_cursoritem1 atree_cursoritem2 atree_cursorexists
    if {$atree_cursoritem1>=0} {
	$atree delete $atree_cursoritem1
    }
    if {$atree_cursoritem2>=0} {
	$atree delete $atree_cursoritem2
    }
    set atree_cursoritem1 -1
    set atree_cursoritem2 -1
    set atree_cursorexists 0
}

#returns the canvas itemnumber for the shape
proc tree_makeshape {ap gx gy} {
    global tree atree
    set s $tree(shapesize)
    set x [expr $gx * $tree(xsize) + $tree(xoffset)]
    set y [expr $gy * $tree(ysize) + $tree(yoffset)]
    set shape rectangle
    set fillcol white

    if {![$ap flag exists]} {
	set fillcol gray75
    }
    if {![$ap flag unread]} {
	set shape oval
    }
#XXX testing
    set score [$ap score]
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

    set new [$atree create $shape [expr $x-$s] [expr $y-$s] \
	[expr $x+$s] [expr $y+$s] -outline black -width 3 \
			-fill $fillcol -tags node]
    return $new
}

proc mkNode {ap gx gy px py} {
    global vnode atree node_msgid tree
    set letter " "

    #Get a new virtual node
    set vn [new_vnode]
    #Set grid coordinates:
    set vnode($vn.x) $gx
    set vnode($vn.y) $gy
    set x [expr $gx * $tree(xsize) + $tree(xoffset)]
    set y [expr $gy * $tree(ysize) + $tree(yoffset)]

#uncomment the next line to see approximately how much time is taken by
#actually drawing the items...
#return
    set new [tree_makeshape $ap $gx $gy]

    set vnode($new.virt) $vn
    set vnode($vn.shape) $new
#Selection code is not very usable now...
#    if {[$ap flag selectedonly] && ![$ap flag selected]} {
#	$atree itemconfigure $new -stipple gray25
#	$atree itemconfigure $new -fill blue
#    }
    if {1} {
#redo the letter code soon
	set letter [$ap subjectletter]
	set newtxt [$atree create text $x $y -text $letter -fill black \
	-tags node]
	set vnode($newtxt.virt) $vn
	set vnode($vn.txt) $newtxt
    }

    set id [$ap header msgid]
    set vnode($vn.id) $id
    #Mapping from ID to virtual node (can allow quick update)
#XXX What if there are bad IDs or shared ones?
    set vnode($id.virt) $vn

#XXX consider making the parent lines part of a vnode...
    if {($px>=0) && [$ap flag parent]} {
	set px [expr $px * $tree(xsize) + $tree(xoffset)]
	set py [expr $py * $tree(ysize) + $tree(yoffset)]
	draw_parent_line $ap $x $y $px $py
    }
    #if subject is different than parent, show it to the right.
    if {![$ap flag parentsubject]} {
	#set x to next node location over to right
	set x [expr $x + $tree(xsize) - $tree(shapesize)]
	set subj [$ap header subject]
	set subjtxt [$atree create text $x $y -text $subj -fill black \
	    -anchor w -tags node]
	set vnode($subjtxt.virt) $vn
	set vnode($vn.subjtxt) $subjtxt
    }
}

set atree_cached ""
proc wipetree {} {
    global atree vnode node_msgid
    global atree_vis2_minx atree_vis2_miny atree_vis2_maxx atree_vis2_maxy
    global msgidlabel msgidlabel_default
    global atree_cached

    set atree_cached ""
    set msgidlabel $msgidlabel_default
    $atree delete all
    catch {
	if {[info exists node_msgid]} {
	    unset node_msgid
	}
	if {[info exists vnode]} {
	    unset vnode
#For now, do not reuse the count
#	    set vnode_count 0
	}
    } trash
    set_canv_scrollsize 1 1
    unplot_cursor
    set atree_vis2_minx -1
    set atree_vis2_miny -1
    set atree_vis2_maxx -1
    set atree_vis2_maxy -1
}

$atree bind node <Enter> {
    set curNode [$atree find withtag current]
    if {$curNode == ""} {
	break
    }
    #This array index must exist for all node-tagged canvas items
    set tmp_vn $vnode($curNode.virt)
    set tmp_shapenode $vnode($tmp_vn.shape)
    $atree itemconfigure $tmp_shapenode -outline red
    set msgidlabel $vnode($tmp_vn.id)
}

$atree bind node <Leave> {
    set curNode [$atree find withtag current]
    if {$curNode == ""} {
	break
    }
    #This array index must exist for all node-tagged canvas items
    set tmp_vn $vnode($curNode.virt)
    set tmp_shapenode $vnode($tmp_vn.shape)
    $atree itemconfigure $tmp_shapenode -outline black
    set msgidlabel $msgidlabel_default
}

####NOTE: this procedure is *required* by the treeprepare command!
proc tree_nodesize {ap} {
    set n 0
    if {![$ap flag parentsubject]} {
	incr n 1
    }
    return $n
}

####NOTE: this procedure is *required* by the treechange command!
proc tree_nodechanged {ap} {
    global vnode atree

    set id [$ap header msgid]
    if {[info exists vnode($id.virt)]} {
	set vn $vnode($id.virt)
    } else {
	#if there is no virtual node, then there is nothing to update
	return
    }
#    dbg "nodechanged: id |$id| vnode |$vn|"

#XXX later see if replacing the shape is always necessary?
    set oldshape $vnode($vn.shape)
    set oldoutline [$atree itemcget $oldshape -outline]
    #delete the old shape
    $atree delete $oldshape
    unset vnode($oldshape.virt)

    set gx $vnode($vn.x)
    set gy $vnode($vn.y)
    set txt $vnode($vn.txt)
    #re-make the shape and relink it
    set new [tree_makeshape $ap $gx $gy]
    $atree itemconfigure $new -outline $oldoutline
    set vnode($new.virt) $vn
    set vnode($vn.shape) $new
    #put the shape below the txt label
    $atree lower $new $txt
#    dbg "nodechanged: oldshape |$oldshape| newshape |$new|"

}

#mangles ap
proc draw_tree_main {ap x y px py} {
    while 1 {
	mkNode $ap $x $y $px $py
#	  update_node $ap $x $y $px $py
	set ns [tree_nodesize $ap]
	if {[$ap flag child]} {
	    set apchild [$ap copy]
	    $apchild move child
	    set y [draw_tree_main $apchild [expr $x+1] [expr $y+$ns] $x $y]
	    rename $apchild ""
	} elseif {$ns == 0} {
	    incr y 1
	} else {
	    incr y $ns
	}
	set tmp [$ap move sibling]
	if {$tmp != 1} {
	    #return y-coord. if no sibling
	    return $y
	}
    }
}

#Use a single global item so that we don't have to keep creating/destroying
#article objects for every cursor plot.
set trn_current_cursor_ap [trn_article]
proc trn_current_cursor {} {
    global vnode trn_current_cursor_ap
    set ap $trn_current_cursor_ap
    set tmp [$ap setcurrent]

    if {$tmp>0} {
	set vn 0
	set id [$ap header msgid]
	if {[info exists vnode($id.virt)]} {
	    set vn $vnode($id.virt)
	}
	if {$vn>0} {
	    plot_cursor $vnode($vn.x) $vnode($vn.y)
	}
    }
}

proc draw_article_tree {ap x y} {
    global tree ttk_tree_x ttk_tree_y atree_enabled
    global atree_cached
#testing...
    global atree

#Check to see if tree article is same as last one...
    if {$atree_enabled} {
	if {[$ap header msgid] == $atree_cached} {
	    atree_visible_region
	    #call update procedure for changed articles.
	    $ap treechange
	} else {
	    set tmpap [$ap copy]
	    wipetree
	    $ap treeprepare
#	    set_canv_scrollsize [expr $ttk_tree_x+$tree(rightblank)] \
#		[expr $ttk_tree_y+1]
#	    atree_visible_region
	    draw_tree_main $tmpap $x $y -1 -1
	    set atree_cached [$ap header msgid]
	    #Delete the temporary article copy
	    rename $tmpap ""
#testing...
	    $atree configure -scrollregion [$atree bbox all]
	}
	trn_current_cursor
    }
}

proc trn_draw_article_tree {x y} {
    global ttk_msgid
    set ap [trn_article]
    $ap setid $ttk_msgid
    draw_article_tree $ap $x $y
    rename $ap ""
}

#XXX move this around somewhere else later...
proc endgroup {} {
    wipetree
    update idletasks
}

#XXX testing...


#later handle 's' (scan mode), 't' thread selector in appropriate ways...
#Within the current group, attempt to go to an article
#(if the article is<=0, do nothing)
proc trn_go_article {anum} {
    global ttk_keys
    if {$anum>0} {
	#if the article number is good...
	set trn_mode [trn_misc mode]
#dbg "mode: $trn_mode"
	if {(($trn_mode == "t"))} {
	    #In the thread tree, so leave the tree
	    append ttk_keys "+"
	}
	if {(($trn_mode == "p") || ($trn_mode == "a") || ($trn_mode == "e")
	    || ($trn_mode == "t"))} {
	    #if we are in the newsgroup
	    append ttk_keys $anum
	    append ttk_keys "\n"
	}
    }
}

set trn_treeclick_ap [trn_article]
$atree bind node <1> {
    global trn_treeclick_ap atree vnode

    set curNode [$atree find withtag current]
    if {$curNode == ""} {
	break
    }

    #This array index must exist for all node-tagged canvas items
    set tmp_vn $vnode($curNode.virt)

    $trn_treeclick_ap setid $vnode($tmp_vn.id)
    #the following is safe even if the id is not found]
    set anum [$trn_treeclick_ap artnum]
    trn_go_article $anum
    #possibly later do things like update the tree, etc...    
}
