#include "segment.h"

KMemCache<SegmentTable>* SegmentTable::s_tableCache = nullptr;
KMemCache<SegmentDesc>* SegmentTable::s_descCache  = nullptr;

SegmentTable::SegmentTable()
    : m_text(nullptr), m_data(nullptr), m_bss(nullptr),
      m_heap(nullptr), m_stack(nullptr), m_mmap(nullptr)
{}

SegmentDesc* SegmentTable::allocDesc(SegType type, uint8_t flags,
                                     uint64_t va_start, uint64_t va_end)
{
    if (!s_descCache)
        s_descCache = new KMemCache<SegmentDesc>();

    auto desc = (SegmentDesc*)s_descCache->alloc();
    if (!desc) return nullptr;

    desc->type = type;
    desc->flags = flags;
    desc->start = va_start;
    desc->end = va_end;
    desc->next = nullptr;
    return desc;
}

void SegmentTable::freeDesc(SegmentDesc* desc) {
    if (desc && s_descCache)
        s_descCache->free(desc);
}

SegmentDesc* SegmentTable::setSingle(SegmentDesc*& slot, SegType type,
    uint8_t flags, uint64_t va_start, uint64_t va_end)
{
    freeDesc(slot);
    slot = allocDesc(type, flags, va_start, va_end);
    return slot;
}


SegmentDesc* SegmentTable::setText(uint8_t flags, uint64_t va_start, uint64_t va_end) {
    return setSingle(m_text,  SegType::TEXT,  flags, va_start, va_end);
}

SegmentDesc* SegmentTable::setData(uint8_t flags, uint64_t va_start, uint64_t va_end) {
    return setSingle(m_data,  SegType::DATA,  flags, va_start, va_end);
}

SegmentDesc* SegmentTable::setBss(uint8_t flags, uint64_t va_start, uint64_t va_end) {
    return setSingle(m_bss,   SegType::BSS,   flags, va_start, va_end);
}

SegmentDesc* SegmentTable::setHeap(uint8_t flags, uint64_t va_start, uint64_t va_end) {
    return setSingle(m_heap,  SegType::HEAP,  flags, va_start, va_end);
}

SegmentDesc* SegmentTable::setStack(uint8_t flags, uint64_t va_start, uint64_t va_end) {
    return setSingle(m_stack, SegType::STACK, flags, va_start, va_end);
}

SegmentDesc* SegmentTable::addMmap(uint8_t flags, uint64_t va_start, uint64_t va_end) {
    SegmentDesc* desc = allocDesc(SegType::MMAP, flags, va_start, va_end);
    if (!desc) return nullptr;

    desc->next = m_mmap;
    m_mmap = desc;
    return desc;
}

SegmentDesc* SegmentTable::findMmap(uint64_t va) const {
    for (SegmentDesc* desc = m_mmap; desc; desc = desc->next)
        if (desc->contains(va)) return desc;
    return nullptr;
}

int SegmentTable::removeMmap(uint64_t va_start) {
    SegmentDesc* prev = nullptr;
    for (SegmentDesc* desc = m_mmap; desc; desc = desc->next) {
        if (desc->start == va_start) {
            if (prev) prev->next = desc->next;
            else m_mmap = desc->next;
            freeDesc(desc);
            return 0;
        }
        prev = desc;
    }
    return -1;
}

SegmentDesc* SegmentTable::find(uint64_t va) const {
    if (m_text  && m_text->contains(va)) return m_text;
    if (m_data  && m_data->contains(va)) return m_data;
    if (m_bss   && m_bss->contains(va)) return m_bss;
    if (m_heap  && m_heap->contains(va)) return m_heap;
    if (m_stack && m_stack->contains(va)) return m_stack;

    return findMmap(va);
}

void SegmentTable::clear() {
    freeDesc(m_text); m_text = nullptr;
    freeDesc(m_data); m_data = nullptr;
    freeDesc(m_bss); m_bss = nullptr;
    freeDesc(m_heap); m_heap = nullptr;
    freeDesc(m_stack); m_stack = nullptr;

    SegmentDesc* desc = m_mmap;
    while (desc) {
        SegmentDesc* next = desc->next;
        freeDesc(desc);
        desc = next;
    }
    m_mmap = nullptr;
}

SegmentTable* SegmentTable::copy(const SegmentTable* src)
{
    if (!src) return nullptr;

    auto dst = new SegmentTable();
    if (!dst) return nullptr;

    if (src->m_text)
        dst->setText(src->m_text->flags,  src->m_text->start,  src->m_text->end);
    if (src->m_data)
        dst->setData(src->m_data->flags,  src->m_data->start,  src->m_data->end);
    if (src->m_bss)
        dst->setBss(src->m_bss->flags,   src->m_bss->start,   src->m_bss->end);
    if (src->m_heap)
        dst->setHeap(src->m_heap->flags,  src->m_heap->start,  src->m_heap->end);
    if (src->m_stack)
        dst->setStack(src->m_stack->flags, src->m_stack->start, src->m_stack->end);

    mmapReverse(dst, src->m_mmap);
    return dst;
}

void SegmentTable::mmapReverse(SegmentTable* dst, const SegmentDesc* desc) {
    if (!desc) return;
    mmapReverse(dst, desc->next);
    dst->addMmap(desc->flags, desc->start, desc->end);
}
