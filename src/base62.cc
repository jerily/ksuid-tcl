#include "base62.h"

static char BASE_62_CHARACTERS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void base62_encode(unsigned char timestamp_and_payload_bytes[], int input_length,
                  unsigned char output[], int output_length) {

    auto offset = output_length;
    auto input = timestamp_and_payload_bytes;
    while (input_length > 0) {
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

}

void base62_decode(unsigned char base62_encoded_bytes[], int input_length, unsigned char output[], int output_length) {
    auto offset = output_length;
    auto input = base62_encoded_bytes;
    while (input_length > 0) {
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
        output[--offset] = remainder;
        std::copy(quotients.begin(), quotients.end(), input);
        input_length = quotients.size();
    }

    while (offset > 0) {
        output[--offset] = 0;
    }

}
