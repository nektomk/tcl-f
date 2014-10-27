package require f
namespace import ::f::*
source testcase.tcl

testcase chain-function { call a chain of functions } {
	set f [ func x { expr $x + 1} ]
	set g [ func x { expr $x * 2 } ]
	set h [ func x { expr $x / 3.0 } ]
	set fgh [ list $f $g $h ]
	set hgf [ list $h $g $f ]
	list [ chain $fgh 11 ] [ chain $hgf 30 ]
} {8.0 21.0}

testcase chain-curry { call a chain with curry } {
	set mid [ func {x y} {expr ($x + $y)/2.0} ]
	set add2 [ func x { expr $x+1} ]
	chain [ list $mid $add2 ] 23 17
} {21.0}


