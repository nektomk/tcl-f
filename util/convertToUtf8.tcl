#!/bin/tclsh
if {$argv=={}} {
	puts "convert source files to utf-8 encoding with \\n line delimiters only"
	puts "Usage: $argv0 file1 file2 ..."
	exit 1
}
foreach name $argv {
    puts -nonewline "convert $name to tmp ... "
    flush stdout
    if { [ catch {
    	set in [ open $name "r" ]
	set out [ file tempfile tmpName ]
    	fconfigure $in -translation auto
    	fconfigure $out -encoding utf-8 -translation lf
	while { ! [ eof $in ] } {
        	set s [ gets $in ]
        	set s [ string trimright $s ]
        	puts $out $s
    	}
	close $in
    	close $out

	} errText ]  } {
	puts "Error in conversion: $errText"
	continue
    }
    puts -nonewline "ok; backup & rewrite ... "
    if { [ catch {
	    file copy -force $name ${name}.backup
	    file copy -force $tmpName ${name}
	    file delete -force $tmpName
    	} errText ] } {
	puts "Error in backup/rewrite: $errtext"
    } else {
        puts "ok"
    }
}


