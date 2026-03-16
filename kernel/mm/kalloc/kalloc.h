#ifndef RISC_V_KALLOC_H
#define RISC_V_KALLOC_H

#include "kmem_cache.h"
#include "../../types.h"

class MemoryAllocator {

public:
    static void init();

    static void* kallocPage();
    static void kfreePage(void* ptr);
    static void* kallocPages(uint32_t count);
    static void kfreePages(void* ptr, uint32_t count);

    static void* kmalloc(size_t size);
    static void kfree(void* ptr);

private:
    static constexpr uint32_t SMB_START_POW = 5;
    static constexpr uint32_t SMB_SIZE = 13;
    static KBufCache m_smb[SMB_SIZE];

};

#endif //RISC_V_KALLOC_H