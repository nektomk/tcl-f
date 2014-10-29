lappend auto_path [pwd]
package require f
namespace import ::f::*

set mid [ func {x y} {expr ($x + $y)/2.0} ]
set add2 [ func x { expr $x+1 } ]
set ret [ chain [ list $mid $add2 ] 23 17 ]
puts $ret
