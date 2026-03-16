#ifndef RISC_V_VIRTIO_H
#define RISC_V_VIRTIO_H

#include "../../types.h"
#include "../../hw/memlayout.h"

static constexpr uint64_t VIRTIO_BASE = MemoryLayout::VIRTIO_BASE;
static constexpr uint32_t VIRTIO_MAGIC = 0X000;
static constexpr uint32_t VIRTIO_MAGIC_VALUE = 0x74726976;
static constexpr uint32_t VIRTIO_VERSION = 0X004;
static constexpr uint32_t VIRTIO_VERSION_VALUE = 0x002;
static constexpr uint32_t VIRTIO_DEVICE_ID = 0x008;
static constexpr uint32_t VIRTIO_DEVICE_ID_VALUE = 0x002;
static constexpr uint32_t VIRTIO_VENDOR_ID = 0x00C;
static constexpr uint32_t VIRTIO_DEVICE_FEATURES = 0x010;
static constexpr uint32_t VIRTIO_DEVICE_FEATURES_SEL = 0x014;
static constexpr uint32_t VIRTIO_DRIVER_FEATURES = 0x020;
static constexpr uint32_t VIRTIO_DRIVER_FEATURES_SEL = 0x024;
static constexpr uint32_t VIRTIO_QUEUE_SEL = 0x030;
static constexpr uint32_t VIRTIO_QUEUE_NUM_MAX = 0x034;
static constexpr uint32_t VIRTIO_QUEUE_NUM = 0x038;
static constexpr uint32_t VIRTIO_QUEUE_READY = 0x044;
static constexpr uint32_t VIRTIO_QUEUE_NOTIFY = 0x050;
static constexpr uint32_t VIRTIO_INTERRUPT_STATUS = 0x060;
static constexpr uint32_t VIRTIO_INTERRUPT_ACK = 0x064;
static constexpr uint32_t VIRTIO_STATUS = 0x070;
static constexpr uint32_t VIRTIO_STATUS_ACK = 0x001;
static constexpr uint32_t VIRTIO_STATUS_DRIVER = 0x002;
static constexpr uint32_t VIRTIO_STATUS_DRIVER_OK = 0x004;
static constexpr uint32_t VIRTIO_STATUS_FEATURES_OK = 0x008;
static constexpr uint64_t VIRTIO_QUEUE_DESC = 0x080;
static constexpr uint64_t VIRTIO_QUEUE_AVAIL = 0x090;
static constexpr uint64_t VIRTIO_QUEUE_USED = 0x0A0;
static constexpr uint32_t QUEUE_SIZE = 8;

static constexpr uint16_t VIRTQ_DESC_F_NEXT = 0x001;
static constexpr uint16_t VIRTQ_DESC_F_WRITE = 0x002;
static constexpr uint16_t VIRTQ_DESC_F_INDIRECT = 0x004;

static constexpr uint16_t VIRTQ_AVAIL_F_NO_INTERRUPT = 0x001;

static constexpr uint16_t VIRTQ_USED_F_NO_NOTIFY = 0x001;

struct VirtqDesc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct VirtqAvail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[QUEUE_SIZE];
};

struct VirtqUsed {
    uint16_t flags;
    uint16_t idx;
    struct { uint32_t id; uint32_t len; } ring[QUEUE_SIZE];
};

struct VirtioBlkReqHeader {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

#endif