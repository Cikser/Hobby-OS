#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef long               ssize_t;
typedef unsigned long      size_t;
typedef int                pid_t;
typedef unsigned long      uintptr_t;
typedef void*              mmap_ptr_t;

#define SYS_GETCWD       17
#define SYS_IOCTL        29
#define SYS_CHDIR        49
#define SYS_OPENAT       56
#define SYS_CLOSE        57
#define SYS_READ         63
#define SYS_WRITE        64
#define SYS_READV        65
#define SYS_WRITEV       66
#define SYS_FSTAT        80
#define SYS_MKDIR        83
#define SYS_EXIT         93
#define SYS_EXIT_GROUP   94
#define SYS_MMAP         222
#define SYS_MUNMAP       215
#define SYS_MPROTECT     226
#define SYS_GETPID       172
#define SYS_BRK          214
#define SYS_FORK         220
#define SYS_EXECVE       221
#define SYS_WAIT4        260

#define STDIN_FILENO   0
#define STDOUT_FILENO  1
#define STDERR_FILENO  2

#define O_RDONLY   0x0
#define O_WRONLY   0x1
#define O_RDWR     0x2
#define O_CREAT    0x40
#define O_TRUNC    0x200
#define O_APPEND   0x400

#define PROT_NONE    0x0
#define PROT_READ    0x1
#define PROT_WRITE   0x2
#define PROT_EXEC    0x4

#define MAP_SHARED     0x01
#define MAP_PRIVATE    0x02
#define MAP_FIXED      0x10
#define MAP_ANONYMOUS  0x20
#define MAP_ANON       MAP_ANONYMOUS

#define MAP_FAILED     ((void*)-1)

#define WIFEXITED(s)    (((s) & 0x7f) == 0)
#define WEXITSTATUS(s)  (((s) >> 8) & 0xff)

static inline long __syscall(long num,
                              long a0, long a1, long a2,
                              long a3, long a4, long a5)
{
    register long _num __asm__("a7") = num;
    register long _a0  __asm__("a0") = a0;
    register long _a1  __asm__("a1") = a1;
    register long _a2  __asm__("a2") = a2;
    register long _a3  __asm__("a3") = a3;
    register long _a4  __asm__("a4") = a4;
    register long _a5  __asm__("a5") = a5;
    __asm__ volatile("ecall"
        : "+r"(_a0)
        : "r"(_num), "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5)
        : "memory");
    return _a0;
}

#define _SC0(n)                 __syscall((n),0,0,0,0,0,0)
#define _SC1(n,a)               __syscall((n),(long)(a),0,0,0,0,0)
#define _SC2(n,a,b)             __syscall((n),(long)(a),(long)(b),0,0,0,0)
#define _SC3(n,a,b,c)           __syscall((n),(long)(a),(long)(b),(long)(c),0,0,0)
#define _SC4(n,a,b,c,d)         __syscall((n),(long)(a),(long)(b),(long)(c),(long)(d),0,0)
#define _SC5(n,a,b,c,d,e)       __syscall((n),(long)(a),(long)(b),(long)(c),(long)(d),(long)(e),0)
#define _SC6(n,a,b,c,d,e,f)     __syscall((n),(long)(a),(long)(b),(long)(c),(long)(d),(long)(e),(long)(f))

struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t _pad1;
    int64_t  st_size;
    int  st_blksize;
    int  _pad2;
    int64_t  st_blocks;

    int64_t  st_atime_sec;
    int64_t  st_atime_nsec;
    int64_t  st_mtime_sec;
    int64_t  st_mtime_nsec;
    int64_t  st_ctime_sec;
    int64_t  st_ctime_nsec;

    uint32_t _unused[2];
};

static inline int fstat(int fd, struct stat* st) {
    return (int)_SC2(SYS_FSTAT, fd, st);
}

static inline int getcwd(char* buf, size_t size) {
    return (int)_SC2(SYS_GETCWD, buf, size);
}

static inline int chdir(const char* path) {
    return (int)_SC1(SYS_CHDIR, path);
}

static inline int mkdir(const char* path, uint32_t mode) {
    return (int)_SC2(SYS_MKDIR, path, mode);
}

static inline void exit(int code) {
    _SC1(SYS_EXIT, code);
}

static inline void exit_group(int code) {
    _SC1(SYS_EXIT_GROUP, code);
}

static inline pid_t getpid(void) {
    return (pid_t)_SC0(SYS_GETPID);
}

static inline pid_t fork(void) {
    return (pid_t)_SC0(SYS_FORK);
}

static inline pid_t wait4(pid_t pid, int* status, int flags) {
    return (pid_t)_SC4(SYS_WAIT4, pid, status, flags, 0);
}

static inline pid_t waitpid(pid_t pid, int* status) {
    return wait4(pid, status, 0);
}

static inline int execve(const char* path, char* const argv[], char* const envp[]) {
    return (int)_SC3(SYS_EXECVE, path, argv, envp);
}

#define AT_FDCWD  -100

