#ifndef KSUID_TCL_HEX_H
#define KSUID_TCL_HEX_H

#include <tcl.h>
#include <vector>
#include <string>

void hex_encode(unsigned char *str, int length, std::string& output);
void hex_decode(const std::string& str, int length, unsigned char *output);

#endif //KSUID_TCL_HEX_H
