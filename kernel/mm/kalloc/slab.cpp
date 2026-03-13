#include "slab.h"
#include "buddy.h"
#include "kmem_cache.h"
#include "console/console.h"

Slab *Slab::create(MemCache *cache) {
    if (cache->m_objSize < OBJ_SIZE_THRESHOLD)
        return createSmall(cache);
    return createLarge(cache);
}

Slab *Slab::createSmall(MemCache *cache) {
    auto slab = (Slab*)Buddy::alloc(cache->m_slabSize);
    slab->m_cache = cache;
    slab->m_maxSlots = (cache->m_slabSize - sizeof(Slab)) / cache->m_objSize;
    slab->m_freeSlots = slab->m_maxSlots;
    slab->next = nullptr;
    slab->m_start = (void*)((uint64_t)slab + sizeof(Slab));
    slab->m_free = 0;
    auto* it = (uint8_t*)slab->m_start;
    for (uint32_t i = 0; i < slab->m_maxSlots - 1; i++) {
        *(uint32_t*)(it + i * cache->m_objSize) = i + 1;
    }
    *(uint32_t*)(it + (slab->m_maxSlots - 1) * cache->m_objSize) = FREE_END;
    return slab;
}

Slab *Slab::createLarge(MemCache *cache) {
    auto slab = (Slab*)MemCache::s_metaCache.alloc();
    slab->m_cache = cache;
    slab->m_maxSlots = (cache->m_slabSize) / cache->m_objSize;
    slab->m_freeSlots = slab->m_maxSlots;
    slab->next = nullptr;
    slab->m_free = 0;
    slab->m_start = Buddy::alloc(cache->m_slabSize);
    auto* it = (uint8_t*)slab->m_start;
    for (uint32_t i = 0; i < slab->m_maxSlots - 1; i++) {
        *(uint32_t*)(it + i * cache->m_objSize) = i + 1;
    }
    *(uint32_t*)(it + (slab->m_maxSlots - 1) * cache->m_objSize) = FREE_END;
    return slab;
}

void Slab::destroy(Slab *slab) {
    if (slab->m_cache->m_objSize < OBJ_SIZE_THRESHOLD)
        destroySmall(slab);
    else
        destroyLarge(slab);
}

void Slab::destroySmall(Slab* slab) {
    Buddy::free(slab, slab->m_cache->m_slabSize);
}

void Slab::destroyLarge(Slab *slab) {
    Buddy::free(slab->m_start, slab->m_cache->m_slabSize);
    MemCache::s_metaCache.free(slab);
}

void* Slab::getSlot() {
    if (m_freeSlots == 0) return nullptr;
    auto ret = (void*)((uint64_t)m_start + m_free * m_cache->m_objSize);
    m_free = *(uint32_t*)ret;
    m_freeSlots--;
    return ret;
}

void Slab::freeSlot(void* slot) {
    if (m_freeSlots == m_maxSlots)
        Console::panic("Slab::freeSlot: slab already empty");
    uint32_t idx = ((uint64_t)slot - (uint64_t)m_start) / m_cache->m_objSize;
    *(uint32_t*)((uint64_t)m_start + idx * m_cache->m_objSize) = m_free;
    m_free = idx;
    m_freeSlots++;
}

bool Slab::contains(void *obj) {
    return (uint64_t)obj >= (uint64_t)m_start && (uint64_t)obj < (uint64_t)this + m_cache->m_slabSize;
}

bool Slab::empty() const {
    return m_freeSlots == m_maxSlots;
}
