#include "base62.h"

static char BASE_62_CHARACTERS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int base62_encode(unsigned char timestamp_and_payload_bytes[], int input_length,
                  unsigned char output[], int output_length) {

    auto offset = output_length;
    auto input = timestamp_and_payload_bytes;
    while (input_length > 0) {
        if (offset == 0) {
            return TCL_ERROR;
        }
        std::vector<unsigned char> quotients;
        unsigned long remainder = 0;
        for (int i = 0; i < input_length; i++) {
            auto b = input[i];
            auto accumulator = (remainder << 8) + b;
            auto quotient = accumulator / 62;
            remainder = accumulator % 62;
            if (!quotients.empty() || quotient != 0) {
                quotients.push_back(quotient);
            }
        }
        output[--offset] = BASE_62_CHARACTERS[remainder];
        std::copy(quotients.begin(), quotients.end(), input);
        input_length = quotients.size();
    }

    while (offset > 0) {
        output[--offset] = BASE_62_CHARACTERS[0];
    }

    return TCL_OK;
}

static unsigned char base62_value(unsigned char digit) {
    static unsigned char offsetUppercase = 10;
    static unsigned char offsetLowercase = 36;

	if (digit >= '0' && digit <= '9') {
        return digit - '0';
    } else if (digit >= 'A' && digit <= 'Z') {
        return offsetUppercase + (digit - 'A');
    } else {
		return offsetLowercase + (digit - 'a');
	}
}

// In order to support a couple of optimizations the function assumes that src
// is 27 bytes long and dst is 20 bytes long.
int base62_decode(const unsigned char src[], unsigned char dst[]) {

    // Create a vector of size 27 to store the ksuid bytes
    unsigned char input[27];
    input[0] = base62_value(src[0]);
    input[1] = base62_value(src[1]);
    input[2] = base62_value(src[2]);
    input[3] = base62_value(src[3]);
    input[4] = base62_value(src[4]);
    input[5] = base62_value(src[5]);
    input[6] = base62_value(src[6]);
    input[7] = base62_value(src[7]);
    input[8] = base62_value(src[8]);
    input[9] = base62_value(src[9]);
    input[10] = base62_value(src[10]);
    input[11] = base62_value(src[11]);
    input[12] = base62_value(src[12]);
    input[13] = base62_value(src[13]);
    input[14] = base62_value(src[14]);
    input[15] = base62_value(src[15]);
    input[16] = base62_value(src[16]);
    input[17] = base62_value(src[17]);
    input[18] = base62_value(src[18]);
    input[19] = base62_value(src[19]);
    input[20] = base62_value(src[20]);
    input[21] = base62_value(src[21]);
    input[22] = base62_value(src[22]);
    input[23] = base62_value(src[23]);
    input[24] = base62_value(src[24]);
    input[25] = base62_value(src[25]);
    input[26] = base62_value(src[26]);

    auto input_length = 27;
    auto offset = 20;
    while (input_length > 0) {
        if (offset == 0) {
            return TCL_ERROR;
        }
        std::vector<unsigned char> quotients;
        unsigned long remainder = 0;
        for (int i = 0; i < input_length; i++) {
            auto b = input[i];
            auto accumulator = (remainder * 62) + b;
            auto quotient = accumulator / 256;
            remainder = accumulator % 256;
            if (!quotients.empty() || quotient != 0) {
                quotients.push_back(quotient);
            }
        }
        dst[--offset] = remainder;
        std::copy(quotients.begin(), quotients.end(), input);
        input_length = quotients.size();
    }

    while (offset > 0) {
        dst[--offset] = 0;
    }

    return TCL_OK;
}
