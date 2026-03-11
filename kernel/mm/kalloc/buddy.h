#ifndef RISC_V_BUDDY_H
#define RISC_V_BUDDY_H

#include "../../types.h"
#include "memlayout.h"

class Buddy {

public:
    static void init();
    static void* alloc(size_t size);
    static void free(void* ptr, size_t size);

    static void print();

private:

    friend class MemoryAllocator;

    static const uint32_t START_POW;
    static const uint32_t END_POW;
    static const uint32_t BUDDY_SIZE;
    static const uint32_t PAGE_SIZE;

    static void* m_startAddr;
    static int m_buddy[];

    static void* blockToPtr(const uint64_t block) { return (void*)((uint64_t)m_startAddr + block * MemoryLayout::PAGE_SIZE); }
    static int ptrToBlock(const void* ptr) { return (int)(((uint64_t)ptr - (uint64_t)m_startAddr) / MemoryLayout::PAGE_SIZE); }
    static void putAtBlock(const uint64_t block, const int value) { *(int*)blockToPtr(block) = value; }
    static int getAtBlock(const uint64_t block) { return *(int*)blockToPtr(block); }

    static void merge(int entry);
    static void putBlock(int entry, int block);

};

#endif //RISC_V_BUDDY_H