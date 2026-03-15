#ifndef RISC_V_FSTEST_H
#define RISC_V_FSTEST_H

#include "../fs/file.h"
#include "../fs/vfs.h"
#include "../fs/ext2/ext2.h"
#include "../io/console/console.h"

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

#define TEST_ASSERT_EQ(a, b, msg)      TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg)      TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_NULL(p, msg)       TEST_ASSERT((p) == nullptr, msg)
#define TEST_ASSERT_NOT_NULL(p, msg)   TEST_ASSERT((p) != nullptr, msg)
#define TEST_ASSERT_STR(a, b, msg)     TEST_ASSERT(strcmp((a), (b)) == 0, msg)

namespace FsTest {

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

// ─── helpers ────────────────────────────────────────────────────────────────

static bool dirContains(const char* dirPath, const char* name) {
    VfsInode* root = nullptr;
    // otvorimo direktorijum kroz resolvePath indirektno — koristimo VFS::open
    // na svakom child-u; lakše je provjeriti kroz readdir na inode-u
    // Dohvatimo inode direktorijuma otvaranjem pa odmah zatvaramo
    File* dummy = VFS::open(dirPath, File::O_RDONLY);
    if (!dummy) return false;
    dummy->close();
    delete dummy;

    // koristimo getRoot/resolvePath indirektno — jednostavnije je
    // samo pokusati da otvorimo putanju
    char path[256];
    int i = 0;
    while (dirPath[i]) { path[i] = dirPath[i]; i++; }
    if (path[i-1] != '/') path[i++] = '/';
    int j = 0;
    while (name[j]) path[i++] = name[j++];
    path[i] = '\0';

    File* f = VFS::open(path, File::O_RDONLY);
    if (!f) return false;
    f->close();
    delete f;
    return true;
}

static int readFile(const char* path, char* buf, int maxLen) {
    File* f = VFS::open(path, File::O_RDONLY);
    if (!f) return -1;
    int n = f->read(buf, maxLen);
    f->close();
    delete f;
    return n;
}

// ─── read tests ─────────────────────────────────────────────────────────────

static void test_open_existing_file() {
    File* f = VFS::open("/readme.txt", File::O_RDONLY);
    TEST_ASSERT_NOT_NULL(f, "open existing file returns non-null");
    if (f) { f->close(); delete f; }
}

static void test_open_nonexistent_file() {
    File* f = VFS::open("/nonexistent.txt", File::O_RDONLY);
    TEST_ASSERT_NULL(f, "open nonexistent file returns null");
}

static void test_read_content() {
    char buf[64] = {};
    int n = readFile("/readme.txt", buf, 63);
    TEST_ASSERT(n > 0, "read returns positive byte count");
    TEST_ASSERT_STR(buf, "hello kernel\n", "read content matches expected");
}

static void test_read_subdir_file() {
    char buf[64] = {};
    int n = readFile("/subdir/nested.txt", buf, 63);
    TEST_ASSERT(n > 0, "read file in subdirectory returns positive count");
    TEST_ASSERT_STR(buf, "nested file\n", "subdir file content matches expected");
}

static void test_read_returns_zero_at_eof() {
    File* f = VFS::open("/readme.txt", File::O_RDONLY);
    TEST_ASSERT_NOT_NULL(f, "open for eof test");
    if (!f) return;
    char buf[64] = {};
    f->read(buf, 63);           // procitaj sve
    int n = f->read(buf, 63);   // jos jednom — treba da vrati 0 ili -1
    TEST_ASSERT(n <= 0, "read at EOF returns <= 0");
    f->close();
    delete f;
}

static void test_seek_and_read() {
    File* f = VFS::open("/readme.txt", File::O_RDONLY);
    TEST_ASSERT_NOT_NULL(f, "open for seek test");
    if (!f) return;
    f->seek(6);
    char buf[32] = {};
    int n = f->read(buf, 6);
    TEST_ASSERT(n > 0, "read after seek returns positive count");
    TEST_ASSERT_STR(buf, "kernel", "read after seek(6) returns correct content");
    f->close();
    delete f;
}

static void test_tell_after_read() {
    File* f = VFS::open("/readme.txt", File::O_RDONLY);
    TEST_ASSERT_NOT_NULL(f, "open for tell test");
    if (!f) return;
    char buf[8] = {};
    f->read(buf, 5);
    TEST_ASSERT_EQ(f->tell(), (uint64_t)5, "tell() returns 5 after reading 5 bytes");
    f->close();
    delete f;
}

static void test_seek_beyond_eof_fails() {
    File* f = VFS::open("/readme.txt", File::O_RDONLY);
    TEST_ASSERT_NOT_NULL(f, "open for seek-beyond-eof test");
    if (!f) return;
    int ret = f->seek(99999);
    TEST_ASSERT_EQ(ret, -1, "seek beyond EOF returns -1");
    f->close();
    delete f;
}

static void test_read_large_file() {
    char buf[4096 + 64] = {};
    int n = readFile("/large.txt", buf, sizeof(buf) - 1);
    TEST_ASSERT(n > 4096, "read large file (>1 block) returns more than 4096 bytes");
}

static void test_open_directory_as_file_fails() {
    // direktorijum nema O_RDONLY read sadrzaj kao fajl — read treba da vrati -1
    File* f = VFS::open("/subdir", File::O_RDONLY);
    if (!f) {
        TEST_ASSERT(true, "open directory returns null (acceptable)");
        return;
    }
    char buf[64] = {};
    int n = f->read(buf, 63);
    TEST_ASSERT(n <= 0, "read on directory inode returns <= 0");
    f->close();
    delete f;
}

// ─── write tests ────────────────────────────────────────────────────────────

static void test_write_existing_file() {
    File* f = VFS::open("/writable.txt", File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "open writable file with O_RDWR");
    if (!f) return;
    int n = f->write("overwritten", 11);
    TEST_ASSERT_EQ(n, 11, "write returns correct byte count");
    f->close();
    delete f;
}

static void test_write_and_read_back() {
    const char* msg = "kernel write test";
    int msgLen = 17;

    File* f = VFS::open("/writable.txt", File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "open for write-read-back");
    if (!f) return;
    f->write(msg, msgLen);
    f->seek(0);
    char buf[64] = {};
    int n = f->read(buf, 63);
    TEST_ASSERT(n > 0, "read after write returns positive count");
    TEST_ASSERT_STR(buf, msg, "read-back content matches written content");
    f->close();
    delete f;
}

static void test_write_readonly_fails() {
    File* f = VFS::open("/readme.txt", File::O_RDONLY);
    TEST_ASSERT_NOT_NULL(f, "open readonly file");
    if (!f) return;
    int n = f->write("x", 1);
    TEST_ASSERT_EQ(n, -1, "write on O_RDONLY file returns -1");
    f->close();
    delete f;
}

static void test_create_new_file() {
    File* f = VFS::open("/newfile.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "open with O_CREAT creates new file");
    if (!f) return;
    f->close();
    delete f;
}

static void test_create_write_read_back() {
    File* f = VFS::open("/created.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create new file");
    if (!f) return;
    int n = f->write("created!", 8);
    TEST_ASSERT_EQ(n, 8, "write to created file returns 8");
    f->seek(0);
    char buf[32] = {};
    n = f->read(buf, 31);
    TEST_ASSERT(n > 0, "read from created file returns positive count");
    TEST_ASSERT_STR(buf, "created!", "created file content matches");
    f->close();
    delete f;
}

static void test_create_persists_after_reopen() {
    // napravi
    File* f = VFS::open("/persistent.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create persistent.txt");
    if (!f) return;
    f->write("persist", 7);
    f->close();
    delete f;

    // otvori ponovo
    char buf[32] = {};
    int n = readFile("/persistent.txt", buf, 31);
    TEST_ASSERT(n > 0, "reopen created file returns positive count");
    TEST_ASSERT_STR(buf, "persist", "content persists after reopen");
}

// ─── mkdir tests ────────────────────────────────────────────────────────────

static void test_mkdir_creates_directory() {
    int ret = VFS::mkdir("/newdir");
    TEST_ASSERT_EQ(ret, 0, "mkdir returns 0");
}

static void test_mkdir_then_create_file_inside() {
    VFS::mkdir("/testdir");
    File* f = VFS::open("/testdir/inside.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create file inside newly created dir");
    if (!f) return;
    f->write("inside", 6);
    f->close();
    delete f;

    char buf[32] = {};
    int n = readFile("/testdir/inside.txt", buf, 31);
    TEST_ASSERT(n > 0, "read file inside new dir returns positive count");
    TEST_ASSERT_STR(buf, "inside", "content of file inside new dir matches");
}

static void test_mkdir_nested() {
    VFS::mkdir("/a");
    VFS::mkdir("/a/b");
    File* f = VFS::open("/a/b/deep.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create file in nested dir /a/b/");
    if (!f) return;
    f->close();
    delete f;
}

static void test_mkdir_duplicate_fails() {
    VFS::mkdir("/dupdir");
    int ret = VFS::mkdir("/dupdir");
    TEST_ASSERT(ret != 0, "mkdir duplicate directory fails");
}

// ─── unlink tests ───────────────────────────────────────────────────────────

static void test_unlink_existing_file() {
    // napravi pa obrisi
    File* f = VFS::open("/todelete.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create file before unlink");
    if (!f) return;
    f->close();
    delete f;

    int ret = VFS::unlink("/todelete.txt");
    TEST_ASSERT_EQ(ret, 0, "unlink existing file returns 0");
}

static void test_unlink_file_no_longer_accessible() {
    File* f = VFS::open("/gone.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create file to be unlinked");
    if (!f) return;
    f->close();
    delete f;

    VFS::unlink("/gone.txt");

    File* f2 = VFS::open("/gone.txt", File::O_RDONLY);
    TEST_ASSERT_NULL(f2, "open unlinked file returns null");
}

static void test_unlink_nonexistent_fails() {
    int ret = VFS::unlink("/doesnotexist.txt");
    TEST_ASSERT(ret != 0, "unlink nonexistent file returns non-zero");
}

static void test_unlink_then_recreate() {
    File* f = VFS::open("/recycle.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "create recycle.txt");
    if (!f) return;
    f->write("first", 5);
    f->close();
    delete f;

    VFS::unlink("/recycle.txt");

    f = VFS::open("/recycle.txt", File::O_CREAT | File::O_RDWR);
    TEST_ASSERT_NOT_NULL(f, "recreate recycle.txt after unlink");
    if (!f) return;
    f->write("second", 6);
    f->close();
    delete f;

    char buf[32] = {};
    int n = readFile("/recycle.txt", buf, 31);
    TEST_ASSERT(n > 0, "read recreated file returns positive count");
    TEST_ASSERT_STR(buf, "second", "recreated file has new content");
}

// ─── run ────────────────────────────────────────────────────────────────────

static void run() {
    Console::kprintf("\n========================================\n");
    Console::kprintf("  FILESYSTEM TEST SUITE\n");
    Console::kprintf("========================================\n");

    printSectionHeader("Read");
    test_open_existing_file();
    test_open_nonexistent_file();
    test_read_content();
    test_read_subdir_file();
    test_read_returns_zero_at_eof();
    test_seek_and_read();
    test_tell_after_read();
    test_seek_beyond_eof_fails();
    test_read_large_file();
    test_open_directory_as_file_fails();

    printSectionHeader("Write");
    test_write_existing_file();
    test_write_and_read_back();
    test_write_readonly_fails();
    test_create_new_file();
    test_create_write_read_back();
    test_create_persists_after_reopen();

    printSectionHeader("Mkdir");
    test_mkdir_creates_directory();
    test_mkdir_then_create_file_inside();
    test_mkdir_nested();
    test_mkdir_duplicate_fails();

    printSectionHeader("Unlink");
    test_unlink_existing_file();
    test_unlink_file_no_longer_accessible();
    test_unlink_nonexistent_fails();
    test_unlink_then_recreate();

    printSummary();
}

}
#endif