package require f
namespace import f::*
source testcase.tcl
set setup {
	package require f
	namespace import ::f::*
}
testcase lazy-withot-args "lazy command w/o args" -body {
   set x [lazy]
} -returnCodes error -match glob -result * -setup $setup

testcase lazy-with-args "lazy command with args" -body {
   lazy expr 1 + 2
} -result 3 -setup $setup

testcase lazy-var "lazy to variable" -body {
   set x [ lazy expr 1 + 2 ]
} -result 3 -setup $setup

testcase lazy-subst "lazy substitution" -body {
   set x [ lazy eval {
	list 1} ]
   set y "x$x"
} -result "x1" -setup $setup
#-constraints {winCrash}
   
testcase lazy-once "lazy only once" -body {
   set x [ lazy list 1 2 ]
   puts -nonewline [ concat $x $x ]
} -output {1 2 1 2} -setup $setup

testcase lazy-copy "one lazy in mult. vars" -body {
   set x [ lazy eval {
      list 1 2
   } ]
   set y $x
   set z $y
   concat $x $y $z
} -result {1 2 1 2 1 2} -setup $setup

testcase lazy-lazy "multiple nested lazy`es" -body {
   set x [ lazy lazy lazy eval {
      list 1 2 3 4
   } ]
} -result {1 2 3 4} -setup $setup



