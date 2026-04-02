#ifndef RISC_V_THREAD_H
#define RISC_V_THREAD_H

#include "../pcb.h"
#include "../process/process.h"

class Thread : public PCB {
public:
    ~Thread() override = default;
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
    bool isProcess() override { return false; }
    void exit(int exitCode) override;
    void clear() override;

private:
    friend class Process;
    friend class PCBGarbage;

    static KMemCache<Thread>* s_cache;

    Process* m_parent;
    Thread* m_nextThread;
};

#endif