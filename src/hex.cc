#include <sstream>
#include <iomanip>
#include "hex.h"

static const char* hex_digits = "0123456789abcdef";

// This lookup table allows fast conversion between ASCII hex letters and their
// corresponding numerical value. The 8-bit range is divided up into 8
// regions of 0x20 characters each. Each of the three character types (numbers,
// uppercase, lowercase) falls into different regions of this range. The table
// contains the amount to subtract from characters in that range to get at
// the corresponding numerical value.
//
// https://chromium.googlesource.com/chromium/src/+/refs/heads/main/url/url_canon_internal.cc#289
//
static const char char_to_hex_lookup[8] = {
        0,         // 0x00 - 0x1f
        '0',       // 0x20 - 0x3f: digits 0 - 9 are 0x30 - 0x39
        'A' - 10,  // 0x40 - 0x5f: letters A - F are 0x41 - 0x46
        'a' - 10,  // 0x60 - 0x7f: letters a - f are 0x61 - 0x66
        0,         // 0x80 - 0x9F
        0,         // 0xA0 - 0xBF
        0,         // 0xC0 - 0xDF
        0,         // 0xE0 - 0xFF
};

static inline int HexCharToValue(unsigned char c) {
    return c - char_to_hex_lookup[c / 0x20];
}

int hex_encode(const unsigned char *str, int length, std::string& output) {
    for (int i = 0; i < length; i++) {
        unsigned char c = str[i];
        output += hex_digits[(c >> 4) & 0xF]; // Extract the high nibble of c and use it as an index in the lookup table
        output += hex_digits[c & 0xF]; // Extract the low nibble of c and use it as an index in the lookup table
    }

    return TCL_OK;
}

int hex_decode(const std::string& str, int length, unsigned char *output, int output_length) {
    // check if length is even
    if (length % 2 != 0) {
        return TCL_ERROR;
    }
    if (length / 2 > output_length) {
        return TCL_ERROR;
    }

    // decode hex encoded "str" into "output", check if it is a valid hex character
    for (int i = 0; i < output_length; i++) {
        auto c0 = str[i * 2];
        auto c1 = str[i * 2 + 1];

        // check if c0 and c1 are valid hex characters
        if (c0 < 0x20 || c0 > 0x7f || c1 < 0x20 || c1 > 0x7f) {
            return TCL_ERROR;
        }

        output[i] = (HexCharToValue(c0) << 4) + HexCharToValue(c1);
    }
    return TCL_OK;
}
