package require f
namespace import f::*
source testcase.tcl

testcase func-decl { create function from declaration } {
	::f::func {x y} { expr $x+$y } 1 
} {::apply {{x y} { expr $x+$y }} 1}

testcase func-decl-ns { create function with namespace } {
	namespace eval foo {}
	::f::func @foo {x y} { expr $x+$y } 1 
} {::apply {{x y} { expr $x+$y } ::foo} 1}

testcase func-decl-apply { create function from apply stmt} {
	::f::func apply {{x y} { expr $x+$y }} 1
} {::apply {{x y} { expr $x+$y }} 1}

testcase func-decl-apply-ns1 { create function from apply stmt with namespace,variant 1} {
	namespace eval foo {}
	::f::func apply {{x y} { expr $x+$y } foo} 1
} {::apply {{x y} { expr $x+$y } ::foo} 1}

testcase func-decl-apply-ns2 { create function from apply stmt with namespace,variant 2} {
	namespace eval foo {}
	::f::func @foo apply {{x y} { expr $x+$y }} 1
} {::apply {{x y} { expr $x+$y } ::foo} 1}

testcase func-decl-apply-conflict { create funstion from apply: namespace conflict} {
	namespace eval foo {}
	namespace eval bar {}
	::f::func @bar apply {{x y} { expr $x+$y } foo} 1
} -code error -match glob -result *

testcase func-decl-command { create function from existing command } {
	func puts "hello word!!"
} {::puts {hello word!!}}

testcase func-decl-command-ns { create function from existing command with ns} {
	namespace eval foo {}
	func @foo puts "hello word!!"
} {::puts {hello word!!}}

testcase func-invoke {function invocation} {
	set f [ func x { incr x } ]
	{*}$f 1
} {2}

testcase func-invoke-curry {function invocation with curry} {
	set f [ func {x y} { expr $x + $y } 2 ]
	{*}$f 1
} {3}

testcase func-invoke-ns1 {function invoke with ns,variant1} {
	namespace eval foo { set z 3 }
	set f [ func @foo { x y } { variable z; expr $x + $y + $z } ]
	{*}$f 1 2
} {6}

testcase func-invoke-ns2 {function invoke with ns,variant1} {
	namespace eval foo { set z 3 }
	set f [ func apply {{ x y } { variable z; expr $x + $y + $z } foo} ]
	{*}$f 1 2
} {6}

