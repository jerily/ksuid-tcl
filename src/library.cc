/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2023 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#include <iostream>
#include <cstdio>
#include <random>
#include <vector>
#include <algorithm>
#include <chrono>
#include "library.h"
#include "base62.h"
#include "hex.h"

#define XSTR(s) STR(s)
#define STR(s) #s

#ifdef DEBUG
# define DBG(x) x
#else
# define DBG(x)
#endif

#define CheckArgs(min, max, n, msg) \
                 if ((objc < min) || (objc >max)) { \
                     Tcl_WrongNumArgs(interp, n, objv, msg); \
                     return TCL_ERROR; \
                 }

static int ksuid_ModuleInitialized;

static int EPOCH = 1400000000;
static int PAYLOAD_BYTES = 16;
static int TIMESTAMP_BYTES = 4;
static int TOTAL_BYTES = TIMESTAMP_BYTES + PAYLOAD_BYTES;
static int PAD_TO_LENGTH = 27;

static int ksuid_ConcatTimestampAndPayload(Tcl_Interp *interp, std::vector<unsigned char> timestamp_bytes,
                                           std::vector<unsigned char> payload_bytes) {

    // ---- Concatenate the timestamp and payload bytes ----
    // Create a vector of size 20 to store the timestamp and payload bytes
    std::vector<unsigned char> timestamp_and_payload_bytes(TOTAL_BYTES);
    // Copy the timestamp bytes to the vector
    std::copy(timestamp_bytes.begin(), timestamp_bytes.end(), timestamp_and_payload_bytes.begin());
    // Copy the payload bytes to the vector
    std::copy(payload_bytes.begin(), payload_bytes.end(), timestamp_and_payload_bytes.begin() + TIMESTAMP_BYTES);

    // ---- Base62 encode with fixed length ----
    std::vector<unsigned char> base62(PAD_TO_LENGTH);
    if (TCL_OK != base62_encode(interp, timestamp_and_payload_bytes, base62)) {
        return TCL_ERROR;
    }
    std::string base62_str(base62.begin(), base62.end());

    Tcl_SetObjResult(interp, Tcl_NewStringObj(base62_str.c_str(), base62_str.length()));
    return TCL_OK;
}

static int ksuid_GenerateKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "GenerateCmd\n"));
    CheckArgs(1, 1, 1, "");

    // ---- Generate the payload ----
    // Create a random device and a mt19937 engine
    std::random_device rd;
    std::mt19937 mt(rd());
    // Create a uniform_int_distribution object that generates unsigned char values between 0 and 255
    std::uniform_int_distribution<unsigned char> dist(0, 255);
    // Create a vector of size 16 to store the random bytes
    std::vector<unsigned char> payload_bytes(PAYLOAD_BYTES);
    // Fill the vector with random bytes using the generate_n algorithm
    std::generate_n(payload_bytes.begin(), payload_bytes.size(), [&](){ return dist(mt); });

    // ---- Generate the timestamp ----
    // Get the current system time
    auto time = std::chrono::system_clock::now();
    // Get the duration since epoch
    auto since_epoch = time.time_since_epoch();
    // Convert the duration to milliseconds
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
    // Get the count of milliseconds as an integer
    long timestamp = millis.count() / 1000 - EPOCH;

    // ---- Convert the timestamp to bytes ----
    // Create a vector of size 4 to store the timestamp bytes
    std::vector<unsigned char> timestamp_bytes(TIMESTAMP_BYTES);
    // Convert the timestamp to bytes and store them in the vector
    for (int i = 0; i < TIMESTAMP_BYTES; i++) {
        timestamp_bytes[TIMESTAMP_BYTES - i - 1] = (timestamp >> (i * 8)) & 0xFF;
    }

    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}

static int ksuid_KsuidToPartsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "KsuidToPartsCmd\n"));
    CheckArgs(2, 2, 1, "ksuid");

    // ---- Convert the ksuid to bytes ----

    // Get the ksuid from the arguments
    auto ksuid = Tcl_GetString(objv[1]);

    // Create a vector of size 27 to store the ksuid bytes
    std::vector<unsigned char> ksuid_bytes(PAD_TO_LENGTH);

    // Decode each character to a byte
    for (int i = 0; i < PAD_TO_LENGTH; i++) {
        // Get the character
        auto c = ksuid[i];

        // Convert the character to an index
        int index;
        if (c >= '0' && c <= '9') {
            index = c - 48;
        } else if (c >= 'A' && c <= 'Z') {
            index = c - 55;
        } else if (c >= 'a' && c <= 'z') {
            index = c - 61;
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid character", -1));
            return TCL_ERROR;
        }

        // Store the index as a byte
        ksuid_bytes[i] = index;
    }

    // ---- Base62 decode ----
    std::vector<unsigned char> timestamp_and_payload_bytes(TOTAL_BYTES);
    if (TCL_OK != base62_decode(interp, ksuid_bytes, timestamp_and_payload_bytes)) {
        return TCL_ERROR;
    }

    // ---- Convert the timestamp bytes to a long ----
    long timestamp = 0;
    for (int i = 0; i < TIMESTAMP_BYTES; i++) {
        timestamp |= timestamp_and_payload_bytes[TIMESTAMP_BYTES - i - 1] << (i * 8);
    }
    timestamp += EPOCH;
    timestamp *= 1000;

    // ---- Convert the payload bytes to a hex string ----
    std::string hex;
    if (TCL_OK != hex_encode(interp, timestamp_and_payload_bytes.data() + TIMESTAMP_BYTES, PAYLOAD_BYTES, hex)) {
        return TCL_ERROR;
    }

    // ---- Return the timestamp and payload ----
    Tcl_Obj *dictPtr = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("timestamp", -1), Tcl_NewLongObj(timestamp));
    Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("payload", -1), Tcl_NewStringObj(hex.c_str(), hex.length()));
    Tcl_SetObjResult(interp, dictPtr);
    return TCL_OK;
}

