#ifndef KSUID_TCL_HEX_H
#define KSUID_TCL_HEX_H

#include <tcl.h>
#include <vector>
#include <string>

int hex_encode(const unsigned char *str, int length, std::string& output);
int hex_decode(const std::string& str, int length, unsigned char *output, int output_length);

#endif //KSUID_TCL_HEX_H
