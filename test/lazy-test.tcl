#!/usr/bin/tclsh
puts "partial test \"lazy\" features, but impossible to test it via tcltest"
proc test { name descr body } {
	puts -nonewline "$name ($descr)"
	if [ catch {
		uplevel $body
	} ret code ] {
		puts "fail"
		puts $code
	} else {
		puts "ok"
	}
}
set owd [pwd]
cd ..
lappend auto_path [pwd]
cd $owd

package require f
namespace import ::f::* 

test lazy-wo-args "lazy without args" {
	lazy
}
test lazy-simple "lazy simple" {
	lazy list 1 2 3
}

