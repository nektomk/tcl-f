package require f
namespace import ::f::*
source testcase.tcl

testcase fold-simple "simple fold" {
	fold {acc x} { expr $acc + $x } "number" { 1 2 3 }
} {6}
