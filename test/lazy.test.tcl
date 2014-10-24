source testcase.tcl

package require tcltest
package require f

namespace import f::*

testcase lazy-without-args "lazy command w/o args" -body {
   set x [lazy]
} -returnCodes error -match glob -result *

testcase lazy-with-args "lazy command with args" -body {
   lazy expr 1 + 2
} -result 3 -returnCodes ok

testcase lazy-var "lazy to variable" -body {
   set x [ lazy expr 1 + 2 ]
   list
} -returnCodes ok -result {}

testcase lazy-subst "lazy substitution" -body {
   set x [ lazy {puts "hello"
	list 1} ]
   set y "x$x"
} -output {hello}  -constraints {winCrash}
   
testcase lazy-once "lazy only once" -body {
   set x [ lazy {
      puts -nonewline "hello"
      list 1 2
   } ]
   concat $x $x
} -output {hello} -result {1 2 1 2}

testcase lazy-copy "one lazy in mult. vars" -body {
   set x [ lazy {
      puts -nonewline "hello"
      list 1 2
   } ]
   set y $x
   set z $y
   concat $x $y $z
} -result {1 2 1 2 1 2} -output {hello} -constraints winCrash

testcase lazy-lazy "multiple nested lazy`es as expr" -body {
   set x [ lazy lazy {
      list 1 2 3 4
   } ]
} -result {1 2 3 4}

testcase lazy-lazy2 "multiple nested lazy`es as list" -body {
   set x [ lazy { lazy {
      list 1 2 3 4
   } } ]
} -result {1 2 3 4}




