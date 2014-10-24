#!/usr/bin/tclsh
package require tcltest

# для более аккуратного (и удобного) формирования тестов
# синтаксис такой-же как у tcltest::test
proc testcase { name descr args } {
    set dict [ dict create ]
    set first [ lindex $args 0 ]
    if { $first!={} && [ string index $first 0 ] != "-" } {
        dict set dict -body $first
        set args [ lrange $args 1 end ]
        set first [ lindex $args 0 ]
        if { $first=={-} || ( $first!={} && [ string index $first 0 ] != "-")  } {
            dict set dict -result $first
            set args [ lrange $args 1 end ]
        }
    }
    foreach {key value} $args {
        set errList {};
        if { [ string index $key 0 ] != "-" } {
            lappend errList "option w/o leading \"-\""
            continue
        }
        switch -exact $key {
            -constraints {}
            -setup {}
            -body {}
            -cleanup {}
            -result {}
            -output {}
            -errorOutput {}
            -returnCodes {}
            -match {}
            -code {
                set key -returnCodes
            }
            default {
                lappend errList "unknown option $key"
            }
        }
        if {$errList=={}} {
            dict set dict $key $value
        } else {
            puts stderr "Errors in definition test-case $name:"
            puts stderr \t[ join $errList "\n\t" ]
        }
    }
    tailcall tcltest::test $name $descr {*}[dict get $dict]  
}

