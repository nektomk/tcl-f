 if {![package vsatisfies [package provide Tcl] 8.5]} {return}
 package ifneeded f 0.0.1 [list apply {dir {
    load [ file join $dir f[info sharedlibextension] ]
    source [ file join $dir f.tcl ]
 }} $dir]
