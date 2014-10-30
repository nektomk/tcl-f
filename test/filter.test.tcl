package require f
namespace import ::f::*
source testcase.tcl

testcase filter-simple "simple filter" {
	filter x { expr $x > 5 } { 1 2 3 4 5 6 7 8 9 10 }
} {6 7 8 9 10}
