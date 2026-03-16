#ifndef RISC_V_LD_H
#define RISC_V_LD_H

#pragma once

extern "C" char _text_start[];
extern "C" char _text_end[];
extern "C" char _rodata_start[];
extern "C" char _rodata_end[];
extern "C" char _data_start[];
extern "C" char _data_end[];
extern "C" char _bss_start[];
extern "C" char _bss_end[];
extern "C" char _heap_start[];
extern "C" char _memory_start[];
extern "C" char _memory_end[];

extern "C" char _kernel_phys_start[];
extern "C" char _kernel_phys_end[];
extern "C" char _kernel_virt_base[];
extern "C" char _kernel_offset[];

extern "C" char _text_start_virt[];
extern "C" char _text_end_virt[];
extern "C" char _bss_start_virt[];
extern "C" char _bss_end_virt[];
extern "C" char _heap_start_virt[];
extern "C" char _memory_end_virt[];

#endif //RISC_V_LD_H