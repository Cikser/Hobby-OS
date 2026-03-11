#include "memlayout.h"
#include "ld.h"

const uint64_t MemoryLayout::KERNEL_BASE = (uint64_t)_memory_start;
const uint64_t MemoryLayout::STACK_START = (uint64_t)_memory_start;
const uint64_t MemoryLayout::STACK_SIZE = (uint64_t)_text_start - (uint64_t)_memory_start;
const uint64_t MemoryLayout::TEXT_START = (uint64_t)_text_start;
const uint64_t MemoryLayout::TEXT_SIZE = (uint64_t)_text_end - (uint64_t)_text_start;
const uint64_t MemoryLayout::HEAP_START = (uint64_t)_heap_start;
const uint64_t MemoryLayout::HEAP_SIZE = (uint64_t)_heap_size;
const uint64_t MemoryLayout::KERNEL_END = (uint64_t)_memory_end;