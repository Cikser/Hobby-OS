#ifndef RISC_V_ELF_H
#define RISC_V_ELF_H

#include "../../types.h"

class PMT;
class SegmentTable;

struct Elf64Header {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint64_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64ProgramHeader {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

static constexpr uint32_t PT_LOAD = 0x1;

static constexpr uint32_t PF_X = 0x1;
static constexpr uint32_t PF_W = 0x2;
static constexpr uint32_t PF_R = 0x4;

static constexpr uint8_t ELF_MAG0 = 0x7f;
static constexpr uint8_t ELF_MAG1 = 'E';
static constexpr uint8_t ELF_MAG2 = 'L';
static constexpr uint8_t ELF_MAG3 = 'F';
static constexpr uint8_t ELF_CLASS_64 = 2;
static constexpr uint8_t ELF_DATA_2_LSB = 1;
static constexpr uint16_t ET_EXEC = 2;
static constexpr uint16_t EM_RISCV = 243;


class ElfLoader {
public:
    static uint64_t load(const char* path, PMT* pmt, SegmentTable* segTable = nullptr);

private:
    static bool validateHeader(const Elf64Header& hdr);
    static uint64_t flagsToPte(uint32_t flags);
    static uint8_t  elfFlagsToSegFlags(uint32_t pflags);
};

#endif