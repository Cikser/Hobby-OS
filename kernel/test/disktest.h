#ifndef RISC_V_DISKTEST_H
#define RISC_V_DISKTEST_H

#include "../io/disk/disk.h"
#include "../io/console/console.h"
#include "../mm/mem.h"

#define TEST_ASSERT(cond, msg)                                              \
    do {                                                                    \
        if (cond) {                                                         \
            Console::kprintf("  [PASS] %s\n", msg);                        \
            s_passed++;                                                     \
        } else {                                                            \
            Console::kprintf("  [FAIL] %s  (line %d)\n", msg, __LINE__);   \
            s_failed++;                                                     \
        }                                                                   \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg)    TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg)    TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_NOT_NULL(p, msg) TEST_ASSERT((p) != nullptr, msg)

namespace DiskTest {

static int s_passed = 0;
static int s_failed = 0;

static constexpr uint32_t SECTOR_SIZE = 512;

static void printSectionHeader(const char* name) {
    Console::kprintf("\n=== %s ===\n", name);
}

static void printSummary() {
    Console::kprintf("\n-----------------------------\n");
    Console::kprintf("Results: %d passed, %d failed\n", s_passed, s_failed);
    Console::kprintf("-----------------------------\n");
}

// ============================================================
// Read
// ============================================================

static void test_read_sector_zero_does_not_panic() {
    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0xAA, SECTOR_SIZE);
    Disk::read(0, buf);
    TEST_ASSERT(true, "read(sector=0) completes without panic");
}

static void test_read_returns_data_in_buffer() {
    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0xAA, SECTOR_SIZE);
    Disk::read(0, buf);
    bool changed = false;
    for (int i = 0; i < SECTOR_SIZE; i++)
        if (buf[i] != 0xAA) { changed = true; break; }
    TEST_ASSERT(changed, "read(sector=0) writes something into buffer");
}

static void test_read_multiple_sectors_do_not_panic() {
    uint8_t buf[SECTOR_SIZE];
    bool ok = true;
    for (uint64_t s = 0; s < 8; s++) {
        memset(buf, 0, SECTOR_SIZE);
        Disk::read(s, buf);
    }
    TEST_ASSERT(ok, "read on sectors 0..7 completes without panic");
}

static void test_read_is_reproducible() {
    uint8_t buf1[SECTOR_SIZE];
    uint8_t buf2[SECTOR_SIZE];
    Disk::read(0, buf1);
    Disk::read(0, buf2);
    bool same = (memcmp(buf1, buf2, SECTOR_SIZE) == 0);
    TEST_ASSERT(same, "two consecutive reads of sector 0 return identical data");
}

static void test_read_different_sectors_differ() {
    uint8_t buf0[SECTOR_SIZE];
    uint8_t buf1[SECTOR_SIZE];
    memset(buf0, 0, SECTOR_SIZE);
    memset(buf1, 0, SECTOR_SIZE);

    uint8_t pattern0 = 0x11;
    uint8_t pattern1 = 0x22;
    memset(buf0, pattern0, SECTOR_SIZE);
    memset(buf1, pattern1, SECTOR_SIZE);

    Disk::write(2, buf0);
    Disk::write(3, buf1);

    uint8_t read0[SECTOR_SIZE];
    uint8_t read1[SECTOR_SIZE];
    Disk::read(2, read0);
    Disk::read(3, read1);

    bool differ = (memcmp(read0, read1, SECTOR_SIZE) != 0);
    TEST_ASSERT(differ, "sectors written with different patterns read back differently");
}

// ============================================================
// Write / read-back
// ============================================================

static void test_write_read_back_single_sector() {
    uint8_t write_buf[SECTOR_SIZE];
    uint8_t read_buf[SECTOR_SIZE];

    for (int i = 0; i < SECTOR_SIZE; i++) write_buf[i] = (uint8_t)(i & 0xFF);
    Disk::write(1, write_buf);

    memset(read_buf, 0, SECTOR_SIZE);
    Disk::read(1, read_buf);

    bool ok = (memcmp(write_buf, read_buf, SECTOR_SIZE) == 0);
    TEST_ASSERT(ok, "write then read on sector 1 returns identical data");
}

static void test_write_read_back_all_zeros() {
    uint8_t write_buf[SECTOR_SIZE];
    uint8_t read_buf[SECTOR_SIZE];

    memset(write_buf, 0x00, SECTOR_SIZE);
    Disk::write(2, write_buf);

    memset(read_buf, 0xFF, SECTOR_SIZE);
    Disk::read(2, read_buf);

    bool ok = (memcmp(write_buf, read_buf, SECTOR_SIZE) == 0);
    TEST_ASSERT(ok, "write all-zeros then read back matches");
}

static void test_write_read_back_all_ones() {
    uint8_t write_buf[SECTOR_SIZE];
    uint8_t read_buf[SECTOR_SIZE];

    memset(write_buf, 0xFF, SECTOR_SIZE);
    Disk::write(3, write_buf);

    memset(read_buf, 0x00, SECTOR_SIZE);
    Disk::read(3, read_buf);

    bool ok = (memcmp(write_buf, read_buf, SECTOR_SIZE) == 0);
    TEST_ASSERT(ok, "write all-0xFF then read back matches");
}

