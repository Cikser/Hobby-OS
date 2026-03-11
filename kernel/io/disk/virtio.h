#ifndef RISC_V_VIRTIO_H
#define RISC_V_VIRTIO_H

#include "../../types.h"

constexpr uint64_t VIRTIO_BASE = 0X10001000;
constexpr uint32_t VIRTIO_MAGIC = 0X000;
constexpr uint32_t VIRTIO_MAGIC_VALUE = 0x74726976;
constexpr uint32_t VIRTIO_VERSION = 0X004;
constexpr uint32_t VIRTIO_VERSION_VALUE = 0x002;
constexpr uint32_t VIRTIO_DEVICE_ID = 0x008;
constexpr uint32_t VIRTIO_DEVICE_ID_VALUE = 0x002;
constexpr uint32_t VIRTIO_VENDOR_ID = 0x00C;
constexpr uint32_t VIRTIO_DEVICE_FEATURES = 0x010;
constexpr uint32_t VIRTIO_DEVICE_FEATURES_SEL = 0x014;
constexpr uint32_t VIRTIO_DRIVER_FEATURES = 0x020;
constexpr uint32_t VIRTIO_DRIVER_FEATURES_SEL = 0x024;
constexpr uint32_t VIRTIO_QUEUE_SEL = 0x030;
constexpr uint32_t VIRTIO_QUEUE_NUM_MAX = 0x034;
constexpr uint32_t VIRTIO_QUEUE_NUM = 0x038;
constexpr uint32_t VIRTIO_QUEUE_READY = 0x044;
constexpr uint32_t VIRTIO_QUEUE_NOTIFY = 0x050;
constexpr uint32_t VIRTIO_INTERRUPT_STATUS = 0x060;
constexpr uint32_t VIRTIO_INTERRUPT_ACK = 0x064;
constexpr uint32_t VIRTIO_STATUS = 0x070;
constexpr uint32_t VIRTIO_STATUS_ACK = 0x001;
constexpr uint32_t VIRTIO_STATUS_DRIVER = 0x002;
constexpr uint32_t VIRTIO_STATUS_DRIVER_OK = 0x004;
constexpr uint32_t VIRTIO_STATUS_FEATURES_OK = 0x008;
constexpr uint64_t VIRTIO_QUEUE_DESC = 0x080;
constexpr uint64_t VIRTIO_QUEUE_AVAIL = 0x090;
constexpr uint64_t VIRTIO_QUEUE_USED = 0x0A0;
constexpr uint32_t QUEUE_SIZE = 8;

constexpr uint16_t VIRTQ_DESC_F_NEXT = 0x001;
constexpr uint16_t VIRTQ_DESC_F_WRITE = 0x002;
constexpr uint16_t VIRTQ_DESC_F_INDIRECT = 0x004;

constexpr uint16_t VIRTQ_AVAIL_F_NO_INTERRUPT = 0x001;

constexpr uint16_t VIRTQ_USED_F_NO_NOTIFY = 0x001;

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