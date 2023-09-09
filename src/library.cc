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
#include "custom_uint128.h"

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

// KSUIDs are 20 bytes:
//  00-03 byte: uint32 BE UTC timestamp with custom epoch
//  04-19 byte: random "payload"

static int EPOCH = 1400000000;
static int PAYLOAD_BYTES = 16;
static int TIMESTAMP_BYTES = 4;
static int TOTAL_BYTES = TIMESTAMP_BYTES + PAYLOAD_BYTES;
static int PAD_TO_LENGTH = 27;

// A string-encoded minimum value for a KSUID
static char MIN_STRING_ENCODED[] = "000000000000000000000000000";

// A string-encoded maximum value for a KSUID
static char MAX_STRING_ENCODED[] = "aWgEPTl1tmebfsQzFP4bxwgy80V";

static int ksuid_ConcatTimestampAndPayload(Tcl_Interp *interp, const unsigned char timestamp_bytes[],
                                           const unsigned char payload_bytes[]) {

    // ---- Concatenate the timestamp and payload bytes ----
    // Create a vector of size 20 to store the timestamp and payload bytes
    unsigned char timestamp_and_payload_bytes[TOTAL_BYTES];
    // Copy the timestamp bytes to the vector
    std::copy(timestamp_bytes, timestamp_bytes + TIMESTAMP_BYTES, timestamp_and_payload_bytes);
    // Copy the payload bytes to the vector
    std::copy(payload_bytes, payload_bytes + PAYLOAD_BYTES, timestamp_and_payload_bytes + TIMESTAMP_BYTES);

    // ---- Base62 encode with fixed length ----
    unsigned char base62[PAD_TO_LENGTH];
    base62_encode(timestamp_and_payload_bytes, TOTAL_BYTES, base62, PAD_TO_LENGTH);
    std::string base62_str(base62, base62 + PAD_TO_LENGTH);

    Tcl_SetObjResult(interp, Tcl_NewStringObj(base62_str.c_str(), base62_str.length()));
    return TCL_OK;
}

static void ksuid_TimestampToBytes(long timestamp, unsigned char timestamp_bytes[]) {
    // Convert the timestamp to bytes and store them in the vector
    for (int i = 0; i < TIMESTAMP_BYTES; i++) {
        timestamp_bytes[TIMESTAMP_BYTES - i - 1] = (timestamp >> (i * 8)) & 0xFF;
    }
}

static int ksuid_GenerateKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "GenerateCmd\n"));
    CheckArgs(1, 1, 1, "");

    // ---- Generate the payload ----
    // Create a random device and a mt19937 engine
    std::random_device rd;
    std::mt19937 mt(rd());
    // Create a uniform_int_distribution object that generates unsigned char values between 0 and 255
    std::uniform_int_distribution<uint64_t> dist(0, 18446744073709551615ULL);
    custom_uint128_t u = make_uint128(dist(mt), dist(mt));
    unsigned char payload_bytes[PAYLOAD_BYTES];
    uint128_to_bytes(u, payload_bytes);

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
    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}

static int ksuid_KsuidToParts(Tcl_Interp *interp, const char * ksuid, Tcl_Obj **resultPtr) {
    // Create a vector of size 27 to store the ksuid bytes
    unsigned char ksuid_bytes[PAD_TO_LENGTH];

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
    unsigned char timestamp_and_payload_bytes[TOTAL_BYTES];
    base62_decode(ksuid_bytes, PAD_TO_LENGTH, timestamp_and_payload_bytes, TOTAL_BYTES);

    // ---- Convert the timestamp bytes to a long ----
    long timestamp = 0;
    for (int i = 0; i < TIMESTAMP_BYTES; i++) {
        timestamp |= timestamp_and_payload_bytes[TIMESTAMP_BYTES - i - 1] << (i * 8);
    }
    timestamp += EPOCH;
    timestamp *= 1000;

    // ---- Convert the payload bytes to a hex string ----
    std::string hex;
    hex_encode(timestamp_and_payload_bytes + TIMESTAMP_BYTES, PAYLOAD_BYTES, hex);

    // ---- Return the timestamp and payload ----
    Tcl_Obj *dictPtr = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("timestamp", -1), Tcl_NewLongObj(timestamp));
    Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("payload", -1), Tcl_NewStringObj(hex.c_str(), hex.length()));
    *resultPtr = dictPtr;
    return TCL_OK;
}

static int ksuid_KsuidToPartsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "KsuidToPartsCmd\n"));
    CheckArgs(2, 2, 1, "ksuid");

    // ---- Convert the ksuid to bytes ----

    // Get the ksuid from the arguments
    auto ksuid = Tcl_GetString(objv[1]);

    Tcl_Obj *dictPtr;
    if (TCL_OK != ksuid_KsuidToParts(interp, ksuid, &dictPtr)) {
        return TCL_ERROR;
    }
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
    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    // Convert the timestamp to bytes and store them in the vector
    for (int i = 0; i < TIMESTAMP_BYTES; i++) {
        timestamp_bytes[TIMESTAMP_BYTES - i - 1] = (timestamp >> (i * 8)) & 0xFF;
    }

    // ---- Convert the payload to bytes ----
    auto payload = Tcl_GetString(payloadPtr);
    unsigned char payload_bytes[PAYLOAD_BYTES];
    hex_decode(payload, PAYLOAD_BYTES, payload_bytes);

    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}


