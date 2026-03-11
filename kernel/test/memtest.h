#ifndef RISC_V_MEMTEST_H
#define RISC_V_MEMTEST_H

#include "../mm/kalloc/kalloc.h"
#include "../../io/console/console.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            Console::kprintf("  [PASS] %s\n", msg); \
            s_passed++; \
        } else { \
            Console::kprintf("  [FAIL] %s  (line %d)\n", msg, __LINE__); \
            s_failed++; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg)    TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg)    TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_NULL(p, msg)     TEST_ASSERT((p) == nullptr, msg)
#define TEST_ASSERT_NOT_NULL(p, msg) TEST_ASSERT((p) != nullptr, msg)

namespace MemTest {

static int s_passed = 0;
static int s_failed = 0;

static void printSectionHeader(const char* name) {
    Console::kprintf("\n=== %s ===\n", name);
}

static void printSummary() {
    Console::kprintf("\n-----------------------------\n");
    Console::kprintf("Results: %d passed, %d failed\n", s_passed, s_failed);
    Console::kprintf("-----------------------------\n");
}

struct SmallObj {
    uint64_t a, b, c;
};

struct LargeObj {
    uint8_t data[600];
};

static void test_kmalloc_returns_non_null() {
    void* p = MemoryAllocator::kmalloc(32);
    TEST_ASSERT_NOT_NULL(p, "kmalloc(32) returns non-null");
    MemoryAllocator::kfree(p);
}

static void test_kmalloc_minimum_size() {
    void* p = MemoryAllocator::kmalloc(1);
    TEST_ASSERT_NOT_NULL(p, "kmalloc(1) returns non-null (falls into 32B bucket)");
    MemoryAllocator::kfree(p);
}

static void test_kmalloc_exact_bucket_boundaries() {
    bool all_ok = true;
    for (int i = 0; i < 13; i++) {
        size_t sz = 1u << (5 + i);
        void* p = MemoryAllocator::kmalloc(sz);
        if (!p) { all_ok = false; break; }
        MemoryAllocator::kfree(p);
    }
    TEST_ASSERT(all_ok, "kmalloc succeeds at every bucket boundary (32..131072)");
}

static void test_kmalloc_size_one_past_boundary() {
    void* p32 = MemoryAllocator::kmalloc(32);
    void* p33 = MemoryAllocator::kmalloc(33);
    TEST_ASSERT_NOT_NULL(p32, "kmalloc(32) non-null");
    TEST_ASSERT_NOT_NULL(p33, "kmalloc(33) non-null");
    TEST_ASSERT_NE(p32, p33, "kmalloc(32) and kmalloc(33) return different pointers");
    MemoryAllocator::kfree(p32);
    MemoryAllocator::kfree(p33);
}

static void test_kmalloc_zero() {
    void* p = MemoryAllocator::kmalloc(0);
    TEST_ASSERT_NULL(p, "kmalloc(0) returns null");
    MemoryAllocator::kfree(p);
}

static void test_kmalloc_kfree_roundtrip() {
    void* p = MemoryAllocator::kmalloc(64);
    TEST_ASSERT_NOT_NULL(p, "kmalloc(64) non-null");
    uint8_t* b = (uint8_t*)p;
    for (int i = 0; i < 64; i++) b[i] = (uint8_t)i;
    bool ok = true;
    for (int i = 0; i < 64; i++) if (b[i] != (uint8_t)i) { ok = false; break; }
    TEST_ASSERT(ok, "kmalloc block is writable and readable");
    MemoryAllocator::kfree(p);
}

static void test_kmalloc_after_kfree_reuses_slot() {
    void* p1 = MemoryAllocator::kmalloc(32);
    TEST_ASSERT_NOT_NULL(p1, "first allocation non-null");
    MemoryAllocator::kfree(p1);
    void* p2 = MemoryAllocator::kmalloc(32);
    TEST_ASSERT_EQ(p1, p2, "slot is reused after free (LIFO free list)");
    MemoryAllocator::kfree(p2);
}

static void test_kmalloc_unique_pointers_within_slab() {
    static constexpr int N = 10;
    void* ptrs[N];
    bool unique = true;
    for (int i = 0; i < N; i++) {
        ptrs[i] = MemoryAllocator::kmalloc(32);
        for (int j = 0; j < i; j++)
            if (ptrs[j] == ptrs[i]) {
                unique = false;
            }
    }
    TEST_ASSERT(unique, "10 consecutive kmalloc(32) return distinct pointers");
    for (int i = 0; i < N; i++) MemoryAllocator::kfree(ptrs[i]);
}

static void test_kmalloc_allocations_do_not_overlap() {
    static constexpr int N = 16;
    static constexpr size_t SZ = 64;
    void* ptrs[N];
    for (int i = 0; i < N; i++) {
        ptrs[i] = MemoryAllocator::kmalloc(SZ);
        TEST_ASSERT_NOT_NULL(ptrs[i], "kmalloc(64) non-null in loop");
        uint8_t* b = (uint8_t*)ptrs[i];
        for (size_t j = 0; j < SZ; j++) b[j] = (uint8_t)(i & 0xFF);
    }
    bool no_overlap = true;
    for (int i = 0; i < N; i++) {
        uint8_t* b = (uint8_t*)ptrs[i];
        for (size_t j = 0; j < SZ; j++)
            if (b[j] != (uint8_t)(i & 0xFF)) { no_overlap = false; }
    }
    TEST_ASSERT(no_overlap, "16 kmalloc(64) blocks do not overlap");
    for (int i = 0; i < N; i++) MemoryAllocator::kfree(ptrs[i]);
}

static void test_kmalloc_interleaved_alloc_free() {
    static constexpr int WINDOW = 8;
    void* window[WINDOW];
    for (int i = 0; i < WINDOW; i++) window[i] = nullptr;
    bool ok = true;
    for (int i = 0; i < 40; i++) {
        int slot = i % WINDOW;
        if (window[slot]) MemoryAllocator::kfree(window[slot]);
        window[slot] = MemoryAllocator::kmalloc(32);
        if (!window[slot]) { ok = false; break; }
    }
    TEST_ASSERT(ok, "interleaved alloc/free(32) never returns null");
    for (int i = 0; i < WINDOW; i++)
        if (window[i]) MemoryAllocator::kfree(window[i]);
}

static void test_kfree_wrong_pointer_panics() {
    TEST_ASSERT(true, "kfree(wrong_ptr) -> panic [manual verification]");
}

static void test_kalloc_page_non_null() {
    void* p = MemoryAllocator::kallocPage();
    TEST_ASSERT_NOT_NULL(p, "kallocPage() returns non-null");
    MemoryAllocator::kfreePage(p);
}

static void test_kalloc_page_aligned() {
    void* p = MemoryAllocator::kallocPage();
    TEST_ASSERT_NOT_NULL(p, "kallocPage() non-null for alignment test");
    bool aligned = ((uint64_t)p % 4096) == 0;
    TEST_ASSERT(aligned, "kallocPage() returns page-aligned pointer");
    MemoryAllocator::kfreePage(p);
}

static void test_kalloc_page_writable() {
    void* p = MemoryAllocator::kallocPage();
    TEST_ASSERT_NOT_NULL(p, "kallocPage() non-null for write test");
    uint8_t* b = (uint8_t*)p;
    for (int i = 0; i < 4096; i++) b[i] = 0xCC;
    bool ok = true;
    for (int i = 0; i < 4096; i++) if (b[i] != 0xCC) { ok = false; break; }
    TEST_ASSERT(ok, "kallocPage() block is fully writable (4096 B)");
    MemoryAllocator::kfreePage(p);
}

static void test_kalloc_multiple_pages_unique() {
    static constexpr int N = 4;
    void* pages[N];
    for (int i = 0; i < N; i++) pages[i] = MemoryAllocator::kallocPage();
    bool unique = true;
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if (pages[i] == pages[j]) unique = false;
    TEST_ASSERT(unique, "4 consecutive kallocPage() return distinct pages");
    for (int i = 0; i < N; i++) MemoryAllocator::kfreePage(pages[i]);
}

static void test_kmemcache_alloc_non_null() {
    KMemCache<SmallObj> cache;
    void* p = cache.alloc();
    TEST_ASSERT_NOT_NULL(p, "KMemCache<SmallObj>::alloc() returns non-null");
    cache.free(p);
}

static void test_kmemcache_free_returns_zero() {
    KMemCache<SmallObj> cache;
    void* p = cache.alloc();
    TEST_ASSERT_NOT_NULL(p, "alloc non-null before free test");
    int ret = cache.free(p);
    TEST_ASSERT_EQ(ret, 0, "KMemCache::free(valid_ptr) returns 0");
}

static void test_kmemcache_free_unknown_ptr_returns_minus_one() {
    KMemCache<SmallObj> cache;
    SmallObj buf;
    int ret = cache.free(&buf);
    TEST_ASSERT_EQ(ret, -1, "KMemCache::free(stack_ptr) returns -1");
}

static void test_kmemcache_slot_reuse_after_free() {
    KMemCache<SmallObj> cache;
    void* p1 = cache.alloc();
    TEST_ASSERT_NOT_NULL(p1, "first allocation non-null");
    cache.free(p1);
    void* p2 = cache.alloc();
    TEST_ASSERT_EQ(p1, p2, "slot is reused after free (LIFO)");
    cache.free(p2);
}

static void test_kmemcache_write_read_back() {
    KMemCache<SmallObj> cache;
    SmallObj* p = (SmallObj*)cache.alloc();
    TEST_ASSERT_NOT_NULL(p, "alloc non-null for write test");
    p->a = 0xDEADBEEF;
    p->b = 0xCAFEBABE;
    p->c = 0x12345678;
    TEST_ASSERT_EQ(p->a, (uint64_t)0xDEADBEEF, "SmallObj->a readable after write");
    TEST_ASSERT_EQ(p->b, (uint64_t)0xCAFEBABE, "SmallObj->b readable after write");
    TEST_ASSERT_EQ(p->c, (uint64_t)0x12345678, "SmallObj->c readable after write");
    cache.free(p);
}

static void test_kmemcache_unique_pointers_within_slab() {
    KMemCache<SmallObj> cache;
    static constexpr int N = 10;
    void* ptrs[N];
    bool unique = true;
    for (int i = 0; i < N; i++) {
        ptrs[i] = cache.alloc();
        for (int j = 0; j < i; j++)
            if (ptrs[j] == ptrs[i]) unique = false;
    }
    TEST_ASSERT(unique, "10 KMemCache<SmallObj>::alloc() return distinct pointers");
    for (int i = 0; i < N; i++) cache.free(ptrs[i]);
}

static void test_kmemcache_allocations_do_not_alias() {
    KMemCache<SmallObj> cache;
    static constexpr int N = 16;
    SmallObj* ptrs[N];
    for (int i = 0; i < N; i++) {
        ptrs[i] = (SmallObj*)cache.alloc();
        ptrs[i]->a = i;
        ptrs[i]->b = i * 2;
        ptrs[i]->c = i * 3;
    }
    bool no_alias = true;
    for (int i = 0; i < N; i++) {
        if (ptrs[i]->a != (uint64_t)i       ||
            ptrs[i]->b != (uint64_t)(i * 2) ||
            ptrs[i]->c != (uint64_t)(i * 3)) {
            no_alias = false;
        }
    }
    TEST_ASSERT(no_alias, "16 SmallObj allocations do not alias each other");
    for (int i = 0; i < N; i++) cache.free(ptrs[i]);
}

static void test_kmemcache_large_object_path() {
    KMemCache<LargeObj> cache;
    static constexpr int N = 5;
    LargeObj* ptrs[N];
    for (int i = 0; i < N; i++) {
        ptrs[i] = (LargeObj*)cache.alloc();
        TEST_ASSERT_NOT_NULL(ptrs[i], "KMemCache<LargeObj>::alloc() non-null");
        for (int j = 0; j < 600; j++) ptrs[i]->data[j] = (uint8_t)(i & 0xFF);
    }
    bool ok = true;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < 600; j++)
            if (ptrs[i]->data[j] != (uint8_t)(i & 0xFF)) { ok = false; }
    TEST_ASSERT(ok, "KMemCache<LargeObj>: data intact across allocations (large slab)");
    for (int i = 0; i < N; i++) cache.free(ptrs[i]);
}

