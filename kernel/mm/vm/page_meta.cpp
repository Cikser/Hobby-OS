#include "page_meta.h"
#include "../../hw/memlayout.h"

KMemCache<PageMeta>* PageMeta::s_cache = nullptr;
PageMeta* PageRefCount::s_buckets[BUCKET_COUNT];
Lock PageRefCount::s_lock;
bool PageRefCount::s_initialized = false;

uint32_t PageRefCount::hash(uint64_t pa) {
    uint64_t key = pa >> MemoryLayout::PAGE_SHIFT;
    key ^= key >> 16;
    key *= 0x45d9f3b;
    return (uint32_t)(key & BUCKET_MASK);
}

PageMeta* PageRefCount::find(uint64_t pa) {
    PageMeta* node = s_buckets[hash(pa)];
    while (node) {
        if (node->pa == pa) return node;
        node = node->next;
    }
    return nullptr;
}

void PageRefCount::erase(uint64_t pa) {
    uint32_t  idx  = hash(pa);
    PageMeta* cur  = s_buckets[idx];
    PageMeta* prev = nullptr;

    while (cur) {
        if (cur->pa == pa) {
            if (prev)
                prev->next = cur->next;
            else
                s_buckets[idx] = cur->next;
            delete cur;
            return;
        }
        prev = cur;
        cur  = cur->next;
    }
}

void PageRefCount::init() {
    if (s_initialized) return;
    for (uint32_t i = 0; i < BUCKET_COUNT; i++)
        s_buckets[i] = nullptr;
    s_initialized = true;
}

void PageRefCount::incRef(uint64_t pa) {
    s_lock.acquire();

    PageMeta* node = find(pa);
    if (node) {
        node->refcount++;
    }
    else {
        auto* m = new PageMeta();
        m->pa = pa;
        m->refcount = 2;
        uint32_t idx = hash(pa);
        m->next = s_buckets[idx];
        s_buckets[idx] = m;
    }

    s_lock.release();
}

bool PageRefCount::decRef(uint64_t pa) {
    s_lock.acquire();

    PageMeta* node = find(pa);
    if (!node) {
        s_lock.release();
        return true;
    }

    node->refcount--;
    if (node->refcount == 0) {
        erase(pa);
        s_lock.release();
        return true;
    }

    s_lock.release();
    return false;
}

uint32_t PageRefCount::getRef(uint64_t pa) {
    s_lock.acquire();
    PageMeta* node = find(pa);
    uint32_t ref = node ? node->refcount : 1;
    s_lock.release();
    return ref;
}