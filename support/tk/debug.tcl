#debug.tcl
#A sample of a non-object-oriented extension using TrnModule

#later:
#  * horizontal scrollbar for text?

#old way--not recommended in tktrn
#set dbg_frame [toplevel .dbg]

#New way--will allow window position to be saved, other benefits later.
set dbg_frame [TrnModule .dbg -toplevel 1]

text $dbg_frame.t -width 60 -height 12 \
	-yscrollcommand "$dbg_frame.vscroll set"

#No generic toplevel bindings
bindtags $dbg_frame.t [list $dbg_frame.t Text $dbg_frame]

entry $dbg_frame.eval -width 60 -relief sunken -textvariable dbg_eval
bindtags $dbg_frame.eval [list $dbg_frame.eval Entry]
bind $dbg_frame.eval <Return> {dbg [eval $dbg_eval]}

label $dbg_frame.errinfo -textvariable errorInfo -justify left -height 3 \
	-anchor nw
bind $dbg_frame.errinfo <1> \
    {dbg "error info follows:\n$errorInfo\n---end of error info---"}

scrollbar $dbg_frame.vscroll -command "$dbg_frame.t yview"

pack $dbg_frame.eval $dbg_frame.errinfo -side top -expand yes -fill both
pack $dbg_frame.vscroll -side right -fill y
pack $dbg_frame.t -expand yes -fill both

proc dbg {txt} {
    global dbg_frame
    $dbg_frame.t insert end $txt
    $dbg_frame.t insert end "\n"
}
dbg "----start of debug output----"
bind $dbg_frame.t <Control-n> { dbg "--------------------" }
bind $dbg_frame.t <Control-x> { $dbg_frame.t delete 2.0 end }
