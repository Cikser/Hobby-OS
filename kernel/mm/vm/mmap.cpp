#include "mmap.h"
#include "../../fs/file.h"
#include "../../hw/memlayout.h"
#include "../../mm/mem.h"
#include "../../proc/process/process.h"

KMemCache<MmapRegion>* MmapRegion::s_cache = nullptr;
KMemCache<Mmap>* Mmap::s_cache = nullptr;

Mmap::Mmap(PMT* pmt, SegmentTable* segTable)
    : m_pmt(pmt),
      m_segTable(segTable),
      m_head(nullptr),
      m_nextHint(MMAP_TOP)
{}

Mmap::~Mmap() {
    MmapRegion* r = m_head;
    while (r) {
        unmapPhysical(r->vaStart, r->vaEnd);
        MmapRegion* next = r->next;
        delete r;
        r = next;
    }
}

uint64_t Mmap::map(uint64_t hint, uint64_t length,
                   uint32_t prot, uint32_t flags,
                   int fd, uint64_t offset)
{
    if (length == 0) return MMAP_FAILED;
    if (!(flags & MMAP_MAP_PRIVATE) && !(flags & MMAP_MAP_SHARED)) return MMAP_FAILED;
    if ((flags & MMAP_MAP_ANONYMOUS) && fd != -1) return MMAP_FAILED;
    if (!(flags & MMAP_MAP_ANONYMOUS) && fd < 0) return MMAP_FAILED;

    uint64_t roundedLen = MemoryLayout::pageRoundUp(length);
    uint64_t pages = roundedLen / PAGE_SIZE;

    bool fixed = (flags & MMAP_MAP_FIXED) != 0;
    uint64_t va = allocVA(hint, pages, fixed);
    if (va == MMAP_FAILED) return MMAP_FAILED;

    if (fixed)
        removeOverlapping(va, roundedLen);

    uint64_t pteFlags = protToPte(prot);

    if (!mapPages(va, pages, pteFlags))
        return MMAP_FAILED;

    if (!(flags & MMAP_MAP_ANONYMOUS)) {
        if (fillFromFile(va, pages, fd, offset, length) < 0) {
            unmapPhysical(va, va + roundedLen);
            return MMAP_FAILED;
        }
    }

    auto r = new MmapRegion();
    r->vaStart = va;
    r->vaEnd = va + roundedLen;
    r->prot = prot;
    r->flags = flags;
    r->fd = fd;
    r->fileOffset = offset;
    r->shared = (flags & MMAP_MAP_SHARED) != 0;
    r->next = nullptr;

    insertRegion(r);

    uint8_t segFlags = (uint8_t)(pteFlags & ~(PMT::PAGE_U | PMT::PAGE_V));
    m_segTable->addMmap(segFlags, va, va + roundedLen);

    return va;
}

int Mmap::unmap(uint64_t addr, uint64_t length) {
    if (length == 0) return -1;
    if (addr % PAGE_SIZE != 0) return -1;

    uint64_t roundedLen = MemoryLayout::pageRoundUp(length);
    uint64_t end = addr + roundedLen;

    MmapRegion* prev = nullptr;
    MmapRegion* r = m_head;

    while (r && r->vaStart < end) {
        if (r->vaEnd <= addr) {
            prev = r;
            r = r->next;
            continue;
        }

        uint64_t overlapStart = (r->vaStart > addr) ? r->vaStart : addr;
        uint64_t overlapEnd   = (r->vaEnd   < end) ? r->vaEnd : end;

        unmapPhysical(overlapStart, overlapEnd);
        m_segTable->removeMmap(overlapStart);

        bool trimLeft = (r->vaStart < addr);
        bool trimRight = (r->vaEnd > end);

        if (!trimLeft && !trimRight) {
            MmapRegion* del = r;
            r = r->next;
            if (prev) prev->next = r;
            else m_head = r;
            delete del;
            continue;
        }
        if (trimLeft && trimRight) {
            uint64_t origEnd = r->vaEnd;
            r->vaEnd = addr;
            m_segTable->addMmap(r->prot, r->vaStart, addr);

            auto* right = new MmapRegion();
            right->vaStart = end;
            right->vaEnd = origEnd;
            right->prot = r->prot;
            right->flags = r->flags;
            right->fd = r->fd;
            right->fileOffset = r->fileOffset + (end - r->vaStart);
            right->shared = r->shared;
            right->next = r->next;
            r->next = right;
            m_segTable->addMmap(right->prot, end, origEnd);

            prev = right;
            r = right->next;
            continue;

        }
        if (trimLeft) {
            r->vaEnd = addr;
            m_segTable->addMmap(r->prot, r->vaStart, addr);
            prev = r;
            r    = r->next;
            continue;

        }
        r->vaStart = end;
        r->fileOffset = r->fileOffset + (end - overlapStart);
        m_segTable->addMmap(r->prot, end, r->vaEnd);
        prev = r;
        r = r->next;
    }

    return 0;
}

