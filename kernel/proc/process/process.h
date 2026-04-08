#ifndef RISC_V_PROCESS_H
#define RISC_V_PROCESS_H

#include "../../fs/fd_table.h"
#include "../../mm/vm/mmap.h"
#include "../sync/sem.h"
#include "../pcb.h"

class Thread;

class Process : public PCB {
public:
    ~Process() override = default;
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
    Thread* createThread(void(*entry)(void*), void* args = nullptr);
    char* resolveRelative(const char* path) const;
    char* cwd() const;

    Process* owner() override { return this; }
    bool isProcess() override { return true; }
    void clear() override;
    int exec(const char* elfPath);
    Process* fork();
    File* getFile(int fd) const;
    uint64_t brk(uint64_t newHeapEnd) const;
    uint64_t openFile(const char* path, uint64_t flags) const;
    int closeFile(int fd) const;
    SegmentTable* segmentTable() const { return m_segTable; };
    void exit(int exitCode) override;
    pid_t wait(pid_t pid, int* status);
    uint64_t mmap(uint64_t addr, uint64_t length, uint32_t prot, uint32_t flags,
        int fd, uint64_t offset) const;
    int mprotect(uint64_t addr, uint64_t length, uint32_t prot) const;
    int munmap(uint64_t addr, uint64_t length) const;
    int chdir(const char* path);
    int mkdir(const char* path, uint32_t mode) const;
    int fstat(int fd, InodeStat* st) const;
    void exitGroup(int exitCode = 0);

    bool checkOperation(uint64_t addr, uint64_t len, uint32_t op) const {
        if (addr > addr + len) return false;
        if (m_segTable->checkOperation(addr, addr + len, op)) return true;
        return m_mmap->checkOperation(addr, addr + len, op);
    };

private:
    friend class Thread;
    friend class PCBGarbage;

    static constexpr uint32_t MAX_FDS = 16;
    static constexpr uint64_t HEAP_START = 0x1000000;

    static KMemCache<Process>* s_cache;
    static Process* s_init;

    Process(PMT* pmt, uint64_t entry, Process* parent,
            FdTable* fdTable = nullptr);

    Thread* m_threads;
    Process* m_parent;
    File* m_fds[MAX_FDS]{};
    SegmentTable* m_segTable;
    uint32_t m_cwdInode;
    char* m_cwdPath;
    Process* m_nextSibling;
    Process* m_firstChild;
    int m_exitCode;
    Semaphore m_selfSem;
    mutable Lock m_spaceLock;
    Mmap* m_mmap;
    bool m_reaped = false;
    pid_t m_tgid;
    FdTable* m_fdTable;
};

#endif