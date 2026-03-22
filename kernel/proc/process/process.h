#ifndef RISC_V_PROCESS_H
#define RISC_V_PROCESS_H

#include "../pcb.h"
#include "../../fs/file.h"

class Thread;

class Process : public PCB {
public:
    ~Process() override;

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<Process>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

    static Process* createInit();
    int exec(const char* elfPath) override;
    Thread* createThread(void(*entry)(void*));

    Process* fork() override;
    File* getFile(int fd) override;
    uint64_t brk(uint64_t newHeapEnd) override;
    uint64_t openFile(char* path, uint64_t flags) override;
    int closeFile(int fd) override;
    SegmentTable* segmentTable() const override { return m_segTable; };
    void exit(int exitCode) override;
    pid_t wait(pid_t pid, int* status) override;

private:
    friend class Thread;

    static constexpr uint32_t MAX_FDS = 16;
    static constexpr uint64_t HEAP_START = 0x1000000;

    static KMemCache<Process>* s_cache;

    Process(PMT* pmt, uint64_t entry, Process* parent);

    Thread* m_threads;
    Process* m_parent;
    File* m_fds[MAX_FDS]{};
    SegmentTable* m_segTable;
    Process* m_nextSibling;
    Process* m_firstChild;
    int m_exitCode;
    Semaphore m_selfSem;
    Lock m_spaceLock;
};

#endif