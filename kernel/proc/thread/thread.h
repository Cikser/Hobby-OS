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

    Process* owner() override { return m_parent; }
    PCB* fork() override { return m_parent->fork(); }
    File* getFile(int fd) override { return m_parent->getFile(fd); }
    uint64_t brk(uint64_t newHeapEnd) override { return m_parent->brk(newHeapEnd); }
    uint64_t openFile(char* path, uint64_t flags) override { return m_parent->openFile(path, flags); };
    int closeFile(int fd) override { return m_parent->closeFile(fd); }
    SegmentTable* segmentTable() const override { return m_parent->segmentTable(); }
    void exit(int exitCode) override;
    pid_t wait(pid_t pid, int* status) override { return -1; }
    int exec(const char* elfPath) override { return -1; }
    uint64_t mmap(uint64_t addr, uint64_t length, uint32_t prot, uint32_t flags,
        int fd, uint64_t offset) override { return m_parent->mmap(addr, length, prot, flags, fd, offset); }
    int mprotect(uint64_t addr, uint64_t length, uint32_t prot) override {
        return m_parent->mprotect(addr, length, prot);
    }
    int munmap(uint64_t addr, uint64_t length) override { return m_parent->munmap(addr, length); }
    int getcwd(char* buf, size_t size) override { return m_parent->getcwd(buf, size); }
    int chdir(const char* path) override { return m_parent->chdir(path); }
    int mkdir(const char* path, uint32_t mode) override { return m_parent->mkdir(path, mode); }

private:
    friend class Process;

    static KMemCache<Thread>* s_cache;

    Process* m_parent;
    Thread* m_nextThread;
};

#endif