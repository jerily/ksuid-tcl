#ifndef KSUID_TCL_BASE62_H
#define KSUID_TCL_BASE62_H

#include <tcl.h>
#include <vector>

void base62_encode(unsigned char input[], int input_length, unsigned char output[], int output_length);
void base62_decode(unsigned char base62_encoded_bytes[], int input_length, unsigned char output[], int output_length);

#endif //KSUID_TCL_BASE62_H
