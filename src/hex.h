#ifndef KSUID_TCL_HEX_H
#define KSUID_TCL_HEX_H

#include <tcl.h>
#include <vector>
#include <string>

int hex_encode(Tcl_Interp *interp, unsigned char *str, int length, std::string& output);
int hex_decode(Tcl_Interp *interp, const std::string& str, std::vector<unsigned char>& output);

#endif //KSUID_TCL_HEX_H
