#This is a crunched version of obtcl 0.54 with the JLV patches.
#The obtcl package was created by Patrik Floding (see the notice below)
#The original patches were found at:
#    ftp://ftp.dynas.se/pub/tcl
#The patches were found at:
#    http://www.osf.org/~loverso/tcl-tk/obTcl/
#
#If you wish to play with obTcl, I highly recommend getting the distribution,
#as its source code has extensive comments and even manual pages.
#
#The following copyright was included in the distribution:
#[none of the tcl_cruncher files are included in trn]
#Copyright (C) 1995 DynaSoft AB.
#
#This software is copyrighted by DynaSoft AB, Sweden.  It is provided
#courtesy of the author, Patrik Floding, and DynaSoft AB.
#The copyright notice below applies to all included files except
#the files under the directory 'tcl_cruncher', which are separately
#copyrighted (see tcl_cruncher/LICENSE).
#
#The authors hereby grant permission to use, copy, modify, distribute,
#and license this software and its documentation for any purpose, provided
#that existing copyright notices are retained in all copies and that this
#notice is included verbatim in any distributions. No written agreement,
#license, or royalty fee is required for any of the authorized uses.
#Modifications to this software may be copyrighted by their authors
#and need not follow the licensing terms described here, provided that
#the new terms are clearly indicated on the first page of each file where
#they apply.
#
#IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
#FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
#ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
#DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
#POSSIBILITY OF SUCH DAMAGE.
#
#THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
#INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
#IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
#NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
#MODIFICATIONS.

proc cmt { args } {}
proc Nop {} {}
proc setIfNew { var val } {global $var
if ![info exists $var] {set $var $val}}
proc crunch_skip { args } {}
proc lappendUniq { v val } {upvar $v var
if { [lsearch $var $val] != -1 } {return}
lappend var $val}
proc listMinus { a b } {set ret {}
foreach i $a {set ArrA($i) 1}
foreach i $b {set ArrB($i) 1}
foreach i [array names ArrA] {if ![info exists ArrB($i)] {lappend ret $i}}
return $ret}
set _otReferenceSBD {
    global tkPriv
    set tkPriv(relief) [$w cget -activerelief]
    $w configure -activerelief sunken
    set element [$w identify $x $y]
    if {$element == "slider"} {
	tkScrollStartDrag $w $x $y
    } else {
	tkScrollSelect $w $element initial
    }
}
proc otTkScrollButtonDown {w x y} {global tkPriv
set tkPriv(relief) [$w cget -activerelief]
set element [$w identify $x $y]
if {$element == "slider"} {tkScrollStartDrag $w $x $y} else {$w configure -activerelief sunken
tkScrollSelect $w $element initial}}
proc StrictMotif {} {global tk_version tk_strictMotif _otReferenceSBD
set tk_strictMotif 1
if { $tk_version == 4.0 ||
		[string compare [info body tkScrollButtonDown]  [set _otReferenceSBD]] == 0 } {if {[info procs otTkScrollButtonDown] != ""} {rename tkScrollButtonDown {}
rename otTkScrollButtonDown tkScrollButtonDown}}}
proc dbputs { s } {}
proc DOC { name rest } {}
proc DOC_get_list {} {}
set obtcl_version "0.54"
proc instvar2global { name } {upvar 1 class class self self
return _oIV_${class}:${self}:$name}
proc classvar { args } {uplevel 1 "foreach _obTcl_i [list $args] {
		upvar #0 _oDCV_\${class}:\$_obTcl_i \$_obTcl_i
	}"}
proc classvar_of_class { class args } {uplevel 1 "foreach _obTcl_i [list $args] {
		upvar #0 _oDCV_${class}:\$_obTcl_i \$_obTcl_i
	}"}
proc iclassvar { args } {uplevel 1 "foreach _obTcl_i [list $args] {
		upvar #0 _oICV_\${iclass}:\$_obTcl_i \$_obTcl_i
	}"}