static int ksuid_KsuidFromTimestampAndPayload(Tcl_Interp *pInterp, long t, custom_uint128_t v) {

    // ---- Convert the timestamp to bytes ----
    // Create a vector of size 4 to store the timestamp bytes
    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(t, timestamp_bytes);

    // ---- Convert the payload to bytes ----
    unsigned char payload_bytes[PAYLOAD_BYTES];
    uint128_to_bytes(v, payload_bytes);

    return ksuid_ConcatTimestampAndPayload(pInterp, timestamp_bytes, payload_bytes);
}

static int ksuid_NextKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "NextKsuidCmd\n"));
    CheckArgs(2, 2, 1, "ksuid");

    auto ksuid = Tcl_GetString(objv[1]);

    Tcl_Obj *dictPtr;
    if (TCL_OK != ksuid_KsuidToParts(interp, ksuid, &dictPtr)) {
        return TCL_ERROR;
    }

    Tcl_Obj *timestampPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, dictPtr, Tcl_NewStringObj("timestamp", -1), &timestampPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing timestamp", -1));
        return TCL_ERROR;
    }
    long timestamp;
    if (TCL_OK != Tcl_GetLongFromObj(interp, timestampPtr, &timestamp)) {
        return TCL_ERROR;
    }
    timestamp /= 1000;
    timestamp -= EPOCH;

    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    Tcl_Obj *payloadPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, dictPtr, Tcl_NewStringObj("payload", -1), &payloadPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing payload", -1));
        return TCL_ERROR;
    }
    auto payload = Tcl_GetString(payloadPtr);
    unsigned char payload_bytes[PAYLOAD_BYTES];
    hex_decode(payload, PAYLOAD_BYTES, payload_bytes);

    auto zero = make_uint128(0, 0);

	auto t = timestamp;
	auto u = make_uint128_from_bytes(payload_bytes);
	auto v = incr128(u);

	if (0 == cmp128(v, zero)) { // overflow
		t++;
	}

    return ksuid_KsuidFromTimestampAndPayload(interp, t, v);
//    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}

static int ksuid_PrevKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "PrevKsuidCmd\n"));
    CheckArgs(2, 2, 1, "ksuid");

    auto ksuid = Tcl_GetString(objv[1]);

    Tcl_Obj *dictPtr;
    if (TCL_OK != ksuid_KsuidToParts(interp, ksuid, &dictPtr)) {
        return TCL_ERROR;
    }

    Tcl_Obj *timestampPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, dictPtr, Tcl_NewStringObj("timestamp", -1), &timestampPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing timestamp", -1));
        return TCL_ERROR;
    }
    long timestamp;
    if (TCL_OK != Tcl_GetLongFromObj(interp, timestampPtr, &timestamp)) {
        return TCL_ERROR;
    }
    timestamp /= 1000;
    timestamp -= EPOCH;

    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    Tcl_Obj *payloadPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, dictPtr, Tcl_NewStringObj("payload", -1), &payloadPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing payload", -1));
        return TCL_ERROR;
    }
    auto payload = Tcl_GetString(payloadPtr);
    unsigned char payload_bytes[PAYLOAD_BYTES];
    hex_decode(payload, PAYLOAD_BYTES, payload_bytes);

    auto zero = make_uint128(0, 0);

    auto t = timestamp;
    auto u = make_uint128_from_bytes(payload_bytes);
    auto v = decr128(u);

    if (0 == cmp128(v, zero)) { // overflow
        t++;
    }

    return ksuid_KsuidFromTimestampAndPayload(interp, t, v);
//    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}

static int ksuid_HexEncodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "HexEncodeCmd\n"));
    CheckArgs(2, 2, 1, "bytes");

    int length;
    auto bytes = Tcl_GetByteArrayFromObj(objv[1], &length);
    std::string hex;
    hex_encode(bytes, length, hex);

    Tcl_SetObjResult(interp, Tcl_NewStringObj(hex.c_str(), hex.length()));
    return TCL_OK;
}

static int ksuid_HexDecodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "HexDecodeCmd\n"));
    CheckArgs(2, 2, 1, "hex_string");

    int length;
    auto hex = Tcl_GetStringFromObj(objv[1], &length);
    unsigned char bytes[length / 2];
    hex_decode(hex, length / 2, bytes);
    Tcl_SetObjResult(interp, Tcl_NewByteArrayObj(bytes, length/2));
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
    Tcl_CreateObjCommand(interp, "::ksuid::next_ksuid", ksuid_NextKsuidCmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "::ksuid::prev_ksuid", ksuid_PrevKsuidCmd, nullptr, nullptr);
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
