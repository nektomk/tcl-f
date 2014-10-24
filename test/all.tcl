#!/usr/bin/env tclsh
if [ catch {
    # attempt to load installed package 
    package require f
} ] {
    # see package from ../f
    set wd [ pwd ]
    cd ..
    set see [ file join [pwd] "f" "pkgIndex.tcl" ]
    puts $see
    if { [ file readable $see ] } {
        lappend auto_path [ pwd ]
    } else {
        puts stderr "package \"f\" does not exists"
        exit 1
    }
    if [ catch {
	set env(TCLLIBPATH) $env(TCLLIBPATH):$[pwd]
    } ] { catch {
	set env(TCLLIBPATH) [pwd]
    } }
    cd $wd
    package require f
}

package require tcltest
namespace import tcltest::*
configure -file *.test.tcl -verbose psbe -debug 0
configure {*}$argv
runAllTests
cleanupTests
