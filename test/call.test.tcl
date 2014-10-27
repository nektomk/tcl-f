package require f
namespace import ::f::*
source testcase.tcl

testcase call-func { call already declared func } {
	set f [ func x { incr x } ]
	call $f 1
} {2}

testcase call-func-ntimes { call already declared func n-times} {
	set f [ func x { incr x } ]
	list [ call $f 1 ] [ call $f 1 ] [ call $f 2 ]
} {2 2 3}

testcase call-func-nested { nested calls already declared func } {
	set f [ func x { incr x } ]
	call $f [ call $f [ call $f 1 ] ]
} {4}

testcase call-func-ns { call function with namespace} {
	namespace eval foo {
		set z 11
	}
	set f [ func @foo x { variable z ; expr $z + $x } ]
	call $f 12
} {23}
