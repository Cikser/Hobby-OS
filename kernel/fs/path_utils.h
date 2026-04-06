#ifndef RISC_V_PATH_UTILS_H
#define RISC_V_PATH_UTILS_H

#include "../types.h"
#include "../mm/kalloc/kalloc.h"
#include "../mm/mem.h"

static constexpr uint64_t PATH_MAX = 4096;

inline char* kstrdup(const char* src, uint64_t maxLen = PATH_MAX) {
    if (!src) return nullptr;
    uint64_t len = 0;
    while (src[len] != '\0') {
        len++;
        if (len > maxLen) return nullptr;
    }
    char* dst = (char*)MemoryAllocator::kmalloc(len + 1);
    if (!dst) return nullptr;
    memcpy(dst, src, len + 1);
    return dst;
}

inline char* kstrdup_user(const char* userSrc, uint64_t maxLen = PATH_MAX) {
    if (!userSrc) return nullptr;

    uint64_t len = 0;
    while (userSrc[len] != '\0') {
        len++;
        if (len > maxLen) return nullptr;
    }

    char* dst = (char*)MemoryAllocator::kmalloc(len + 1);
    if (!dst) return nullptr;
    memcpy(dst, userSrc, len + 1);
    return dst;
}

struct AutoPath {
    char* path;
    explicit AutoPath(char* p) : path(p) {}
    ~AutoPath() {
        if (path) MemoryAllocator::kfree(path);
    }
    AutoPath(const AutoPath&) = delete;
    AutoPath& operator=(const AutoPath&) = delete;
    operator char*() const { return path; }
    operator const char*() const { return path; }
    bool valid() const { return path != nullptr; }
};

#endif