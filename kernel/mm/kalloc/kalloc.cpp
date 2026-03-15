#include "kalloc.h"
#include "buddy.h"
#include "../../hw/memlayout.h"
#include "../../io/console/console.h"

KBufCache MemoryAllocator::m_smb[SMB_SIZE];

void MemoryAllocator::init() {
    Buddy::init();
    for (int i = 0; i < SMB_SIZE; i++) {
        m_smb[i].setObjSize(1 << (SMB_START_POW + i));
    }
    MemCache::s_metaCache.setObjSize(sizeof(Slab));
    MemCache::s_memCache.setObjSize(sizeof(MemCache));
    Console::kprintf("MemoryAllocator initialized\n");
}

void *MemoryAllocator::kallocPage() {
    return Buddy::alloc(MemoryLayout::PAGE_SIZE);
}

void MemoryAllocator::kfreePage(void *ptr) {
    Buddy::free(ptr, MemoryLayout::PAGE_SIZE);
}

void *MemoryAllocator::kmalloc(size_t size) {
    if (size == 0) return nullptr;
    if (size > 1 << (SMB_START_POW + SMB_SIZE - 1))
        Console::panic("MemoryAllocator::kmalloc(): allocation too big");
    int pow = SMB_START_POW;
    while (size > 1 << pow) {
        pow++;
    }
    int entry = pow - SMB_START_POW;
    return m_smb[entry].alloc();
}

void MemoryAllocator::kfree(void *ptr) {
    for (auto & cache : m_smb) {
        if (cache.free(ptr) == 0)
            return;
    }
    Console::panic("MemoryAllocator::kfree(): wrong pointer freed");
}

void* operator new(size_t size) {
    return MemoryAllocator::kmalloc(size);
}

void operator delete(void* ptr) noexcept {
    MemoryAllocator::kfree(ptr);
}

void* operator new[](size_t size) {
    return MemoryAllocator::kmalloc(size);
}

void operator delete[](void* ptr) noexcept {
    MemoryAllocator::kfree(ptr);
}