int Mmap::protect(uint64_t addr, uint64_t length, uint32_t prot) const {
    if (addr % PAGE_SIZE != 0) return -1;
    if (length == 0) return -1;

    uint64_t pages = MemoryLayout::pageRoundUp(length) / PAGE_SIZE;
    uint64_t end = addr + pages * PAGE_SIZE;

    uint64_t pteFlags = protToPte(prot);

    for (uint64_t va = addr; va < end; va += PAGE_SIZE) {
        uint64_t pa = m_pmt->translate(va);
        if (!pa) continue;
        m_pmt->unmapPage(va);
        m_pmt->mapPage(va, pa, pteFlags);
    }

    for (MmapRegion* r = m_head; r; r = r->next) {
        if (r->vaEnd <= addr || r->vaStart >= end) continue;
        r->prot = prot;
        m_segTable->removeMmap(r->vaStart);
        uint8_t segFlags = (uint8_t)(pteFlags & ~(PMT::PAGE_U | PMT::PAGE_V));
        m_segTable->addMmap(segFlags, r->vaStart, r->vaEnd);
    }

    return 0;
}

Mmap* Mmap::clone(PMT* newPmt, SegmentTable* newSegTable) const {
    auto* child = new Mmap(newPmt, newSegTable);
    if (!child) return nullptr;

    for (MmapRegion* r = m_head; r; r = r->next) {
        uint64_t pages    = (r->vaEnd - r->vaStart) / PAGE_SIZE;
        uint64_t pteFlags = protToPte(r->prot);

        if (r->shared) {
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t va = r->vaStart + i * PAGE_SIZE;
                uint64_t pa = m_pmt->translate(va);
                if (pa) newPmt->mapPage(va, pa, pteFlags);
            }
        } else {
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t va     = r->vaStart + i * PAGE_SIZE;
                uint64_t srcPa  = m_pmt->translate(va);
                if (!srcPa) continue;

                void* newPage = MemoryAllocator::kallocPage();
                if (!newPage) { delete child; return nullptr; }

                memcpy(newPage,
                       (void*)MemoryLayout::p2v(srcPa),
                       PAGE_SIZE);

                uint64_t newPa = MemoryLayout::v2p((uint64_t)newPage);
                newPmt->mapPage(va, newPa, pteFlags);
            }
        }

        auto* nr = new MmapRegion();
        nr->vaStart = r->vaStart;
        nr->vaEnd = r->vaEnd;
        nr->prot = r->prot;
        nr->flags = r->flags;
        nr->fd = r->fd;
        nr->fileOffset = r->fileOffset;
        nr->shared = r->shared;
        nr->next = nullptr;
        child->insertRegion(nr);

        uint8_t segFlags = (uint8_t)(pteFlags & ~(PMT::PAGE_U | PMT::PAGE_V));
        newSegTable->addMmap(segFlags, nr->vaStart, nr->vaEnd);
    }

    child->m_nextHint = m_nextHint;
    return child;
}

MmapRegion* Mmap::find(uint64_t va) const {
    for (MmapRegion* r = m_head; r; r = r->next)
        if (va >= r->vaStart && va < r->vaEnd) return r;
    return nullptr;
}

