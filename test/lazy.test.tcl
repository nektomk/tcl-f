package require tcltest
package require f
namespace import f::*

tcltest::test lazy-withot-args "lazy command w/o args" -body {
   set x [lazy]
} -returnCodes error -match glob -result *

tcltest::test lazy-with-args "lazy command with args" -body {
   lazy expr 1 + 2
} -result 3 -returnCodes ok

tcltest::test lazy-var "lazy to variable" -body {
   set x [ lazy expr 1 + 2 ]
   list
} -returnCodes ok -result {}

tcltest::test lazy-subst "lazy substitution" -body {
   set x [ lazy {puts "hello"
	list 1} ]
   set y "x$x"
} -output {hello}  -constraints {winCrash}
   
tcltest::test lazy-once "lazy only once" -body {
   set x [ lazy {
      puts -nonewline "hello"
      list 1 2
   } ]
   puts [ concat $x $x ]
} -output {hello 1 2 1 2}

tcltest::test lazy-copy "one lazy in mult. vars" -body {
   set x [ lazy {
      puts -nonewline "hello"
      list 1 2
   } ]
   set y $x
   set z $y
   puts [ concat $x $y $z ]
   
} -result {1 2 1 2 1 2} -output {hello}

tcltest::test lazy-lazy "multiple nested lazy`es" -body {
   set x [ lazy lazy lazy {
      list 1 2 3 4
   } ]
} -result {1 2 3 4}



