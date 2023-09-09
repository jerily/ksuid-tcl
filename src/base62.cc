#include "base62.h"

static char BASE_62_CHARACTERS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int base62_encode(Tcl_Interp *interp, const std::vector<unsigned char> &timestamp_and_payload_bytes,
                  std::vector<unsigned char> &result) {
    if (result.size() < timestamp_and_payload_bytes.size()) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("bytes exceeds expected length", -1));
        return TCL_ERROR;
    }

    auto offset = result.size();
    auto input = timestamp_and_payload_bytes;
    while (!input.empty()) {
        std::vector<unsigned char> quotients;
        unsigned long remainder = 0;
        for (auto b: input) {
            auto accumulator = (remainder << 8) + b;
            auto quotient = accumulator / 62;
            remainder = accumulator % 62;
            if (!quotients.empty() || quotient != 0) {
                quotients.push_back(quotient);
            }
        }
        result[--offset] = BASE_62_CHARACTERS[remainder];
        input = quotients;
    }

    while (offset > 0) {
        result[--offset] = BASE_62_CHARACTERS[0];
    }

// copy the result to the output
    std::copy(result.begin(), result.end(), result.begin());

    return TCL_OK;
}

int base62_decode(Tcl_Interp *interp, const std::vector<unsigned char> &base62_encoded_bytes,
                  std::vector<unsigned char> &result) {
    auto offset = result.size();
    auto input = base62_encoded_bytes;
    while (!input.empty()) {
        std::vector<unsigned char> quotients;
        unsigned long remainder = 0;
        for (auto b: input) {
            auto accumulator = (remainder * 62) + b;
            auto quotient = accumulator / 256;
            remainder = accumulator % 256;
            if (!quotients.empty() || quotient != 0) {
                quotients.push_back(quotient);
            }
        }
        result[--offset] = remainder;
        input = quotients;
    }

    while (offset > 0) {
        result[--offset] = 0;
    }

// copy the result to the output
    std::copy(result.begin(), result.end(), result.begin());

    return TCL_OK;
}
