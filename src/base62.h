#ifndef KSUID_TCL_BASE62_H
#define KSUID_TCL_BASE62_H

#include <tcl.h>
#include <vector>

int base62_encode(Tcl_Interp *interp, const std::vector<unsigned char>& timestamp_and_payload_bytes, std::vector<unsigned char>& result);
int base62_decode(Tcl_Interp *interp, const std::vector<unsigned char>& base62_encoded_bytes, std::vector<unsigned char>& result);

#endif //KSUID_TCL_BASE62_H
