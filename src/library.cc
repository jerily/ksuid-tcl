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
#include <limits>
#include "library.h"
#include "base62.h"
#include "hex.h"
#include "custom_uint128.h"

#ifndef TCL_SIZE_MAX
typedef int Tcl_Size;
# define Tcl_GetSizeIntFromObj Tcl_GetIntFromObj
# define Tcl_NewSizeIntObj Tcl_NewIntObj
# define TCL_SIZE_MAX      INT_MAX
# define TCL_SIZE_MODIFIER ""
#endif

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

#if defined(_WIN32) // Windows is always little endian
#define IS_BIG_ENDIAN 0
#elif defined(__linux__) // Linux has a header with endianness macros
#include <endian.h>
#define IS_BIG_ENDIAN (__BYTE_ORDER == __BIG_ENDIAN)
#elif defined(__APPLE__) // Mac OS X has a header with endianness macros
#include <machine/endian.h>
#define IS_BIG_ENDIAN (__DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN)
#else
// Unknown system, use a generic method
//#define IS_BIG_ENDIAN (!(*(unsigned char *)&(uint16_t){1}))
#define IS_BIG_ENDIAN 0
#endif

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

static void ksuid_TimestampToBytes(unsigned int x, unsigned char bytes[]) {
#if IS_BIG_ENDIAN
    // just use memcpy
    memcpy(bytes, &x, 4);
#else
    // Extract each byte of x and store it in the array in reverse order
    bytes[0] = (x >> 24) & 0xFF; // The most significant byte
    bytes[1] = (x >> 16) & 0xFF; // The second most significant byte
    bytes[2] = (x >> 8) & 0xFF; // The third most significant byte
    bytes[3] = x & 0xFF; // The least significant byte
#endif
}

unsigned int ksuid_BytesToTimestamp(const unsigned char bytes[]) {
    unsigned int result = 0;
#if IS_BIG_ENDIAN
    // just use memcpy
    memcpy(&result, bytes, 4);
#else
    result |= (bytes[0] << 24); // Shift the most significant byte left by 24 bits and add it to the result
    result |= (bytes[1] << 16); // Shift the second most significant byte left by 16 bits and add it to the result
    result |= (bytes[2] << 8); // Shift the third most significant byte left by 8 bits and add it to the result
    result |= bytes[3]; // Add the least significant byte to the result
#endif
    return result;
}



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
    if (TCL_OK != base62_encode(timestamp_and_payload_bytes, TOTAL_BYTES, base62, PAD_TO_LENGTH)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid base62", -1));
        return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj((const char *) base62, PAD_TO_LENGTH));
    return TCL_OK;
}

static int ksuid_GenerateKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "GenerateCmd\n"));
    CheckArgs(1, 1, 1, "");

    // ---- Generate the payload ----
    // Create a random device and a mt19937 engine
    std::random_device rd;
    std::mt19937 mt(rd());
    // Create a uniform_int_distribution object that generates unsigned char values between 0 and uint64_t max.
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
    custom_uint128_t v = make_uint128(dist(mt), dist(mt));
    unsigned char payload_bytes[PAYLOAD_BYTES];
    uint128_to_bytes(v, payload_bytes);

    // ---- Generate the timestamp ----
    // Get the current system time
    auto time = std::chrono::system_clock::now();
    // Get the duration since epoch
    auto since_epoch = time.time_since_epoch();
    // Convert the duration to milliseconds
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
    // Get the count of milliseconds as an integer
    unsigned int timestamp = millis.count() / 1000 - EPOCH;

    // ---- Convert the timestamp to bytes ----
    // Create a vector of size 4 to store the timestamp bytes
    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    return ksuid_ConcatTimestampAndPayload(interp, timestamp_bytes, payload_bytes);
}

