#ifndef RISC_V_MMAP_H
#define RISC_V_MMAP_H

#include "../../types.h"
#include "pmt.h"
#include "segment.h"

static constexpr uint32_t MMAP_PROT_NONE = 0x0;
static constexpr uint32_t MMAP_PROT_READ = 0x1;
static constexpr uint32_t MMAP_PROT_WRITE = 0x2;
static constexpr uint32_t MMAP_PROT_EXEC = 0x4;

static constexpr uint32_t MMAP_MAP_SHARED = 0x01;
static constexpr uint32_t MMAP_MAP_PRIVATE = 0x02;
static constexpr uint32_t MMAP_MAP_FIXED = 0x10;
static constexpr uint32_t MMAP_MAP_ANONYMOUS = 0x20;

static constexpr uint64_t MMAP_FAILED = ~0ULL;

static constexpr uint64_t MMAP_TOP = 0x0000003F00000000ULL;
static constexpr uint64_t MMAP_BASE = 0x0000002000000000ULL;

struct MmapRegion {
    uint64_t vaStart;
    uint64_t vaEnd;
    uint32_t prot;
    uint32_t flags;
    int fd;
    uint64_t fileOffset;
    bool shared;

    MmapRegion* next;

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<MmapRegion>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    static KMemCache<MmapRegion>* s_cache;
};

class Mmap {
public:
    explicit Mmap(PMT* pmt, SegmentTable* segTable);
    ~Mmap();

    uint64_t map(uint64_t hint, uint64_t length, uint32_t prot,
                 uint32_t flags, int fd, uint64_t offset);

    int unmap(uint64_t addr, uint64_t length);
    int protect(uint64_t addr, uint64_t length, uint32_t prot) const;

    Mmap* clone(PMT* newPmt, SegmentTable* newSegTable) const;
    MmapRegion* find(uint64_t va) const;

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<Mmap>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    uint64_t allocVA(uint64_t hint, uint64_t pages, bool fixed) const;
    static uint64_t protToPte(uint32_t prot);
    bool mapPages(uint64_t vaStart, uint64_t pages, uint64_t pteFlags) const;
    void unmapPhysical(uint64_t vaStart, uint64_t vaEnd) const;
    void insertRegion(MmapRegion* r);
    void removeOverlapping(uint64_t addr, uint64_t len);
    bool isFree(uint64_t addr, uint64_t len) const;
    int fillFromFile(uint64_t vaStart, uint64_t pages,
                     int fd, uint64_t offset, uint64_t fileLen) const;

    PMT* m_pmt;
    SegmentTable* m_segTable;
    MmapRegion* m_head;
    uint64_t m_nextHint;

    static KMemCache<Mmap>* s_cache;

    static constexpr uint64_t PAGE_SIZE = MemoryLayout::PAGE_SIZE;
};

#endif