#include "custom_uint128.h"

// A function that returns the sum with carry of x, y and carry
// The carry input must be 0 or 1; otherwise the behavior is undefined
// The carryOut output is guaranteed to be 0 or 1
void add64(uint64_t x, uint64_t y, uint64_t carry, uint64_t& sum, uint64_t& carryOut) {
    // Add x and y with the carry
    sum = x + y + carry;
    // Check if there is an overflow
    if (sum < x || sum < y) {
        // Set the carryOut to 1
        carryOut = 1;
    } else {
        // Set the carryOut to 0
        carryOut = 0;
    }
}

// A function that returns the difference with borrow of x, y and borrow
// The borrow input must be 0 or 1; otherwise the behavior is undefined
// The borrowOut output is guaranteed to be 0 or 1
void sub64(uint64_t x, uint64_t y, uint64_t borrow, uint64_t& diff, uint64_t& borrowOut) {
    // Subtract y and borrow from x
    diff = x - y - borrow;
    // Check if there is an underflow
    if (x < y || x < diff) {
        // Set the borrowOut to 1
        borrowOut = 1;
    } else {
        // Set the borrowOut to 0
        borrowOut = 0;
    }
}

custom_uint128_t add128(custom_uint128_t& x, custom_uint128_t& y) {
    custom_uint128_t result;
    uint64_t sum;
    uint64_t carry;

    // Add the low 64 bits of x and y
    add64(x.lo, y.lo, 0, sum, carry);
    result.lo = sum;

    // Add the high 64 bits of x and y with the carry
    add64(x.hi, y.hi, carry, sum, carry);
    result.hi = sum;

    return result;
}

custom_uint128_t sub128(custom_uint128_t& x, custom_uint128_t& y) {
    custom_uint128_t result;
    uint64_t diff;
    uint64_t borrow;

    // Subtract the low 64 bits of y from x
    sub64(x.lo, y.lo, 0, diff, borrow);
    result.lo = diff;

    // Subtract the high 64 bits of y from x with the borrow
    sub64(x.hi, y.hi, borrow, diff, borrow);
    result.hi = diff;

    return result;
}

custom_uint128_t incr128(custom_uint128_t& x) {
    // use "add64" to add 1 to the low 64 bits
    uint64_t sum;
    uint64_t carry;

    add64(x.lo, 1, 0, sum, carry);
    x.lo = sum;
    x.hi += carry;

    return x;
}

custom_uint128_t decr128(custom_uint128_t& x) {
    // use "sub64" to subtract 1 from the low 64 bits
    uint64_t diff;
    uint64_t borrow;

    sub64(x.lo, 1, 0, diff, borrow);
    x.lo = diff;
    x.hi -= borrow;

    return x;
}

int cmp128(custom_uint128_t& x, custom_uint128_t& y) {
    if (x.hi < y.hi) {
        return -1;
    } else if (x.hi > y.hi) {
        return 1;
    } else if (x.lo < y.lo) {
        return -1;
    } else if (x.lo > y.lo) {
        return 1;
    } else {
        return 0;
    }
}

custom_uint128_t make_uint128(uint64_t lo, uint64_t hi) {
    custom_uint128_t result;
    result.lo = lo;
    result.hi = hi;
    return result;
}


custom_uint128_t make_uint128_from_bytes(const unsigned char* bytes) {
    custom_uint128_t result;
    result.lo = 0;
    result.hi = 0;
    // big endian
    for (int i = 0; i < 8; i++) {
        result.hi |= ((uint64_t) bytes[i]) << (i * 8);
    }
    for (int i = 0; i < 8; i++) {
        result.lo |= ((uint64_t) bytes[i + 8]) << (i * 8);
    }
    return result;
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

// The equivalent of uint128_to_bytes in go-lang is:
//func (v uint128) bytes() (out [16]byte) {
//    binary.BigEndian.PutUint64(out[:8], v[1]) // high
//    binary.BigEndian.PutUint64(out[8:], v[0]) // low
//    return
//}

void uint128_to_bytes(custom_uint128_t x, unsigned char* bytes) {
    for (int i = 0; i < 8; i++) {
#if IS_BIG_ENDIAN
        // when x.hi is in big endian format:
        bytes[i] = (unsigned char) (x.hi >> ((7 - i) * 8));
#else
        // when x.hi is in little endian format:
        bytes[i] = (unsigned char) (x.hi >> (i * 8));
#endif
    }
    for (int i = 0; i < 8; i++) {
#if IS_BIG_ENDIAN
        // when x.hi is in big endian format:
        bytes[i + 8] = (unsigned char) (x.lo >> ((7 - i) * 8));
#else
        // when x.hi is in little endian format:
        bytes[i + 8] = (unsigned char) (x.lo >> (i * 8));
#endif
    }
}