static int ksuid_KsuidToParts(Tcl_Interp *interp, const unsigned char * ksuid, Tcl_Obj **resultPtr) {

    // ---- Base62 decode ----
    unsigned char timestamp_and_payload_bytes[TOTAL_BYTES];
    if (TCL_OK != base62_decode(ksuid, timestamp_and_payload_bytes)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid base62", -1));
        return TCL_ERROR;
    }

    // ---- Convert the timestamp bytes to a long ----
    unsigned int timestamp = ksuid_BytesToTimestamp(timestamp_and_payload_bytes);
//    timestamp += EPOCH;
//    timestamp *= 1000;

    // ---- Convert the payload bytes to a hex string ----
    std::string hex;
    hex_encode(timestamp_and_payload_bytes + TIMESTAMP_BYTES, PAYLOAD_BYTES, hex);

    // ---- Return the timestamp and payload ----
    Tcl_Obj *dictPtr = Tcl_NewDictObj();
//    Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("epoch", -1), Tcl_NewLongObj(EPOCH));
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
    Tcl_Size length;
    auto ksuid = (const unsigned char *) Tcl_GetStringFromObj(objv[1], &length);
    if (length != PAD_TO_LENGTH) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid ksuid", -1));
        return TCL_ERROR;
    }

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

    // Create a vector of size 4 to store the timestamp bytes
    // Convert the timestamp to bytes and store them in the vector
    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    // ---- Convert the payload to bytes ----
    Tcl_Size payload_length;
    auto payload = Tcl_GetStringFromObj(payloadPtr, &payload_length);
    unsigned char payload_bytes[PAYLOAD_BYTES];
    if (TCL_OK != hex_decode(payload, payload_length, payload_bytes, PAYLOAD_BYTES)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid hex", -1));
        return TCL_ERROR;
    }

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

    // Get the ksuid from the arguments
    Tcl_Size length;
    auto ksuid = (const unsigned char *) Tcl_GetStringFromObj(objv[1], &length);
    if (length != PAD_TO_LENGTH) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid ksuid", -1));
        return TCL_ERROR;
    }

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

    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    Tcl_Obj *payloadPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, dictPtr, Tcl_NewStringObj("payload", -1), &payloadPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing payload", -1));
        return TCL_ERROR;
    }
    Tcl_Size payload_length;
    auto payload = Tcl_GetStringFromObj(payloadPtr, &payload_length);
    unsigned char payload_bytes[PAYLOAD_BYTES];
    if (TCL_OK != hex_decode(payload, payload_length, payload_bytes, PAYLOAD_BYTES)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid hex", -1));
        return TCL_ERROR;
    }

    auto zero = make_uint128(0, 0);

	auto t = timestamp;
	auto u = make_uint128_from_bytes(payload_bytes);
	auto v = incr128(u);

	if (0 == cmp128(v, zero)) { // overflow
		t++;
	}

    return ksuid_KsuidFromTimestampAndPayload(interp, t, v);
}

static int ksuid_PrevKsuidCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "PrevKsuidCmd\n"));
    CheckArgs(2, 2, 1, "ksuid");

    // Get the ksuid from the arguments
    Tcl_Size length;
    auto ksuid = (const unsigned char *) Tcl_GetStringFromObj(objv[1], &length);
    if (length != PAD_TO_LENGTH) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid ksuid", -1));
        return TCL_ERROR;
    }

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

    unsigned char timestamp_bytes[TIMESTAMP_BYTES];
    ksuid_TimestampToBytes(timestamp, timestamp_bytes);

    Tcl_Obj *payloadPtr;
    if (TCL_OK != Tcl_DictObjGet(interp, dictPtr, Tcl_NewStringObj("payload", -1), &payloadPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("missing payload", -1));
        return TCL_ERROR;
    }
    Tcl_Size payload_length;
    auto payload = Tcl_GetStringFromObj(payloadPtr, &payload_length);
    unsigned char payload_bytes[PAYLOAD_BYTES];
    if (TCL_OK != hex_decode(payload, payload_length, payload_bytes, PAYLOAD_BYTES)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid hex", -1));
        return TCL_ERROR;
    }

    auto max = make_uint128(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());

    auto t = timestamp;
    auto u = make_uint128_from_bytes(payload_bytes);
    auto v = decr128(u);

    if (0 == cmp128(v, max)) { // overflow
        t--;
    }

    return ksuid_KsuidFromTimestampAndPayload(interp, t, v);
}

static int ksuid_HexEncodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "HexEncodeCmd\n"));
    CheckArgs(2, 2, 1, "bytes");

    Tcl_Size length;
    auto bytes = Tcl_GetByteArrayFromObj(objv[1], &length);
    std::string hex;
    if (TCL_OK != hex_encode(bytes, length, hex)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid hex", -1));
        return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(hex.c_str(), hex.length()));
    return TCL_OK;
}

static int ksuid_HexDecodeCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "HexDecodeCmd\n"));
    CheckArgs(2, 2, 1, "hex_string");

    Tcl_Size length;
    auto hex = Tcl_GetStringFromObj(objv[1], &length);
    unsigned char bytes[length / 2];
    if (TCL_OK != hex_decode(hex, length, bytes, length / 2)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid hex", -1));
        return TCL_ERROR;
    }
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
