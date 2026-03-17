#ifndef RISC_V_PROCLIST_H
#define RISC_V_PROCLIST_H
#include "../pcb.h"

class ProcList {
public:
    ProcList() : m_head(nullptr), m_tail(nullptr) {}
    ~ProcList();

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<ProcList>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

    void put(PCB* pcb);
    PCB* get();
    bool empty() const { return m_head == nullptr; }

private:
    static KMemCache<ProcList>* s_cache;

    PCB* m_head;
    PCB* m_tail;
};

#endif