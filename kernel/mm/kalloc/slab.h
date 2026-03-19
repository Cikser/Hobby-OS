#ifndef RISC_V_SLAB_H
#define RISC_V_SLAB_H

#include "../../types.h"

class MemCache;

class Slab {

public:
    static Slab* create(MemCache* cache);
    static void destroy(Slab* slab);

    Slab() = delete;
    Slab(const Slab&) = delete;
    Slab operator=(const Slab&) = delete;

    void* getSlot();
    void freeSlot(void* slot);

    bool empty() const;
    bool full() const;
    bool contains(void* obj);

    Slab *next;

private:

    static Slab* createSmall(MemCache* cache);
    static Slab* createLarge(MemCache* cache);
    static void destroySmall(Slab* slab);
    static void destroyLarge(Slab* slab);

    uint32_t m_free;
    uint32_t m_freeSlots;
    uint32_t m_maxSlots;
    MemCache* m_cache;
    void* m_start;

    static constexpr uint32_t FREE_END = ~0;
    static constexpr uint64_t OBJ_SIZE_THRESHOLD = 512;

};

#endif //RISC_V_SLAB_H