proc instvar_of_class { class args } {uplevel 1 "foreach _obTcl_i [list $args] {
		upvar #0 _oIV_${class}:\${self}:\$_obTcl_i \$_obTcl_i
	}"}
proc instvar { args } {uplevel 1 "foreach _obTcl_i [list $args] {
		upvar #0 _oIV_\${class}:\${self}:\$_obTcl_i \$_obTcl_i
	}"}
proc import { class args } {uplevel 1 "foreach _obTcl_i [list $args] {
		upvar #0 _oIV_${class}:\${self}:\$_obTcl_i \$_obTcl_i
	}"}
proc renamed_instvar { normal_name new_name } {uplevel 1 "upvar #0 _oIV_\${class}:\${self}:$normal_name $new_name"}
proc is_object { name } {global _obTcl_Objects
if [info exists _obTcl_Objects($name)] {return 1} else {return 0}}
proc is_class { name } {global _obTcl_Classes
if [info exists _obTcl_Classes($name)] {return 1} else {return 0}}
proc otNew { iclass obj args } {global _obTcl_Objclass _obTcl_Objects
set _obTcl_Objclass($iclass,$obj) $obj
if ![info exists _obTcl_Objects($obj)] {catch {rename $obj ${obj}-cmd}}
set _obTcl_Objects($obj) 1
proc $obj { cmd args } "
		set self $obj
		set iclass $iclass

		if \[catch {eval {$iclass::\$cmd} \$args} val\] {
			return -code error  -errorinfo \"$obj: \$val\" \"$obj: \$val\"
		} else {
			return \$val
		}
	"
set self $obj
eval {$iclass::initialize}
eval {$iclass::init} $args}
proc otInstance { iclass obj args } {global _obTcl_Objclass _obTcl_Objects
set _obTcl_Objclass($iclass,$obj) $obj
if {[info procs ${obj}-cmd] == ""} {catch {rename $obj ${obj}-cmd}}
set _obTcl_Objects($obj) 1
proc $obj { cmd args } "
		set self $obj
		set iclass $iclass

		if \[catch {eval {$iclass::\$cmd} \$args} val\] {
			return -code error  -errorinfo \"$obj: \$val\" \"$obj: \$val\"
		} else {
			return \$val
		}
	"
set self $obj
eval {$iclass::initialize}}
proc otFreeObj { obj } {global _obTcl_Objclass _obTcl_Objects
otGetSelf
catch {uplevel #0 "eval unset _obTcl_Objclass($iclass,$obj)  _obTcl_Objects($obj)  \[info vars _oIV_*:${self}:*\]"}
catch {rename $obj {}}}
setIfNew _obTcl_Classes() ""
setIfNew _obTcl_NoClasses 0
proc class { class } {global _obTcl_NoClasses _obTcl_Classes _obTcl_Inherits
if [info exists _obTcl_Classes($class)] {set self $class
otClassDestroy $class}
if [string match "*:*" $class] {puts stderr "class: Fatal Error:"
puts stderr "       class name `$class' contains reserved character `:'"
return}
incr _obTcl_NoClasses 1
set _obTcl_Classes($class) 1
set iclass $class
set obj $class
proc $class { cmd args } "
		set self $obj
		set iclass $iclass

		switch -glob \$cmd {
		.*		{ eval {$class::new \$cmd} \$args }
		new		{ eval {$class::new} \$args }
		method		{ eval {otMkMethod N $class} \$args}
		inherit		{ eval {otInherit $class} \$args}
		destroy		{ eval {otClassDestroy $class} \$args }
		init		{ return -code error  -errorinfo \"$obj: Error: classes may not be init'ed!\"  \"$obj: Error: classes may not be init'ed!\"
		}
 		default		{
			if \[catch {eval {$iclass::\$cmd} \$args} val\] {
				return -code error  -errorinfo \"$obj: \$val\" \"$obj: \$val\"
			} else {
				return \$val
			}
		 }
		}
	"
if { "$class" != "Base" } {$class inherit "Base"} else {set _obTcl_Inherits($class) {}}
return $class}
proc otClassDestroy { class } {global _obTcl_NoClasses _obTcl_Classes
otGetSelf
if ![info exists _obTcl_Classes($class)] {return}
otInvalidateCaches 0 $class [otClassInfoMethods $class]
otDelAllMethods $class
rename $class {}
incr _obTcl_NoClasses -1
unset _obTcl_Classes($class)
uplevel #0 "
		foreach _iii  \[info vars _oICV_${class}:*\] {
			unset \$_iii
		}
		foreach _iii  \[info vars _oDCV_${class}:*\] {
			unset \$_iii
		}
		catch {unset _iii}
	"
otFreeObj $class}
proc otGetSelf {} {uplevel 1 {upvar 1 self self iclass iclass Umethod method}}
proc otMkMethod { mode class name params body } {otInvalidateCaches 0 $class $name
if { "$name" == "unknown" } {set method ""} else {set method "set method $name"}
proc $class::$name $params "upvar 1 self self iclass iclass Umethod method
set class $class
$method

$body"
if { $mode == "S" } {global _obTcl_SysMethod
set _obTcl_SysMethod($class::$name) 1}}
proc otRmMethod { class name } {global _obTcl_SysMethod
if { "$name" == "unknown" } {otInvalidateCaches 0 $class *} else {otInvalidateCaches 0 $class $name}
rename $class::$name {}
catch {unset _obTcl_SysMethod($class::$name)}}
proc otDelAllMethods { class } {global _obTcl_Cached
foreach i [info procs $class::*] {if [info exists _obTcl_SysMethod($i)] {continue}
if [info exists _obTcl_Cached($i)] {unset _obTcl_Cached($i)}
rename $i {}}}
proc otObjInfoVars { glob base { match "" } } {if { "$match" == "" } {set match "*"}
set l [info globals ${glob}$match]
set all {}
foreach i $l {regsub "${base}(.*)" $i {\1} tmp
lappend all $tmp}
return $all}
proc otObjInfoObjects { class } {global _obTcl_Objclass
set l [array names _obTcl_Objclass $class,*]
set all {}
foreach i $l {regsub "${class},(.*)" $i {\1} tmp
lappend all $tmp}
return $all}
proc otClassInfoBody { class method } {global _obTcl_Objclass _obTcl_Cached
if [info exists _obTcl_Cached(${class}::$method)] {return}
if [catch {set b [info body ${class}::$method]} ret] {return -code error -errorinfo "info body: Method '$method' not defined in class $class" "info body: Method '$method' not defined in class $class"} else {return $b}}
proc otClassInfoArgs { class method } {global _obTcl_Objclass _obTcl_Cached
if [info exists _obTcl_Cached(${class}::$method)] {return}
if [catch {set b [info args ${class}::$method]} ret] {return -code error -errorinfo "info args: Method '$method' not defined in class $class" "info args: Method '$method' not defined in class $class"} else {return $b}}
proc otClassInfoMethods+Cached { class } {global _obTcl_Objclass _obTcl_SysMethod
set l [info procs ${class}::*]
set all {}
foreach i $l {regsub "${class}::(.*)" $i {\1} tmp
if [info exists _obTcl_SysMethod($i)] {continue}
lappend all $tmp}
return $all}
proc otClassInfoMethods { class } {global _obTcl_Objclass _obTcl_Cached _obTcl_SysMethod
set l [info procs ${class}::*]
set all {}
foreach i $l {if [info exists _obTcl_Cached($i)] {continue}
if [info exists _obTcl_SysMethod($i)] {continue}
regsub "${class}::(.*)" $i {\1} tmp
lappend all $tmp}
return $all}
proc otClassInfoSysMethods { class } {global _obTcl_Objclass _obTcl_Cached _obTcl_SysMethod
set l [info procs ${class}::*]
set all {}
foreach i $l {if [info exists _obTcl_Cached($i)] {continue}
if ![info exists _obTcl_SysMethod($i)] {continue}
regsub "${class}::(.*)" $i {\1} tmp
lappend all $tmp}
return $all}
proc otClassInfoCached { class } {global _obTcl_Objclass _obTcl_Cached _obTcl_SysMethod
if ![array exists _obTcl_Cached] {return}
set l [array names _obTcl_Cached $class::*]
set all {}
foreach i $l {regsub "${class}::(.*)" $i {\1} tmp
if [info exists _obTcl_SysMethod($i)] {continue}
lappend all $tmp}
return $all}
proc obtcl_mkindex {dir args} {global errorCode errorInfo
set oldDir [pwd]
cd $dir
set dir [pwd]
append index "# Tcl autoload index file, version 2.0\n"
append index "# This file is generated by the \"obtcl_mkindex\" command\n"
append index "# and sourced to set up indexing information for one or\n"
append index "# more commands/classes.  Typically each line is a command/class that\n"
append index "# sets an element in the auto_index array, where the\n"
append index "# element name is the name of a command/class and the value is\n"
append index "# a script that loads the command/class.\n\n"
foreach file [eval glob $args] {set f ""
set error [catch {
	    set f [open $file]
	    while {[gets $f line] >= 0} {
		if [regexp {^(proc|class)[ 	]+([^ 	]*)} $line match dummy entityName] {
		    append index "set [list auto_index($entityName)]"
		    append index " \"source \$dir/$file\"\n"
		}
	    }
	    close $f
	} msg]
if $error {set code $errorCode
set info $errorInfo
catch {close $f}
cd $oldDir
error $msg $info $code}}
set f [open tclIndex w]
puts $f $index nonewline
close $f
cd $oldDir}
proc otPrInherits {} {global _obTcl_Classes
foreach i [array names _obTcl_Classes] {puts "$i inherits from: [$i inherit]"}}
proc otInherit { class args } {global _obTcl_Inherits
if { "$args" == "" } {return [set _obTcl_Inherits($class)]}
if { "$class" != "Base" && [lsearch $args "Base"] == -1 } {set args [concat $args "Base"]}
if { [info exists _obTcl_Inherits($class)] == 1 } {otInvalidateCaches 0 $class [otClassInfoCached ${class}]} else {set _obTcl_Inherits($class) {}}
set _obTcl_Inherits($class) $args}
proc otInvalidateCaches { level class methods } {global _obTcl_CacheStop
foreach i $methods {if {"$i" == "unknown" } {set i "*"}
set _obTcl_CacheStop($i) 1}
if [array exists _obTcl_CacheStop] {otDoInvalidate}}
proc otDoInvalidate {} {global _obTcl_CacheStop _obTcl_Cached
if ![array exists _obTcl_Cached] {unset _obTcl_CacheStop
return}
if [info exists _obTcl_CacheStop(*)] {set stoplist "*"} else {set stoplist [array names _obTcl_CacheStop]}
foreach i $stoplist {set tmp [array names _obTcl_Cached *::$i]
eval lappend tmp [array names _obTcl_Cached *::${i}_next]
foreach k $tmp {catch {
				rename $k {}
				unset _obTcl_Cached($k)
			}}}
if {[array size _obTcl_Cached] == 0} {unset _obTcl_Cached}
unset _obTcl_CacheStop}
if { [info procs otUnknown] == "" } {rename unknown otUnknown}
proc otResolve { class func } {return [otGetFunc 0 $class $func]}
setIfNew _obTcl_unknBarred() ""
proc unknown args {global _obTcl_unknBarred
set name [lindex $args 0]
if [string match "*::*" $name] {set tmp [split $name :]
set class [lindex $tmp 0]
set func [join [lrange $tmp 2 end] :]
set flist [otGetFunc 0 $class $func]
if [string match "" $flist] {if [info exists _obTcl_unknBarred($name)] {return -code error}
set flist [otGetFunc 0 $class "unknown"]}
if ![string match "" $flist] {proc $name args " upvar 1 self self iclass iclass Umethod method
				set Umethod $func
				eval [lindex $flist 0] \$args"} else {proc $name args "
				return -code error -errorinfo \"Undefined method '$func' invoked\"  \"Undefined method '$func' invoked\"
			"}
global _obTcl_Cached
set _obTcl_Cached(${class}::$func) $class
global errorCode errorInfo
set code [catch {uplevel $args} msg]
if {$code ==  1} {set new [split $errorInfo \n]
set new [join [lrange $new 0 [expr [llength $new] - 6]] \n]
return -code error -errorcode $errorCode -errorinfo $new $msg} else {return -code $code $msg}} else {uplevel [concat otUnknown $args]}}
setIfNew _obTcl_Cnt 0
proc otChkCall { cmd var } {global _obTcl_Trace _obTcl_Cnt _obTcl_nextRet
if [info exists _obTcl_Trace($cmd)] {return $_obTcl_nextRet}
set _obTcl_Trace($cmd) 1
catch {uplevel 1 "uplevel 1 \"$cmd \[set $var\]\""} _obTcl_nextRet
return $_obTcl_nextRet}
proc otNextElse {} {uplevel 1 {set all [otGetNextFunc $class $method]
foreach i $all {append tmp "otChkCall $i args\n"}
if [info exists tmp] {proc $class::${method}_next args "
				$tmp
			"} else {proc $class::${method}_next args "
				return
			"}
set _obTcl_Cached(${class}::${method}_next) $class
incr _obTcl_Cnt 1
set ret [catch {uplevel 1 {${class}::${method}_next} $args} val]
incr _obTcl_Cnt -1}}
proc next args {global _obTcl_Cnt _obTcl_Cached _obTcl_nextRet
upvar 1 self self method method class class iclass iclass
if { $_obTcl_Cnt == 0 } {set _obTcl_nextRet ""}
if [info exists _obTcl_Cached(${class}::${method}_next)] {incr _obTcl_Cnt 1
set ret [catch {uplevel 1 {${class}::${method}_next} $args} val]
incr _obTcl_Cnt -1} else {otNextElse}
if { $_obTcl_Cnt == 0 } {global _obTcl_Trace
catch {unset _obTcl_Trace}}
if { $ret != 0 } {return -code error -errorinfo "$self: $val" "$self: $val"} else {return $val}}
proc otGetNextFunc { class func } {global _obTcl_Inherits
set all ""
foreach i [set _obTcl_Inherits($class)] {foreach k [otGetFunc 0 $i $func] {lappendUniq all $k}}
return $all}
proc otGetFunc { depth class func } {global _obTcl_Inherits _obTcl_Cached _obTcl_NoClasses _obTcl_Classes
if { "$depth" > "$_obTcl_NoClasses" } {otGetFuncErr $depth $class $func
return ""}
incr depth
set all ""
if ![info exists _obTcl_Classes($class)] {if ![auto_load $class] {otGetFuncMissingClass $depth $class $func
return ""}}
if { [info procs $class::$func] != "" &&
	     ![info exists _obTcl_Cached(${class}::$func)] } {return "$class::$func"}
foreach i [set _obTcl_Inherits($class)] {set ret [otGetFunc $depth $i $func]
if { $ret != "" } {foreach i $ret {lappendUniq all $i}}}
return $all}
proc otGetFuncErr { depth class func } {puts stderr "GetFunc: depth=$depth, circular dependency!?"
puts stderr "         class=$class func=$func"}
proc otGetFuncMissingClass { depth class func } {puts stderr "GetFunc: Unable to inherit from $class"
puts stderr "         $class not defined (and auto load failed)"
puts stderr "         Occurred while looking for $class::$func"}
class Base
Base method init args {}
Base method destroy args {otFreeObj $self}
Base method class args {return $iclass}
Base method set args {set class $iclass
set var [lindex $args 0]
regexp -- {^([^(]*)\(.*\)$} $var m var
instvar $var
return [eval set $args]}
Base method eval l {return [eval $l]}
Base method info { cmd args } {switch $cmd {
	"instvars"	{return [eval {otObjInfoVars _oIV_${iclass}:${self}: _oIV_${iclass}:${self}:} $args]}
	"iclassvars"	{otObjInfoVars _oICV_${iclass}: _oICV_${iclass}: $args}
	"classvars"	{otObjInfoVars _oDCV_${iclass}: _oDCV_${iclass}: $args}
	"objects"	{otObjInfoObjects $iclass}
	"methods"	{otClassInfoMethods $iclass}
	"sysmethods"	{otClassInfoSysMethods $iclass}
	"cached"	{otClassInfoCached $iclass}
	"body"		{otClassInfoBody $iclass $args}
	"args"		{otClassInfoArgs $iclass $args}

	"options"	{$iclass::collectOptions values ret
			return [array get ret] }
	"defaults"	{$iclass::collectOptions defaults ret
			return [array get ret] }

	default	{
			return -code error  -errorinfo "Undefined command 'info $cmd'"  "Undefined command 'info $cmd'"
		}
	}}
Base method unknown { args } {return -code error -errorinfo "Undefined method '$method' invoked" "Undefined method '$method' invoked"}
Base method new { obj args } {eval {otNew $iclass $obj} $args}
Base method instance { obj args } {eval {otInstance $iclass $obj} $args}
Base method sys_method { args } {eval {otMkMethod S $iclass} $args}
Base method method { args } {eval {otMkMethod N $iclass} $args}
Base method del_method { args } {eval {otRmMethod $iclass} $args}
Base method inherit { args } {eval {otInherit $iclass} $args}
class AnonInst
AnonInst method anonPrefix { p } {
	iclassvar _prefix
	set _prefix $p
}
AnonInst method new {{obj {}} args} {
	iclassvar _count _prefix
	if ![info exists _count] {
		set _count 0
	}
	if ![info exists _prefix] {
		set _prefix "$iclass"
	}
	if [string match "" $obj] {
        	set obj $_prefix[incr _count]
        }
        eval next {$obj} $args
        return $obj
}
proc otMkSectMethod { class name sect } {$class sys_method $name { args } "
		array set Opts \$args
		foreach i \[array names Opts\] {
			global _obTcl_unknBarred
			set _obTcl_unknBarred(\$class::${sect}:\$i) 1
			if \[catch {\$class::$sect:\$i \$Opts(\$i)} err\] {
				if \[catch {\$class::${sect}_unknown \$i \$Opts(\$i)}\] {
					unset _obTcl_unknBarred(\$class::${sect}:\$i)
				  	error \"Unable to do '$sect \$i \$Opts(\$i)'\n \t\$err
					\"
			 	}
			}
			unset _obTcl_unknBarred(\$class::${sect}:\$i)
		}
	"}
proc otMkOptHandl {} {uplevel 1 {$iclass sys_method "cget" { opt } "
		classvar classOptions
		if \[catch {$iclass::cget:\$opt} ret\] {
			if \[catch {\$class::cget_unknown \$opt} ret\] {
			  	error \"Unable to do 'cget \$opt'\"
			}
		}
		return \$ret
	"
otMkSectMethod $iclass conf_init init
$iclass sys_method initialize {} {
		next
		classvar optDefaults
		eval instvar [array names optDefaults]
		foreach i [array names optDefaults] {
			set $i $optDefaults($i)
		}
	}
$iclass sys_method collectOptions { mode arr } {
		classvar classOptions optDefaults

		upvar 1 $arr ret
		next $mode ret
		eval instvar [array names optDefaults]
		foreach i [array names optDefaults] {
			if { $mode == "defaults" } {
				set ret(-$i) $optDefaults($i)
			} else {
				set ret(-$i) [set $classOptions(-$i)]
			}
		}
	}
otMkSectMethod $iclass conf_verify verify
otMkSectMethod $iclass configure configure
set _optPriv(section,cget) 1
set _optPriv(section,init) 1
set _optPriv(section,initialize) 1
set _optPriv(section,verify) 1
set _optPriv(section,configure) 1}}
otMkSectMethod Base configure configure
Base method option { opt dflt args } {classvar_of_class $iclass optDefaults classOptions _optPriv
set var [string range $opt 1 end]
set optDefaults($var) $dflt
set classOptions($opt) $var
array set tmp $args
if ![info exists _optPriv(initialize)] {otMkOptHandl
set _optPriv(initialize) 1}
foreach i [array names tmp] {if ![info exists _optPriv(section,$i)] {otMkSectMethod $iclass $i $i
set _optPriv(section,$i) 1}
$iclass sys_method "$i:$opt" _val "
			instvar $var
			set _old_val \$[set var]
			set $var \$_val
			set ret \[catch {$tmp($i)} res\]
			if {\$ret != 0 && \$ret != 2 } {
				set $var \$_old_val
				return -code \$ret -errorinfo \$res \$res
			}
			return \$res
		"
set _optPriv($i:$opt) 1}
if ![info exists _optPriv(cget:$opt)] {$iclass sys_method "cget:$opt" {} "
			instvar $var
			return \$[set var]
		"
set _optPriv(cget:$opt) 1}
if ![info exists tmp(verify)] {$iclass sys_method "verify:$opt" _val "
			instvar $var
			set $var \$_val
		"
set _optPriv(verify:$opt) 1}
if ![info exists tmp(configure)] {$iclass sys_method "configure:$opt" _val "
			instvar $var
			set $var \$_val
		"
set _optPriv(configure:$opt) 1}
if ![info exists tmp(init)] {$iclass sys_method "init:$opt" _val {}
set _optPriv(init:$opt) 1}}
Base sys_method init_unknown { opt val } {}
Base sys_method verify_unknown { opt val } {}
Base sys_method initialize {} {}
Base sys_method conf_init {} {}
class Widget
Widget method init { args } {instvar win wincmd
next
set first "[lindex $args 0]"
set c1 "[string index $first 0]"
if { "$c1" == "" || "$c1" == "-" } {set type frame
set cl "-class $iclass"} else {set type $first
set args [lrange $args 1 end]
set cl ""}
if {[info commands $self-cmd] != ""} {set win $self
set wincmd $self-cmd} else {if { [string index $self 0] == "." } {rename $self _ooTmp
eval $type $self $cl $args
rename $self $self-cmd
rename _ooTmp $self
set win $self
set wincmd $self-cmd} else {eval $type .$self $cl $args
set win .$self
set wincmd .$self}}
bind $win <Destroy> " if { \"%W\" == \"$self\" && !\[catch {info args $self}\] } {
			$self destroy -obj_only }"
return $self}
Widget sys_method configure { args } {instvar wincmd
eval {$wincmd configure} $args}
Widget sys_method cget { opt } {instvar wincmd
eval {$wincmd cget} $opt}
Widget sys_method configure_unknown { opt args } {instvar wincmd
eval {$wincmd configure $opt} $args}
Widget sys_method cget_unknown { opt } {instvar wincmd
$wincmd cget $opt}
Widget sys_method init_unknown { opt val } {puts "init_unknown: $opt $val (iclass=$iclass class=$class)"}
Widget sys_method unknown args {instvar wincmd
eval {$wincmd $method} $args}
Widget method destroy { args } {instvar win wincmd
set wp $win
set w $wincmd
otFreeObj $self
catch {bind $w <Destroy> {}}
if { $args != "-obj_only" } {if [string compare $w $wp] {rename $w $wp}
if { $args != "-keepwin" } {destroy $wp}}}
Widget sys_method set { args } {instvar wincmd
eval {$wincmd set} $args}
Widget sys_method base_set { args } {eval Base::set $args}
