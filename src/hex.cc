#include <sstream>
#include <iomanip>
#include "hex.h"


int hex_encode(Tcl_Interp *interp, unsigned char *str, int length, std::string& output) {
    std::stringstream ss;
    for (int i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)str[i];
    }
    output = ss.str();
    return TCL_OK;
}

int hex_decode(Tcl_Interp *interp, const std::string& str, std::vector<unsigned char>& output) {
    for (int i = 0; i < output.size(); i++) {
        std::stringstream ss;
        ss << std::hex << str.substr(i * 2, 2);
        int n;
        ss >> n;
        output[i] = n;
    }
    return TCL_OK;
}
