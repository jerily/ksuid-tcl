package require ksuid

set ksuid [::ksuid::generate_ksuid]
puts ksuid=$ksuid
puts prev_of_next_ksuid=[::ksuid::prev_ksuid [::ksuid::next_ksuid $ksuid]]
puts next_of_prev_ksuid=[::ksuid::next_ksuid [::ksuid::prev_ksuid $ksuid]]
puts ksuid_length=[string length $ksuid]

set parts [::ksuid::ksuid_to_parts $ksuid]
puts parts=$parts
puts ksuid_from_parts=[::ksuid::parts_to_ksuid $parts]

set hex_encoded_payload [dict get $parts payload]
set bytes [::ksuid::hex_decode $hex_encoded_payload]
set hex_encoded_bytes [::ksuid::hex_encode $bytes]
puts hex_encoded_bytes=$hex_encoded_bytes

set hex_of_hello_world [::ksuid::hex_encode "hello world"]
if { $hex_of_hello_world eq "68656c6c6f20776f726c64" } {
    puts "hex_encode works"
} else {
    puts "hex_encode failed"
}

if { [::ksuid::hex_decode $hex_of_hello_world] eq "hello world" } {
    puts "hex_decode works"
} else {
    puts "hex_decode failed"
}
