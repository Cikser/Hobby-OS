#ifndef RISC_V_SEGMENT_H
#define RISC_V_SEGMENT_H

#include "../../types.h"
#include "../kalloc/kmem_cache.h"
#include "pmt.h"

enum class SegType : uint8_t {
    TEXT = 0,
    DATA = 1,
    BSS = 2,
    HEAP = 3,
    STACK = 4,
    MMAP = 5,
};

struct SegmentDesc {

    SegType type;
    uint8_t flags;
    uint64_t start;
    uint64_t end;

    SegmentDesc* next;

    static constexpr uint8_t SEG_R = PMT::PAGE_R;
    static constexpr uint8_t SEG_W = PMT::PAGE_W;
    static constexpr uint8_t SEG_X = PMT::PAGE_X;

    uint64_t size() const { return end - start; }

    bool contains(uint64_t va) const {
        return va >= start && va < end;
    }
};

class SegmentTable {
public:
    SegmentTable();
    ~SegmentTable() { clear(); }

    static SegmentTable* copy(const SegmentTable* src);

    SegmentDesc* setText(uint8_t flags, uint64_t va_start, uint64_t va_end);
    SegmentDesc* setData(uint8_t flags, uint64_t va_start, uint64_t va_end);
    SegmentDesc* setBss(uint8_t flags, uint64_t va_start, uint64_t va_end);
    SegmentDesc* setHeap(uint8_t flags, uint64_t va_start, uint64_t va_end);
    SegmentDesc* setStack(uint8_t flags, uint64_t va_start, uint64_t va_end);

    SegmentDesc* text() const { return m_text; }
    SegmentDesc* data() const { return m_data; }
    SegmentDesc* bss() const { return m_bss; }
    SegmentDesc* heap() const { return m_heap; }
    SegmentDesc* stack() const { return m_stack; }

    SegmentDesc* addMmap(uint8_t flags, uint64_t va_start, uint64_t va_end);
    SegmentDesc* findMmap(uint64_t va) const;
    int removeMmap(uint64_t va_start);
    SegmentDesc* mmapHead() const { return m_mmap; }

    SegmentDesc* find(uint64_t va) const;

    void clear();

    void* operator new(size_t) {
        if (!s_tableCache)
            s_tableCache = new KMemCache<SegmentTable>();
        return s_tableCache->alloc();
    }
    void operator delete(void* ptr) { s_tableCache->free(ptr); }

private:
    static KMemCache<SegmentTable>* s_tableCache;
    static KMemCache<SegmentDesc>* s_descCache;

    static SegmentDesc* allocDesc(SegType type, uint8_t flags,
        uint64_t va_start, uint64_t va_end);
    static void freeDesc(SegmentDesc* desc);
    static void mmapReverse(SegmentTable* dst, const SegmentDesc* desc);

    static SegmentDesc* setSingle(SegmentDesc*& slot, SegType type,
        uint8_t flags, uint64_t va_start, uint64_t va_end);

    SegmentDesc* m_text;
    SegmentDesc* m_data;
    SegmentDesc* m_bss;
    SegmentDesc* m_heap;
    SegmentDesc* m_stack;
    SegmentDesc* m_mmap;
};

#endif