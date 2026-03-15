#include "kmem_cache.h"
#include "../../io/console/console.h"
#include "../../hw/memlayout.h"

MemCache MemCache::s_metaCache(sizeof(Slab));
MemCache MemCache::s_memCache(sizeof(MemCache));

MemCache::MemCache(const uint64_t objSize) : m_full(nullptr), m_empty(nullptr),
                                            m_partial(nullptr), m_objSize(objSize) {
    int pow = MemoryLayout::PAGE_SHIFT;
    while (m_objSize > 1 << pow) pow++;
    m_slabSize = 1 << pow;
}

void* MemCache::alloc() {
    void* ret = nullptr;
    if (!m_partial) {
        if (!m_empty) {
            m_empty = Slab::create(this);
        }
        m_partial = m_empty;
        m_empty = m_empty->next;
    }
    ret = m_partial->getSlot();
    if (m_partial->empty()) {
        Slab* temp = m_partial->next;
        m_partial->next = m_empty;
        m_empty = m_partial;
        m_partial = temp;
    }
    return ret;
}

int MemCache::free(void *obj) {
    Slab* it = m_partial, *prev = nullptr;
    while (it) {
        if (it->contains(obj)) {
            it->freeSlot(obj);
            if (it->empty()) {
                if (it == m_partial) {
                    m_partial = it->next;
                }
                else {
                    prev->next = it->next;
                }
                it->next = m_empty;
                m_empty = it;
            }
            return 0;
        }
        prev = it;
        it = it->next;
    }
    it = m_full, prev = nullptr;
    while (it) {
        if (it->contains(obj)) {
            it->freeSlot(obj);
            if(it == m_full){
                m_full = it->next;
            }
            else{
                prev->next = it->next;
            }
            it->next = m_partial;
            m_partial = it;
            return 0;
        }
        prev = it;
        it = it->next;
    }
    return -1;
}

void MemCache::shrink() {
    while (m_empty) {
        Slab* it = m_empty;
        m_empty = m_empty->next;
        Slab::destroy(it);
    }
}

void MemCache::setObjSize(const uint64_t objSize) {
    m_objSize = objSize;
    int pow = MemoryLayout::PAGE_SHIFT;
    while (m_objSize > 1 << pow) pow++;
    m_slabSize = 1 << pow;
}