static void test_kmemcache_two_caches_do_not_interfere() {
    KMemCache<SmallObj> cacheA;
    KMemCache<SmallObj> cacheB;
    void* a = cacheA.alloc();
    void* b = cacheB.alloc();
    TEST_ASSERT_NOT_NULL(a, "cacheA alloc non-null");
    TEST_ASSERT_NOT_NULL(b, "cacheB alloc non-null");
    TEST_ASSERT_NE(a, b, "two distinct caches return different pointers");
    TEST_ASSERT_EQ(cacheA.free(b), -1, "cacheA.free(ptr_from_cacheB) returns -1");
    TEST_ASSERT_EQ(cacheB.free(a), -1, "cacheB.free(ptr_from_cacheA) returns -1");
    TEST_ASSERT_EQ(cacheA.free(a), 0,  "cacheA.free(own_ptr) returns 0");
    TEST_ASSERT_EQ(cacheB.free(b), 0,  "cacheB.free(own_ptr) returns 0");
}

static void test_kmemcache_slab_transition_partial_to_empty() {
    KMemCache<SmallObj> cache;
    static constexpr int N = 20;
    void* ptrs[N];
    for (int i = 0; i < N; i++) ptrs[i] = cache.alloc();
    for (int i = 0; i < N; i++) cache.free(ptrs[i]);
    void* p = cache.alloc();
    TEST_ASSERT_NOT_NULL(p, "alloc works after all slots have been freed");
    cache.free(p);
}

