#include <sstream>
#include <iomanip>
#include "hex.h"


void hex_encode(unsigned char *str, int length, std::string& output) {
    std::stringstream ss;
    for (int i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)str[i];
    }
    output = ss.str();
}

void hex_decode(const std::string& str, int length, unsigned char *output) {
    for (int i = 0; i < length; i++) {
        std::stringstream ss;
        ss << std::hex << str.substr(i * 2, 2);
        int n;
        ss >> n;
        output[i] = n;
    }
}