static void test_write_read_back_alternating_pattern() {
    uint8_t write_buf[SECTOR_SIZE];
    uint8_t read_buf[SECTOR_SIZE];

    for (int i = 0; i < SECTOR_SIZE; i++) write_buf[i] = (i % 2 == 0) ? 0xAA : 0x55;
    Disk::write(4, write_buf);

    memset(read_buf, 0, SECTOR_SIZE);
    Disk::read(4, read_buf);

    bool ok = (memcmp(write_buf, read_buf, SECTOR_SIZE) == 0);
    TEST_ASSERT(ok, "write alternating 0xAA/0x55 pattern then read back matches");
}

static void test_write_does_not_corrupt_adjacent_sector() {
    uint8_t before[SECTOR_SIZE];
    uint8_t after[SECTOR_SIZE];
    uint8_t write_buf[SECTOR_SIZE];

    memset(write_buf, 0xBB, SECTOR_SIZE);
    Disk::write(6, write_buf);

    Disk::read(5, before);

    memset(write_buf, 0xCC, SECTOR_SIZE);
    Disk::write(6, write_buf);

    Disk::read(5, after);

    bool ok = (memcmp(before, after, SECTOR_SIZE) == 0);
    TEST_ASSERT(ok, "write to sector 6 does not corrupt sector 5");
}

static void test_write_multiple_sectors_sequential() {
    uint8_t write_buf[SECTOR_SIZE];
    uint8_t read_buf[SECTOR_SIZE];
    bool ok = true;

    for (uint64_t s = 0; s < 8; s++) {
        memset(write_buf, (uint8_t)(s * 17), SECTOR_SIZE);
        Disk::write(s, write_buf);
    }

    for (uint64_t s = 0; s < 8; s++) {
        memset(read_buf, 0, SECTOR_SIZE);
        Disk::read(s, read_buf);
        for (int i = 0; i < SECTOR_SIZE; i++) {
            if (read_buf[i] != (uint8_t)(s * 17)) { ok = false; }
        }
    }

    TEST_ASSERT(ok, "8 sequential sector writes read back correctly");
}

static void test_overwrite_sector_reads_new_data() {
    uint8_t buf_old[SECTOR_SIZE];
    uint8_t buf_new[SECTOR_SIZE];
    uint8_t read_buf[SECTOR_SIZE];

    memset(buf_old, 0x11, SECTOR_SIZE);
    memset(buf_new, 0x22, SECTOR_SIZE);

    Disk::write(7, buf_old);
    Disk::write(7, buf_new);

    Disk::read(7, read_buf);

    bool ok = (memcmp(read_buf, buf_new, SECTOR_SIZE) == 0);
    TEST_ASSERT(ok, "overwritten sector reads back with new data");
}

// ============================================================
// Buffer integrity
// ============================================================

static void test_read_does_not_write_outside_buffer() {
    uint8_t guard_before[16];
    uint8_t buf[SECTOR_SIZE];
    uint8_t guard_after[16];

    memset(guard_before, 0xDE, 16);
    memset(guard_after,  0xDE, 16);
    memset(buf, 0, SECTOR_SIZE);

    Disk::read(0, buf);

    bool ok = true;
    for (int i = 0; i < 16; i++)
        if (guard_before[i] != 0xDE || guard_after[i] != 0xDE) ok = false;

    TEST_ASSERT(ok, "read does not write outside the provided buffer");
}

static void test_write_does_not_read_outside_buffer() {
    uint8_t guard_before[16];
    uint8_t buf[SECTOR_SIZE];
    uint8_t guard_after[16];

    memset(guard_before, 0xBE, 16);
    memset(guard_after,  0xBE, 16);
    memset(buf, 0x42, SECTOR_SIZE);

    Disk::write(1, buf);

    bool ok = true;
    for (int i = 0; i < 16; i++)
        if (guard_before[i] != 0xBE || guard_after[i] != 0xBE) ok = false;

    TEST_ASSERT(ok, "write does not read outside the provided buffer");
}

// ============================================================
// Entry point
// ============================================================

static void run() {
    Console::kprintf("\n========================================\n");
    Console::kprintf("  DISK TEST SUITE\n");
    Console::kprintf("========================================\n");

    printSectionHeader("Disk::read");
    test_read_sector_zero_does_not_panic();
    test_read_returns_data_in_buffer();
    test_read_multiple_sectors_do_not_panic();
    test_read_is_reproducible();
    test_read_different_sectors_differ();

    printSectionHeader("Disk::write / read-back");
    test_write_read_back_single_sector();
    test_write_read_back_all_zeros();
    test_write_read_back_all_ones();
    test_write_read_back_alternating_pattern();
    test_write_does_not_corrupt_adjacent_sector();
    test_write_multiple_sectors_sequential();
    test_overwrite_sector_reads_new_data();

    printSectionHeader("Buffer integrity");
    test_read_does_not_write_outside_buffer();
    test_write_does_not_read_outside_buffer();

    printSummary();
}

}

#endif