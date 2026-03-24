#include "buddy.h"
#include "../../io/console/console.h"

const uint32_t Buddy::START_POW = MemoryLayout::PAGE_SHIFT;
const uint32_t Buddy::END_POW = MemoryLayout::MEM_LIMIT_SHIFT;
const uint32_t Buddy::BUDDY_SIZE = END_POW - START_POW + 1;
const uint32_t Buddy::PAGE_SIZE = MemoryLayout::PAGE_SIZE;

int Buddy::m_buddy[BUDDY_SIZE];
void* Buddy::m_startAddr = nullptr;
Lock Buddy::m_lock;

void Buddy::init() {
    m_startAddr = (void*)MemoryLayout::pageRoundUp(MemoryLayout::HEAP_START);
    for (auto& entry: m_buddy) {
        entry = -1;
    }
    int i = BUDDY_SIZE - 1;
    uint64_t blockNum = (MemoryLayout::KERNEL_END - (uint64_t)m_startAddr) / PAGE_SIZE;
    int block = 0;
    while(blockNum > 0 && i >= 0){
        int size = 1 << i;
        if(blockNum >= size){
            if(m_buddy[i] == -1){
                m_buddy[i] = block;
                putAtBlock(block, -1);
            }
            else{
                putAtBlock(block, m_buddy[i]);
                m_buddy[i] = block;
            }
            blockNum -= size;
            block += size;
        }
        else{
            i--;
        }
    }
}

void Buddy::print() {
    for(int i = 0; i < BUDDY_SIZE; i++){
        Console::kprintf("buddy[%d]: %d ", i, m_buddy[i]);
        int block = m_buddy[i];
        while(block != -1){
            block = getAtBlock(block);
            if(block == -1) break;
            Console::kprintf("%d ", block);
        }
        Console::kprintf("\n");
    }
    Console::kprintf("\n");
}

void* Buddy::alloc(size_t size) {
    m_lock.acquire();

    int i = 0;
    while(size > (1 << (START_POW + i))){
        i++;
    }
    int entry = i, diff = 0;
    while(m_buddy[entry + diff] == -1 && entry + diff < BUDDY_SIZE){
        diff++;
    }
    if(entry + diff >= BUDDY_SIZE){
        m_lock.release();
        return nullptr;
    }
    void* alloc = nullptr;
    while(diff > 0){
        int block = m_buddy[entry + diff];
        m_buddy[entry + diff] = getAtBlock(m_buddy[entry + diff]);
        diff--;
        putAtBlock(block, m_buddy[entry + diff]);
        m_buddy[entry + diff] = block;
        block = block + (1 << (START_POW + entry + diff)) / PAGE_SIZE;
        putAtBlock(block, m_buddy[entry + diff]);
        m_buddy[entry + diff] = block;
    }
    alloc = blockToPtr(m_buddy[entry]);
    m_buddy[entry] = getAtBlock(m_buddy[entry]);

    m_lock.release();
    return alloc;
}

int getOrder(size_t size) {
    int order = 0;
    size_t n = (size + MemoryLayout::PAGE_SIZE - 1) / MemoryLayout::PAGE_SIZE;
    while (n > (1 << order)) order++;
    return order;
}

void Buddy::free(void* ptr, size_t size) {
    m_lock.acquire();

    int block = ptrToBlock(ptr);
    int entry = getOrder(size);
    putBlock(entry, block);
    merge(entry);

    m_lock.release();
}

void Buddy::putBlock(int entry, int block) {
    int curr = m_buddy[entry], prev = -1;
    while(block > curr && curr != -1){
        prev = curr;
        curr = getAtBlock(curr);
    }
    if(prev == -1){
        m_buddy[entry] = block;
        putAtBlock(block, curr);
    }
    else{
        putAtBlock(prev, block);
        putAtBlock(block, curr);
    }
}

void Buddy::merge(int entry) {
    int first = -1, second = -1, third = -1, fourth = m_buddy[entry];
    do{
        if(second != -1 && third != -1 &&
          third - second == 1 << entry &&
          second % (1 << (entry + 1)) == 0){
            putBlock(entry + 1, second);
            if(first == -1){
                m_buddy[entry] = fourth;
            }
            else{
               putAtBlock(first, fourth);
            }
            merge(entry + 1);
            break;
          }
        if(fourth == -1) break;
        first = second;
        second = third;
        third = fourth;
        fourth = getAtBlock(fourth);
    } while(third != -1);
}