uint64_t Mmap::allocVA(uint64_t hint, uint64_t pages, bool fixed) const {
    uint64_t len = pages * PAGE_SIZE;

    if (fixed) {
        if (hint == 0) return MMAP_FAILED;
        if (hint % PAGE_SIZE != 0) return MMAP_FAILED;
        if (hint < MMAP_BASE || hint + len > MMAP_TOP) return MMAP_FAILED;
        return hint;
    }

    if (hint != 0 && hint % PAGE_SIZE == 0 &&
        hint >= MMAP_BASE && hint + len <= MMAP_TOP &&
        isFree(hint, len)) {
        return hint;
    }

    if (m_nextHint < len + MMAP_BASE) return MMAP_FAILED;
    uint64_t candidate = (m_nextHint - len) & ~(PAGE_SIZE - 1);

    for (;;) {
        if (candidate < MMAP_BASE) return MMAP_FAILED;
        if (isFree(candidate, len)) {
            const_cast<Mmap*>(this)->m_nextHint = candidate;
            return candidate;
        }
        if (candidate < PAGE_SIZE) return MMAP_FAILED;
        candidate -= PAGE_SIZE;
    }
}

uint64_t Mmap::protToPte(uint32_t prot) {
    uint64_t flags = PMT::PAGE_V | PMT::PAGE_U;
    if (prot & MMAP_PROT_READ) flags |= PMT::PAGE_R;
    if (prot & MMAP_PROT_WRITE) flags |= PMT::PAGE_W;
    if (prot & MMAP_PROT_EXEC) flags |= PMT::PAGE_X;
    return flags;
}

bool Mmap::mapPages(uint64_t vaStart, uint64_t pages, uint64_t pteFlags) const {
    for (uint64_t i = 0; i < pages; i++) {
        void* page = MemoryAllocator::kallocPage();
        if (!page) {
            for (uint64_t j = 0; j < i; j++) {
                uint64_t va = vaStart + j * PAGE_SIZE;
                uint64_t pa = m_pmt->translate(va);
                m_pmt->unmapPage(va);
                if (pa) MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pa));
            }
            return false;
        }
        memset(page, 0, PAGE_SIZE);
        uint64_t pa = MemoryLayout::v2p((uint64_t)page);
        if (!m_pmt->mapPage(vaStart + i * PAGE_SIZE, pa, pteFlags)) {
            MemoryAllocator::kfreePage(page);
            for (uint64_t j = 0; j < i; j++) {
                uint64_t va = vaStart + j * PAGE_SIZE;
                uint64_t pa2 = m_pmt->translate(va);
                m_pmt->unmapPage(va);
                if (pa2) MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pa2));
            }
            return false;
        }
    }
    return true;
}

void Mmap::unmapPhysical(uint64_t vaStart, uint64_t vaEnd) const {
    for (uint64_t va = vaStart; va < vaEnd; va += PAGE_SIZE) {
        uint64_t pa = m_pmt->unmapPage(va);
        if (pa) MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pa));
    }
}

void Mmap::insertRegion(MmapRegion* r) {
    MmapRegion* prev = nullptr;
    MmapRegion* cur = m_head;
    while (cur && cur->vaStart < r->vaStart) {
        prev = cur;
        cur = cur->next;
    }
    r->next = cur;
    if (prev) prev->next = r;
    else m_head = r;
}

void Mmap::removeOverlapping(uint64_t addr, uint64_t len) {
    unmap(addr, len);
}

bool Mmap::isFree(uint64_t addr, uint64_t len) const {
    uint64_t end = addr + len;
    for (MmapRegion* r = m_head; r; r = r->next) {
        if (r->vaStart >= end) break;
        if (r->vaEnd > addr) return false;
    }
    return true;
}

int Mmap::fillFromFile(uint64_t vaStart, uint64_t pages,
                       int fd, uint64_t offset, uint64_t fileLen) const {
    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return -1;

    uint64_t remaining = fileLen;
    uint64_t va = vaStart;

    for (uint64_t i = 0; i < pages && remaining > 0; i++, va += PAGE_SIZE) {
        uint64_t pa  = m_pmt->translate(va);
        if (!pa) return -1;
        auto kva = (void*)MemoryLayout::p2v(pa);

        uint64_t toRead = (remaining < PAGE_SIZE) ? remaining : PAGE_SIZE;

        int n = file->seek(offset + i * PAGE_SIZE, File::SEEK_SET);
        if (n < 0) return -1;

        n = file->read(kva, toRead);
        if (n < 0) return -1;

        remaining -= (uint64_t)n;
    }
    return 0;
}
