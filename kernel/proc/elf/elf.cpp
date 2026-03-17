#include "elf.h"
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

uint64_t ElfLoader::load(const char* path, PMT* pmt) {
    File* file = VFS::open(path, File::O_RDONLY);
    if (!file) return 0;

    Elf64Header header;
    file->read(&header, sizeof(header));
    if (!validateHeader(header)) {
        file->close();
        return 0;
    }
    for (uint16_t i = 0; i < header.e_phnum; i++) {
        Elf64ProgramHeader programHeader;
        file->seek(header.e_phoff + i * sizeof(Elf64ProgramHeader));
        file->read(&programHeader, sizeof(programHeader));

        if (programHeader.p_type != PT_LOAD) continue;
        if (programHeader.p_memsz == 0) continue;

        uint64_t pages = (programHeader.p_memsz + MemoryLayout::PAGE_SIZE - 1) / MemoryLayout::PAGE_SIZE;
        auto mem = MemoryAllocator::kallocPages(pages);
        if (!mem) {
            file->close();
            return 0;
        }
        memset(mem, 0, pages * MemoryLayout::PAGE_SIZE);
        file->seek(programHeader.p_offset);
        file->read(mem, programHeader.p_filesz);
        uint64_t va = MemoryLayout::pageRoundDown(programHeader.p_vaddr);
        uint64_t pa = MemoryLayout::v2p((uint64_t)mem);
        uint64_t flags = flagsToPte(programHeader.p_flags);
        pmt->mapPages(va, pa, pages, flags);
    }
    file->close();
    return header.e_entry;
}
