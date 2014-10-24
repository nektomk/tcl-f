source testcase.tcl
package require tcltest
package require f
namespace import f::*
testcase tuple-no-args {command w/o args} {tuple} {} -code error
testcase tuple-with-args {command with args} {tuple 1 2 3} {1 2 3}
testcase tuple-of-tuples {tuple of tuples} {tuple [ tuple 1 2 ] 3 4 [ tuple 5 6 ] } {1 2 3 4 5 6}
testcase tuple-of-lists {tuple of lists} {tuple [ list 1 2 ] 3 4 [ list 5 6 ] } {{1 2} 3 4 {5 6}}
testcase tuple-in-list {list with tuples} {list [ tuple 1 2 ] 3 4 [ tuple 5 6 ] } {{1 2} 3 4 {5 6}}
testcase tuple-vars {tuple in variables} {
 	set a [ tuple 1 2 ]
	set b [ tuple 3 4 ]
	set c [ tuple 5 6 ]
	tuple $a $b $c
} {1 2 3 4 5 6}
testcase tuple-subst {substitute tuples in string} {
	set a [ tuple 1 2 3 ]
	set ret "x${a}x"
} {x1 2 3x}
testcase tuple-empty {tuple with empty tuples} {tuple [tuple 1 2] [tuple] [tuple] 3 4 } {1 2 3 4}
testcase tuple-empty-multyple {multiply nested emptyes in tuple} {tuple [tuple] [tuple [tuple] ] [tuple]} {}
testcase tuple-empty-in-tuple {tuple with empty and others} {tuple [tuple] 1 2 [tuple 3 4] [tuple]} {1 2 3 4}


