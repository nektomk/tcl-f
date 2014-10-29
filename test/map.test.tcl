package require f
namespace import ::f::*
source testcase.tcl

testcase map-wo-args { map w/o args shoulds return error } {
	map
} -match glob -result * -code error

testcase map-single-arg { map with single arg shoulds return arg } {
	map hello
} {hello}

testcase map-list-of-digits {map list of digits} {
	 map x {expr $x+1} {1 2 3}
} {2 3 4}

testcase map-list-of-words {map list of any words} {
	 map string toupper {hello word}
} {HELLO WORD}

testcase map-list-of-list { map list of other list } {
	 map llength {{hello word} {1 2 3 4} "to be or not to beeee"}
} {2 4 6}

testcase map-list-of-tuples { map list of tuples } {
	 map concat "hello " [ list [ tuple anna bolein ] [ tuple peaples of word ] [ tuple 1 2 three {7 monkeys} ] ]
} {{hello anna bolein} {hello peaples of word} {hello 1 2 three {7 monkeys}}}

testcase map-tuple-of-list { map tuple of lists } {
	 map concat "hello " [ tuple [ list anna bolein ] [ list peaples of word ] [ list 1 2 three {7 monkeys} ] ]
} {{hello anna bolein} {hello peaples of word} {hello 1 2 three {7 monkeys}}}

testcase map-tuple-of-tuples { map tuple of tuples } {
	 map concat "hello " [ tuple [ tuple anna bolein ] [ tuple peaples of word ] [ tuple 1 2 three {7 monkeys} ] ]
} {{hello anna} {hello bolein} {hello peaples} {hello of} {hello word} {hello 1} {hello 2} {hello three} {hello 7 monkeys}}



