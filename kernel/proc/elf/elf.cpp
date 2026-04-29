#include "elf.h"
#include "../../mm/vm/segment.h"
#include "../pcb.h"
#include "../../fs/file.h"
#include "../../fs/vfs.h"
#include "../../mm/mem.h"

bool ElfLoader::validateHeader(const Elf64Header& hdr) {
    if (hdr.e_ident[0] != ELF_MAG0 || hdr.e_ident[1] != ELF_MAG1 ||
        hdr.e_ident[2] != ELF_MAG2 || hdr.e_ident[3] != ELF_MAG3) {
        return false;
    }

    if (hdr.e_ident[4] != ELF_CLASS_64) return false;
    if (hdr.e_ident[5] != ELF_DATA_2_LSB) return false;

    if (hdr.e_type != ET_EXEC) return false;
    if (hdr.e_machine != EM_RISCV) return false;

    return true;
}

uint64_t ElfLoader::flagsToPte(uint32_t flags) {
    uint64_t pte = PMT::PAGE_U | PMT::PAGE_V;
    if (flags & PF_R) pte |= PMT::PAGE_R;
    if (flags & PF_W) pte |= PMT::PAGE_W;
    if (flags & PF_X) pte |= PMT::PAGE_X;
    return pte;
}

ElfLoadInfo* ElfLoader::load(const char* path, PMT* pmt, SegmentTable* segTable) {
    File* file = VFS::open(path, File::O_RDONLY);
    if (!file) return nullptr;

    Elf64Header header;
    file->read(&header, sizeof(header));
    if (!validateHeader(header)) {
        file->close();
        return nullptr;
    }
    auto* outInfo = (ElfLoadInfo*)MemoryAllocator::kmalloc(sizeof(ElfLoadInfo));

    outInfo->entry = header.e_entry;
    outInfo->phent = header.e_phentsize;
    outInfo->phnum = header.e_phnum;
    outInfo->phdr_va = 0;

    for (uint16_t i = 0; i < header.e_phnum; i++) {
        Elf64ProgramHeader programHeader;
        file->seek(header.e_phoff + i * sizeof(Elf64ProgramHeader), File::SEEK_SET);
        file->read(&programHeader, sizeof(programHeader));

        if (programHeader.p_type != PT_LOAD) continue;
        if (programHeader.p_memsz == 0) continue;

        uint64_t vaddr_aligned = MemoryLayout::pageRoundDown(programHeader.p_vaddr);
        uint64_t offset_in_page = programHeader.p_vaddr - vaddr_aligned;

        uint64_t pages = (offset_in_page + programHeader.p_memsz +
            MemoryLayout::PAGE_SIZE - 1) / MemoryLayout::PAGE_SIZE;

        auto mem = MemoryAllocator::kallocPages(pages);
        if (!mem) {
            file->close();
            MemoryAllocator::kfree(outInfo);
            return nullptr;
        }
        memset(mem, 0, pages * MemoryLayout::PAGE_SIZE);
        file->seek(programHeader.p_offset, File::SEEK_SET);
        file->read((uint8_t*)mem + offset_in_page, programHeader.p_filesz);

        uint64_t pa = MemoryLayout::v2p((uint64_t)mem);
        uint64_t flags = flagsToPte(programHeader.p_flags);
        pmt->mapPages(vaddr_aligned, pa, pages, flags);

        if (programHeader.p_offset == 0) {
            outInfo->phdr_va = programHeader.p_vaddr + header.e_phoff;
        }

        if (!segTable) continue;

        uint64_t vaEnd = vaddr_aligned + pages * MemoryLayout::PAGE_SIZE;
        uint8_t segFlags = (uint8_t)(flags & ~(PMT::PAGE_U | PMT::PAGE_V));

        if (programHeader.p_flags & PF_X) {
            segTable->setText(segFlags, vaddr_aligned, vaEnd);
        }
        else if ((programHeader.p_flags & (PF_R | PF_W)) == PF_R) {
            segTable->setRoData(segFlags, vaddr_aligned, vaEnd);
        }
        else if (programHeader.p_filesz == programHeader.p_memsz) {
            segTable->setData(segFlags, vaddr_aligned, vaEnd);
        }
        else {
            uint64_t dataEnd = MemoryLayout::pageRoundUp(
                vaddr_aligned + offset_in_page + programHeader.p_filesz);
            if (dataEnd > vaddr_aligned)
                segTable->setData(segFlags & ~SegmentDesc::SEG_X,
                                  vaddr_aligned, dataEnd);
            if (dataEnd < vaEnd)
                segTable->setBss(SegmentDesc::SEG_R | SegmentDesc::SEG_W,
                                 dataEnd, vaEnd);
        }
    }
    file->close();
    if (outInfo->phdr_va == 0) {
        outInfo->phdr_va = MemoryLayout::pageRoundDown(header.e_entry) + header.e_phoff;
    }
    return outInfo;
}
