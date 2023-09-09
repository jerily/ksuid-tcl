#ifndef KSUID_TCL_CUSTOM_UINT128_H
#define KSUID_TCL_CUSTOM_UINT128_H

#include <cstdint>

typedef struct {
    uint64_t lo;
    uint64_t hi;
} custom_uint128_t;

custom_uint128_t make_uint128(uint64_t lo, uint64_t hi);
custom_uint128_t make_uint128_from_bytes(const unsigned char* bytes);
custom_uint128_t add128(custom_uint128_t& x, custom_uint128_t& y);
custom_uint128_t sub128(custom_uint128_t& x, custom_uint128_t& y);
custom_uint128_t incr128(custom_uint128_t& x);
custom_uint128_t decr128(custom_uint128_t& x);
int cmp128(custom_uint128_t& x, custom_uint128_t& y);
void uint128_to_bytes(custom_uint128_t x, unsigned char* bytes);

#endif //KSUID_TCL_CUSTOM_UINT128_H