static int ksuid_PartsToKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "PartsToKsuidCmd\n"));
    CheckArgs(2, 2, 1, "parts_dict");

    // ---- Get the timestamp and payload from the dictionary ----
    Tcl_Obj *timestampPtr;
    Tcl_Obj *payloadPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, objv[1], Tcl_NewStringObj("timestamp", -1), &timestampPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing timestamp", -1));
        return TCL_ERROR;
    }
    if (TCL_OK != Tcl_DictObjGet(interp, objv[1], Tcl_NewStringObj("payload", -1), &payloadPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing payload", -1));
        return TCL_ERROR;
    }

    // ---- Convert the timestamp to bytes ----
    long timestamp;
    if (TCL_OK != Tcl_GetLongFromObj(interp, timestampPtr, &timestamp)) {
        return TCL_ERROR;
    }
    timestamp /= 1000;
    timestamp -= EPOCH;

    // Create a vector of size 4 to store the timestamp bytes
    std::vector<unsigned char> timestamp_bytes(TIMESTAMP_BYTES);
    // Convert the timestamp to bytes and store them in the vector
    for (int i = 0; i < TIMESTAMP_BYTES; i++) {
        timestamp_bytes[TIMESTAMP_BYTES - i - 1] = (timestamp >> (i * 8)) & 0xFF;
    }

    // ---- Convert the payload to bytes ----
    auto payload = Tcl_GetString(payloadPtr);
    std::vector<unsigned char> payload_bytes(PAYLOAD_BYTES);
    if (TCL_OK != hex_decode(interp, payload, payload_bytes)) {
        return TCL_ERROR;
    }

    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}

static int ksuid_HexEncodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "HexEncodeCmd\n"));
    CheckArgs(2, 2, 1, "bytes");

    int length;
    auto bytes = Tcl_GetByteArrayFromObj(objv[1], &length);
    std::string hex;
    if (TCL_OK != hex_encode(interp, bytes, length, hex)) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(hex.c_str(), hex.length()));
    return TCL_OK;
}

static int ksuid_HexDecodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "HexDecodeCmd\n"));
    CheckArgs(2, 2, 1, "hex_string");

    int length;
    auto hex = Tcl_GetStringFromObj(objv[1], &length);
    std::vector<unsigned char> bytes(length / 2);
    if (TCL_OK != hex_decode(interp, hex, bytes)) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewByteArrayObj(bytes.data(), bytes.size()));
    return TCL_OK;
}

static void ksuid_ExitHandler(ClientData unused) {
}


void ksuid_InitModule() {
    if (!ksuid_ModuleInitialized) {
        Tcl_CreateThreadExitHandler(ksuid_ExitHandler, nullptr);
        ksuid_ModuleInitialized = 1;
    }
}

int Ksuid_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.6", 0) == nullptr) {
        return TCL_ERROR;
    }

    ksuid_InitModule();

    Tcl_CreateNamespace(interp, "::ksuid", nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "::ksuid::generate_ksuid", ksuid_GenerateKsuidCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "::ksuid::ksuid_to_parts", ksuid_KsuidToPartsCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "::ksuid::parts_to_ksuid", ksuid_PartsToKsuidCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "::ksuid::hex_encode", ksuid_HexEncodeCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "::ksuid::hex_decode", ksuid_HexDecodeCmd, nullptr, nullptr);

    return Tcl_PkgProvide(interp, "ksuid", XSTR(PROJECT_VERSION));
}

#ifdef USE_NAVISERVER
int Ns_ModuleInit(const char *server, const char *module) {
    Ns_TclRegisterTrace(server, (Ns_TclTraceProc *) Ksuid_Init, server, NS_TCL_TRACE_CREATE);
    return NS_OK;
}
#endif
