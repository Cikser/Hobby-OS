#ifndef RISC_V_DISK_H
#define RISC_V_DISK_H

#include "virtio.h"
#include "../../types.h"
#include "../../proc/sync/sem.h"
#include "../../proc/sync/lock.h"

class Disk {

public:
    static void init();

    static void read(uint64_t sector, void* buf);
    static void write(uint64_t sector, void* buf);

    static void interruptHandler();
    static void enableInterruptMode();

    static constexpr uint32_t SECTOR_SIZE = 512;

private:
    static void writeReg(uint32_t offset, uint32_t value);
    static uint32_t readReg(uint32_t offset);
    static void writeReg64(uint32_t offset, uint64_t value);

    enum opType {READ = 0, WRITE = 1};

    static void sendRequest(uint64_t sector, void* buf, opType op);

    static uint16_t m_usedIdx;
    static VirtqAvail* m_avail;
    static VirtqUsed* m_used;
    static VirtqDesc* m_desc;
    static VirtioBlkReqHeader m_req[QUEUE_SIZE / 3];
    static volatile uint8_t m_status[QUEUE_SIZE / 3];
    static uint8_t m_free[QUEUE_SIZE / 3];
    static Semaphore* m_slotSem[QUEUE_SIZE / 3];
    static Lock m_lock;
    static bool m_interruptMode;

    static int  allocSlot();
    static void freeSlot(int slot);
};

inline void Disk::writeReg(uint32_t offset, uint32_t value) {
    *(uint32_t*)(VIRTIO_BASE + offset) = value;
}

inline uint32_t Disk::readReg(uint32_t offset) {
    return *(uint32_t*)(VIRTIO_BASE + offset);
}

inline void Disk::writeReg64(uint32_t offset, uint64_t value) {
    writeReg(offset,(uint32_t)(value & 0xFFFFFFFF));
    writeReg(offset + 4,(uint32_t)(value >> 32));
}

#endif