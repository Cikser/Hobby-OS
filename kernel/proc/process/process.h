#ifndef RISC_V_PROCESS_H
#define RISC_V_PROCESS_H

#include "console.h"
#include "elf.h"
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
    Thread* cloneThread(uint64_t entry, uint64_t userStack, uint64_t tls,
                    int* parentTidPtr, int* childTidPtr, int* clearTidPtr);

    Process* owner() override { return this; }
    bool isProcess() override { return true; }
    pid_t ppid() const { return m_parent ? m_parent->m_pid : 0; }
    void clear() override;
    int exec(const char* elfPath, char* argv[] = nullptr, char* envp[] = nullptr);
    Process* fork();
    File* getFile(int fd) const;
    uint64_t brk(uint64_t newHeapEnd) const;
    uint64_t openFile(const char* path, uint64_t flags) const;
    int closeFile(int fd) const;
    SegmentTable* segmentTable() const { return m_segTable; };
    void exit(int exitCode) override;
    void exitViaSignal(int signum);

    static constexpr int WNOHANG = 1;
    static constexpr int WUNTRACED = 2;
    static constexpr int WCONTINUED = 8;

    pid_t wait(pid_t pid, int* status, int options = 0);
    uint64_t mmap(uint64_t addr, uint64_t length, uint32_t prot, uint32_t flags,
        int fd, uint64_t offset) const;
    int mprotect(uint64_t addr, uint64_t length, uint32_t prot) const;
    int munmap(uint64_t addr, uint64_t length) const;
    int chdir(const char* path);
    int mkdir(const char* path, uint32_t mode) const;
    int fstat(int fd, InodeStat* st) const;
    void exitGroup(int exitCode = 0);
    int kill(int signum);
    int sigaction(int signum, const SignalAction* act, SignalAction* oldact) const;
    int sigprocmask(int how, const uint64_t* set, uint64_t* oldset);

    bool checkOperation(uint64_t addr, uint64_t len, uint32_t op) const {
        if (addr > addr + len) return false;
        if (m_segTable->checkOperation(addr, addr + len, op)) return true;
        return m_mmap->checkOperation(addr, addr + len, op);
    }

    pid_t pgid() const { return m_pgid; }
    pid_t sid() const { return m_sid; }
    int setpgid(pid_t targetPid, pid_t pgid);
    pid_t getpgid(pid_t targetPid) const;
    pid_t setsid();
    uint32_t umask() const { return m_umask; }
    uint32_t setUmask(uint32_t mask);

    static Process* findByPid(pid_t pid);
    static void signalProcessGroup(pid_t pgid, int signum);
    void notifyStopped(int signum);
    void wakeStoppedThreads();

private:
    friend class Thread;
    friend class PCBGarbage;
    friend class SyscallHandler;

    static constexpr uint32_t MAX_FDS = 16;
    static constexpr uint64_t HEAP_START = 0x1000000;

    static KMemCache<Process>* s_cache;
    static Process* s_init;

    Process(PMT* pmt, uint64_t entry, Process* parent,
            FdTable* fdTable = nullptr);

    static Process* findProcess(pid_t pid);
    uint64_t setupInitialStack(const char* path, const ElfLoadInfo& elfInfo, uint8_t* randomBytes16,
                                char* argv[], char* evnp[]) const;

    Thread* m_threads;
    Process* m_parent;
    File* m_fds[MAX_FDS]{};
    SegmentTable* m_segTable;
    uint32_t m_cwdInode;
    char* m_cwdPath;
    Process* m_nextSibling;
    Process* m_firstChild;
    int m_exitCode;
    int m_termSignal;
    Semaphore m_selfSem;
    mutable Lock m_spaceLock;
    Mmap* m_mmap;
    bool m_reaped = false;
    FdTable* m_fdTable;

    pid_t m_pgid;
    pid_t m_sid;
    uint32_t m_umask;
    Process* m_allNext;

    static Process* s_allHead;
    static Lock s_allLock;

    static void registerProcess(Process* p);
    static void unregisterProcess(Process* p);
};

static constexpr uint64_t CSIGNAL = 0xFF;
static constexpr uint64_t SIGCHLD_FLAG = 17;
static constexpr uint64_t CLONE_VM = 0x00000100;
static constexpr uint64_t CLONE_FS = 0x00000200;
static constexpr uint64_t CLONE_FILES = 0x00000400;
static constexpr uint64_t CLONE_SIGHAND = 0x00000800;
static constexpr uint64_t CLONE_THREAD = 0x00010000;
static constexpr uint64_t CLONE_SETTLS = 0x00080000;
static constexpr uint64_t CLONE_PARENT_SETTID = 0x00100000;
static constexpr uint64_t CLONE_CHILD_CLEARTID = 0x00200000;
static constexpr uint64_t CLONE_CHILD_SETTID = 0x01000000;

#endif