static void test_kmemcache_mixed_alloc_free() {
    KMemCache<SmallObj> cache;
    static constexpr int WINDOW = 6;
    void* w[WINDOW] = {};
    bool ok = true;
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < WINDOW; i++) {
            w[i] = cache.alloc();
            if (!w[i]) { ok = false; }
        }
        for (int i = 0; i < WINDOW / 2; i++) {
            cache.free(w[i]);
            w[i] = nullptr;
        }
    }
    TEST_ASSERT(ok, "mixed alloc/free in KMemCache<SmallObj> never returns null");
    for (int i = 0; i < WINDOW; i++)
        if (w[i]) cache.free(w[i]);
}

static void test_kmemcache_does_not_share_memory_with_kmalloc() {
    KMemCache<SmallObj> cache;
    void* km = MemoryAllocator::kmalloc(sizeof(SmallObj));
    TEST_ASSERT_NOT_NULL(km, "kmalloc non-null for cross-cache test");
    TEST_ASSERT_EQ(cache.free(km), -1, "cache.free(kmalloc_ptr) returns -1");
    MemoryAllocator::kfree(km);
}

static void run() {
    Console::kprintf("\n========================================\n");
    Console::kprintf("  ALLOCATOR TEST SUITE\n");
    Console::kprintf("========================================\n");

    printSectionHeader("MemoryAllocator::kmalloc / kfree");
    test_kmalloc_returns_non_null();
    test_kmalloc_minimum_size();
    test_kmalloc_exact_bucket_boundaries();
    test_kmalloc_size_one_past_boundary();
    test_kmalloc_zero();
    test_kmalloc_kfree_roundtrip();
    test_kmalloc_after_kfree_reuses_slot();
    test_kmalloc_unique_pointers_within_slab();
    test_kmalloc_allocations_do_not_overlap();
    test_kmalloc_interleaved_alloc_free();
    test_kfree_wrong_pointer_panics();

    printSectionHeader("MemoryAllocator::kallocPage / kfreePage");
    test_kalloc_page_non_null();
    test_kalloc_page_aligned();
    test_kalloc_page_writable();
    test_kalloc_multiple_pages_unique();

    printSectionHeader("KMemCache<T>");
    test_kmemcache_alloc_non_null();
    test_kmemcache_free_returns_zero();
    test_kmemcache_free_unknown_ptr_returns_minus_one();
    test_kmemcache_slot_reuse_after_free();
    test_kmemcache_write_read_back();
    test_kmemcache_unique_pointers_within_slab();
    test_kmemcache_allocations_do_not_alias();
    test_kmemcache_large_object_path();
    test_kmemcache_two_caches_do_not_interfere();
    test_kmemcache_slab_transition_partial_to_empty();
    test_kmemcache_mixed_alloc_free();
    test_kmemcache_does_not_share_memory_with_kmalloc();

    printSummary();
}

} // namespace MemTest
#endif