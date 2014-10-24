#!/bin/env tclsh
lappend auto_path [pwd]
package require f
namespace import ::f::*

set x [ lazy {
	puts "hello"
	expr 1 + 2
} ]

concat "$x $x"

