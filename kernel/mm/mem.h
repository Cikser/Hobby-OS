#ifndef RISC_V_MEM_H
#define RISC_V_MEM_H

#include "../types.h"

extern "C" inline void* memset(void *dest, int val, size_t len) {
    uint8_t *ptr = (uint8_t*)dest;
    while (len-- > 0) {
        *ptr++ = (uint8_t)val;
    }
    return dest;
}

extern "C" inline void* memcpy(void* dest, const void* source, size_t len) {
    const uint8_t* src = (const uint8_t*)source;
    for (size_t i = 0; i < len; ++i) {
        ((uint8_t*)dest)[i] = src[i];
    }
    return dest;
}

extern "C" inline int memcmp(const void *src1, const void *src2, size_t len) {
    const auto *p1 = (const uint8_t *)src1;
    const auto *p2 = (const uint8_t *)src2;
    for (size_t i = 0; i < len; ++i) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]) ? -1 : 1;
        }
    }
    return 0;
}

extern "C" inline int strcmp(const char* src1, const char* src2) {
    while (*src1 && *src1 == *src2) { src1++; src2++; }
    return *src1 - *src2;
}

extern "C" inline size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

#endif