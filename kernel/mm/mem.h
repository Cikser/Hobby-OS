#ifndef RISC_V_MEM_H
#define RISC_V_MEM_H

#include "../types.h"

inline void* memset(void *dest, int val, size_t len) {
    uint8_t *ptr = (uint8_t*)dest;
    while (len-- > 0) {
        *ptr++ = (uint8_t)val;
    }
    return dest;
}

inline void* memcpy(void* dest, const void* source, size_t len) {
    const uint8_t* src = (const uint8_t*)source;
    for (size_t i = 0; i < len; ++i) {
        ((uint8_t*)dest)[i] = src[i];
    }
    return dest;
}

#endif