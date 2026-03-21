#ifndef RISC_V_THREAD_H
#define RISC_V_THREAD_H

#include "../pcb.h"
#include "../process/process.h"

class Thread : public PCB {
public:
    ~Thread() override;
    Thread(Process* parent, uint64_t entry, void* args = nullptr);
    explicit Thread(void (*entry)(void*), void* args = nullptr);

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<Thread>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

    PCB* fork() override { return m_parent->fork(); }
    File* getFile(int fd) override { return m_parent->getFile(fd); }
    uint64_t brk(uint64_t newHeapEnd) override { return m_parent->brk(newHeapEnd); }
    uint64_t openFile(char* path, uint64_t flags) override { return m_parent->openFile(path, flags); };
    int closeFile(int fd) override { return m_parent->closeFile(fd); }
    SegmentTable* segmentTable() const override { return m_parent->segmentTable(); }

private:
    friend class Process;

    static KMemCache<Thread>* s_cache;

    Process* m_parent;
    Thread* m_nextThread;
};

#endif