static inline int open(const char* path, int flags) {
    return (int)_SC4(SYS_OPENAT, AT_FDCWD, path, flags, 0);
}

static inline int openat(int dirfd, const char* path, int flags, int mode) {
    return (int)_SC4(SYS_OPENAT, dirfd, path, flags, mode);
}

static inline int close(int fd) {
    return (int)_SC1(SYS_CLOSE, fd);
}

static inline ssize_t read(int fd, void* buf, size_t len) {
    return (ssize_t)_SC3(SYS_READ, fd, buf, len);
}

static inline ssize_t write(int fd, const void* buf, size_t len) {
    return (ssize_t)_SC3(SYS_WRITE, fd, buf, len);
}

static inline void* mmap(void* addr, size_t length, int prot,
                          int flags, int fd, long offset)
{
    void* ret = (void*)_SC6(SYS_MMAP, addr, length, prot, flags, fd, offset);
    return ret;
}

static inline int munmap(void* addr, size_t length) {
    return (int)_SC2(SYS_MUNMAP, addr, length);
}

static inline int mprotect(void* addr, size_t length, int prot) {
    return (int)_SC3(SYS_MPROTECT, addr, length, prot);
}

static inline void* brk(void* addr) {
    return (void*)_SC1(SYS_BRK, addr);
}

struct malloc_header {
    size_t size;
    int is_free;
};

#define HEADER_SIZE sizeof(struct malloc_header)

static void* heap_start = (void*)0;
static void* current_break = (void*)0;

static inline void* malloc(size_t size) {
    if (size == 0) return (void*)0;

    size = (size + 7) & ~7;
    size_t total_size = size + HEADER_SIZE;

    if (heap_start == (void*)0) {
        heap_start = brk((void*)0);
        current_break = heap_start;
    }

    struct malloc_header* header = (struct malloc_header*)heap_start;
    while ((void*)header < current_break) {
        if (header->is_free && header->size >= total_size) {
            header->is_free = 0;
            return (void*)(header + 1);
        }
        header = (struct malloc_header*)((char*)header + header->size);
    }

    void* new_block = brk((char*)current_break + total_size);
    if (new_block == (void*)-1) return (void*)0;

    header = (struct malloc_header*)current_break;
    header->size = total_size;
    header->is_free = 0;

    current_break = (char*)current_break + total_size;

    return (void*)(header + 1);
}

static inline void free(void* ptr) {
    if (!ptr) return;
    struct malloc_header* header = (struct malloc_header*)ptr - 1;
    header->is_free = 1;
}

static inline void free_pages(void* ptr, size_t size) {
    munmap(ptr, size);
}

static inline size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static inline int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static inline int strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (a[i] == '\0')  return 0;
    }
    return 0;
}

static inline char* strcpy(char* dst, const char* src) {
    char* p = dst;
    while ((*dst++ = *src++));
    return p;
}

static inline char* strncpy(char* dst, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

static inline void* memset(void* dst, int val, size_t len) {
    unsigned char* p = (unsigned char*)dst;
    while (len--) *p++ = (unsigned char)val;
    return dst;
}

static inline void* memcpy(void* dst, const void* src, size_t len) {
    unsigned char*       d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (len--) *d++ = *s++;
    return dst;
}

static inline int memcmp(const void* a, const void* b, size_t len) {
    const unsigned char* p = (const unsigned char*)a;
    const unsigned char* q = (const unsigned char*)b;
    for (size_t i = 0; i < len; i++) {
        if (p[i] != q[i]) return p[i] < q[i] ? -1 : 1;
    }
    return 0;
}

static inline void _puts(const char* s) {
    write(STDOUT_FILENO, s, strlen(s));
}

static inline void _putc(char c) {
    write(STDOUT_FILENO, &c, 1);
}

static inline void _putulong(unsigned long v, int base) {
    const char* digits = "0123456789abcdef";
    char buf[32];
    int i = 0;
    if (v == 0) { _putc('0'); return; }
    while (v) { buf[i++] = digits[v % base]; v /= base; }
    while (--i >= 0) _putc(buf[i]);
}

static inline void printf(const char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    for (const char* p = fmt; *p; p++) {
        if (*p != '%') { _putc(*p); continue; }
        p++;
        switch (*p) {
            case 'd': {
                long v = __builtin_va_arg(args, long);
                if (v < 0) { _putc('-'); v = -v; }
                _putulong((unsigned long)v, 10);
                break;
            }
            case 'u': _putulong(__builtin_va_arg(args, unsigned long), 10); break;
            case 'x': _putulong(__builtin_va_arg(args, unsigned long), 16); break;
            case 'p':
                _puts("0x");
                _putulong((unsigned long)__builtin_va_arg(args, void*), 16);
                break;
            case 's': _puts(__builtin_va_arg(args, const char*)); break;
            case 'c': _putc((char)__builtin_va_arg(args, int));   break;
            case '%': _putc('%'); break;
            default:  _putc('%'); _putc(*p); break;
        }
    }
    __builtin_va_end(args);
}

#endif