#ifndef KSUID_TCL_BASE62_H
#define KSUID_TCL_BASE62_H

#include <tcl.h>
#include <vector>

int base62_encode(unsigned char input[], int input_length, unsigned char output[], int output_length);
int base62_decode(const unsigned char src[], unsigned char dst[]);

#endif //KSUID_TCL_BASE62_H
