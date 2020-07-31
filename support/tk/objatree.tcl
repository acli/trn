#object-oriented replacement for the old article tree code

#XXX Later move tree outside...
source $script_dir/Tree.tcl
source $script_dir/ArtTree.tcl

ArtTree .articleTree -toplevel 1

#XXX later this will be fixed up:
.articleTree configure -title "Article Tree"

#Procedures in the old style...

proc trn_draw_article_tree {x y} {
    global ttk_msgid
    .articleTree draw $ttk_msgid
}

proc endgroup {} {
    .articleTree erase
    update idletasks
}

proc wipetree {} {
    .articleTree erase
}
