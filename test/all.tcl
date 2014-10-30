#!/usr/bin/env tclsh
set owd [ pwd ]
cd ..
set env(TCLLIBPATH) [ pwd ]
lappend auto_path [ pwd ]
cd $owd

package require tcltest
namespace import tcltest::*
configure -file *.test.tcl -verbose psbe -debug 0 -skip lazy-*
configure {*}$argv
runAllTests
cleanupTests
