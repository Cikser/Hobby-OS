#include "disk.h"
#include "../../hw/memlayout.h"
#include "../../mm/kalloc/kalloc.h"
#include "../console/console.h"
#include "../../mm/mem.h"

uint16_t Disk::m_usedIdx = 0;
volatile uint8_t Disk::m_status[QUEUE_SIZE / 3];
VirtioBlkReqHeader Disk::m_req[QUEUE_SIZE / 3];
uint8_t Disk::m_free[QUEUE_SIZE / 3];
VirtqAvail* Disk::m_avail = nullptr;
VirtqUsed* Disk::m_used = nullptr;
VirtqDesc* Disk::m_desc = nullptr;

void Disk::init() {
    if (readReg(VIRTIO_MAGIC) != VIRTIO_MAGIC_VALUE) {
        Console::panic("Disk::init(): wrong magic value");
    }
    if (readReg(VIRTIO_VERSION) != VIRTIO_VERSION_VALUE) {
        Console::panic("Disk::init(): wrong version value");
    }
    if (readReg(VIRTIO_DEVICE_ID) != VIRTIO_DEVICE_ID_VALUE) {
        Console::panic("Disk::init(): wrong device id");
    }

    uint32_t status = 0;
    writeReg(VIRTIO_STATUS, status |= VIRTIO_STATUS_ACK);
    writeReg(VIRTIO_STATUS, status |= VIRTIO_STATUS_DRIVER);

    writeReg(VIRTIO_DEVICE_FEATURES_SEL, 0);
    writeReg(VIRTIO_DRIVER_FEATURES_SEL, 0);
    writeReg(VIRTIO_DRIVER_FEATURES, 0);

    writeReg(VIRTIO_DEVICE_FEATURES_SEL, 1);
    writeReg(VIRTIO_DRIVER_FEATURES_SEL, 1);
    writeReg(VIRTIO_DRIVER_FEATURES, 1);

    writeReg(VIRTIO_STATUS, status |= VIRTIO_STATUS_FEATURES_OK);

    if (!(readReg(VIRTIO_STATUS) & VIRTIO_STATUS_FEATURES_OK))
        Console::panic("Disk::init(): features not accepted");

    writeReg(VIRTIO_QUEUE_SEL, 0);
    writeReg(VIRTIO_QUEUE_NUM, QUEUE_SIZE);

    m_desc = (VirtqDesc*)MemoryAllocator::kallocPage();
    m_avail = (VirtqAvail*)MemoryAllocator::kallocPage();
    m_used = (VirtqUsed*)MemoryAllocator::kallocPage();

    memset(m_desc, 0, MemoryLayout::PAGE_SIZE);
    memset(m_avail, 0, MemoryLayout::PAGE_SIZE);
    memset(m_used, 0, MemoryLayout::PAGE_SIZE);

    writeReg64(VIRTIO_QUEUE_DESC, MemoryLayout::v2p((uint64_t)m_desc));
    writeReg64(VIRTIO_QUEUE_AVAIL, MemoryLayout::v2p((uint64_t)m_avail));
    writeReg64(VIRTIO_QUEUE_USED, MemoryLayout::v2p((uint64_t)m_used));
    writeReg(VIRTIO_QUEUE_READY, 1);

    writeReg(VIRTIO_STATUS, status | VIRTIO_STATUS_DRIVER_OK);

    Console::kprintf("Disk initialized\n");
}

int Disk::allocSlot() {
    for (int i = 0; i < QUEUE_SIZE / 3; i++) {
        if (!m_free[i]) {
            m_free[i] = 1;
            return i;
        }
    }
    return -1;
}

void Disk::freeSlot(int slot) {
    m_free[slot] = 0;
}

void Disk::sendRequest(uint64_t sector, void* buf, opType op) {
    int slot = allocSlot();
    if (slot < 0)
        Console::panic("Disk::sendRequest(): no free slots");

    int d0 = slot * 3;
    int d1 = slot * 3 + 1;
    int d2 = slot * 3 + 2;

    m_req[slot].type = op;
    m_req[slot].reserved = 0;
    m_req[slot].sector = sector;
    m_status[slot] = 0xFF;

    m_desc[d0] = {MemoryLayout::v2p((uint64_t)&m_req[slot]),
        sizeof(VirtioBlkReqHeader), VIRTQ_DESC_F_NEXT, (uint16_t)d1 };
    m_desc[d1] = {MemoryLayout::v2p((uint64_t)buf),
        512, (uint16_t)(VIRTQ_DESC_F_NEXT | (op == READ ? VIRTQ_DESC_F_WRITE : 0)), (uint16_t)d2 };
    m_desc[d2] = {MemoryLayout::v2p((uint64_t)&m_status[slot]),
        1, VIRTQ_DESC_F_WRITE, 0 };

    m_avail->ring[m_avail->idx % QUEUE_SIZE] = d0;
    __sync_synchronize();
    m_avail->idx++;
    __sync_synchronize();

    writeReg(VIRTIO_QUEUE_NOTIFY, 0);

    while (m_status[slot] == 0xFF) {}
    __sync_synchronize();

    if (m_status[slot] != 0)
        Console::panic("Disk::sendRequest(): request failed");

    freeSlot(slot);
}

void Disk::read(uint64_t sector, void *buf) {
    sendRequest(sector, buf, READ);
}

void Disk::write(uint64_t sector, void *buf) {
    sendRequest(sector, buf, WRITE);
}
