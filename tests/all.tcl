package require tcltest
namespace import -force ::tcltest::test

if { [llength $argv] == 0 } {
    puts stderr "Usage: $argv0 libdir"
    exit 1
}

lappend auto_path [lindex $argv 0]
set argv [lrange $argv 1 end]

::tcltest::configure -singleproc true -testdir [file dirname [info script]]

exit [::tcltest::runAllTests]