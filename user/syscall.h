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

#define SYS_GETCWD          17
#define SYS_IOCTL           29
#define SYS_UNLINKAT        35
#define SYS_CHDIR           49
#define SYS_OPENAT          56
#define SYS_CLOSE           57
#define SYS_PIPE2           59
#define SYS_LSEEK           62
#define SYS_READ            63
#define SYS_WRITE           64
#define SYS_READV           65
#define SYS_WRITEV          66
#define SYS_FSTAT           80
#define SYS_MKDIR           83
#define SYS_EXIT            93
#define SYS_EXIT_GROUP      94
#define SYS_SET_TID_ADDRESS 96
#define SYS_FUTEX           98
#define SYS_CLOCK_GETTIME   113
#define SYS_SCHED_YIELD     124
#define SYS_UNAME           160
#define SYS_GETPID          172
#define SYS_GETUID          174
#define SYS_GETEUID         175
#define SYS_GETGID          176
#define SYS_GETEGID         177
#define SYS_GETTID          178
#define SYS_BRK             214
#define SYS_MUNMAP          215
#define SYS_CLONE           220
#define SYS_EXECVE          221
#define SYS_MMAP            222
#define SYS_MPROTECT        226
#define SYS_WAIT4           260
#define SYS_DUP             23
#define SYS_DUP3            24
#define SYS_FCNTL           25
#define SYS_GETDENTS64      61
#define SYS_PREAD64         67
#define SYS_PWRITE64        68
#define SYS_READLINKAT      78
#define SYS_NEWFSTATAT      79
#define SYS_FTRUNCATE       46
#define SYS_NANOSLEEP       101
#define SYS_CLOCK_GETRES    114
#define SYS_GETPPID         173
#define SYS_GETTIMEOFDAY    169
#define SYS_GETRUSAGE       165
#define SYS_UMASK           166
#define SYS_KILL            129
#define SYS_TKILL           130
#define SYS_TGKILL          131
#define SYS_SIGALTSTACK     132
#define SYS_RT_SIGACTION    134
#define SYS_RT_SIGPROCMASK  135
#define SYS_RT_SIGPENDING   136
#define SYS_RT_SIGRETURN    139
#define SYS_MADVISE         233
#define SYS_PRLIMIT64       261
#define SYS_GETRANDOM       278
#define SYS_MEMBARRIER      283
#define SYS_STATX           291
#define SYS_RSEQ            293

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define O_RDONLY  0x0
#define O_WRONLY  0x1
#define O_RDWR    0x2
#define O_CREAT   0x40
#define O_TRUNC   0x200
#define O_APPEND  0x400

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_ANON      MAP_ANONYMOUS

#define MAP_FAILED ((void*)-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

#define AT_FDCWD -100
#define AT_REMOVEDIR 0x200
#define AT_EMPTY_PATH 0x1000

#define DT_UNKNOWN 0
#define DT_REG     8
#define DT_DIR     4

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#define FUTEX_OK 0
#define FUTEX_EAGAIN -11
#define FUTEX_EINVAL -22
#define FUTEX_ETIMEDOUT -110

#define CLONE_VM               0x00000100
#define CLONE_FS               0x00000200
#define CLONE_FILES            0x00000400
#define CLONE_SIGHAND          0x00000800
#define CLONE_THREAD           0x00010000
#define CLONE_SETTLS           0x00080000
#define CLONE_PARENT_SETTID    0x00100000
#define CLONE_CHILD_CLEARTID   0x00200000
#define CLONE_CHILD_SETTID     0x01000000

#define WIFEXITED(s)   (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)

#define RLIMIT_STACK  3
#define RLIMIT_NOFILE 7
#define RLIMIT_AS     9
#define RLIM_INFINITY (~0ULL)

#define MADV_NORMAL     0
#define MADV_DONTNEED   4
#define MADV_FREE       8

#define GRND_NONBLOCK 0x1
#define GRND_RANDOM   0x2

#define STATX_TYPE    0x00000001U
#define STATX_MODE    0x00000002U
#define STATX_NLINK   0x00000004U
#define STATX_UID     0x00000008U
#define STATX_GID     0x00000010U
#define STATX_SIZE    0x00000200U
#define STATX_BLOCKS  0x00000400U
#define STATX_ALL     0x00000FFFU

#define TIOCGWINSZ 0x5413
#define TCGETS 0x5401
#define FIONREAD 0x541B

#define NULL ((void*)0)

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

#define _SC0(n)             __syscall((n),0,0,0,0,0,0)
#define _SC1(n,a)           __syscall((n),(long)(a),0,0,0,0,0)
#define _SC2(n,a,b)         __syscall((n),(long)(a),(long)(b),0,0,0,0)
#define _SC3(n,a,b,c)       __syscall((n),(long)(a),(long)(b),(long)(c),0,0,0)
#define _SC4(n,a,b,c,d)     __syscall((n),(long)(a),(long)(b),(long)(c),(long)(d),0,0)
#define _SC5(n,a,b,c,d,e)   __syscall((n),(long)(a),(long)(b),(long)(c),(long)(d),(long)(e),0)
#define _SC6(n,a,b,c,d,e,f) __syscall((n),(long)(a),(long)(b),(long)(c),(long)(d),(long)(e),(long)(f))

static void printf(const char* fmt, ...);

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
    int32_t  st_blksize;
    int32_t  _pad2;
    int64_t  st_blocks;
    int64_t  st_atime_sec;
    int64_t  st_atime_nsec;
    int64_t  st_mtime_sec;
    int64_t  st_mtime_nsec;
    int64_t  st_ctime_sec;
    int64_t  st_ctime_nsec;
    uint32_t _unused[2];
};

struct timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

struct iovec {
    void*  iov_base;
    size_t iov_len;
};

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

struct timeval {
    int64_t tv_sec;
    int64_t tv_usec;
};

struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
    int64_t ru_maxrss;
    int64_t ru_ixrss;
    int64_t ru_idrss;
    int64_t ru_isrss;
    int64_t ru_minflt;
    int64_t ru_majflt;
    int64_t ru_nswap;
    int64_t ru_inblock;
    int64_t ru_oublock;
    int64_t ru_msgsnd;
    int64_t ru_msgrcv;
    int64_t ru_nsignals;
    int64_t ru_nvcsw;
    int64_t ru_nivcsw;
};

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    uint16_t d_reclen;
    uint8_t  d_type;
    char     d_name[256];
};

static inline void exit(int code) {
    _SC1(SYS_EXIT, code);
}

static inline void exit_group(int code) {
    _SC1(SYS_EXIT_GROUP, code);
}

static inline pid_t getpid(void) {
    return (pid_t)_SC0(SYS_GETPID);
}

static inline uint32_t getuid(void) {
    return (uint32_t)_SC0(SYS_GETUID);
}

static inline uint32_t geteuid(void) {
    return (uint32_t)_SC0(SYS_GETEUID);
}

static inline uint32_t getgid(void) {
    return (uint32_t)_SC0(SYS_GETGID);
}

static inline uint32_t getegid(void) {
    return (uint32_t)_SC0(SYS_GETEGID);
}

static inline pid_t fork(void) {
    return (pid_t)_SC4(SYS_CLONE, 0, 0, 0, 0);
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

static inline int open(const char* path, int flags) {
    return (int)_SC4(SYS_OPENAT, AT_FDCWD, path, flags, 0);
}

static inline int openat(int dirfd, const char* path, int flags, int mode) {
    return (int)_SC4(SYS_OPENAT, dirfd, path, flags, mode);
}

static inline int unlink(const char* path) {
    return (int)_SC3(SYS_UNLINKAT, AT_FDCWD, path, 0);
}

static inline int rmdir(const char* path) {
    return (int)_SC3(SYS_UNLINKAT, AT_FDCWD, path, AT_REMOVEDIR);
}

static inline int creat(const char* path) {
    return open(path, O_WRONLY | O_CREAT | O_TRUNC);
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

static inline ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
    return (ssize_t)_SC3(SYS_READV, fd, iov, iovcnt);
}

static inline ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    return (ssize_t)_SC3(SYS_WRITEV, fd, iov, iovcnt);
}

static inline int64_t lseek(int fd, int64_t offset, int whence) {
    return (int64_t)_SC3(SYS_LSEEK, fd, offset, whence);
}

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

static inline void* brk(void* addr) {
    return (void*)_SC1(SYS_BRK, addr);
}

static inline void* mmap(void* addr, size_t length, int prot,
                          int flags, int fd, long offset) {
    return (void*)_SC6(SYS_MMAP, addr, length, prot, flags, fd, offset);
}

static inline int munmap(void* addr, size_t length) {
    return (int)_SC2(SYS_MUNMAP, addr, length);
}

static inline int mprotect(void* addr, size_t length, int prot) {
    return (int)_SC3(SYS_MPROTECT, addr, length, prot);
}

static inline int clock_gettime(int clockid, struct timespec* ts) {
    return (int)_SC2(SYS_CLOCK_GETTIME, clockid, ts);
}

static inline int uname(struct utsname* buf) {
    return (int)_SC1(SYS_UNAME, buf);
}

static inline pid_t set_tid_address(int* tidptr) {
    return (pid_t)_SC1(SYS_SET_TID_ADDRESS, tidptr);
}

static inline long futex(uint32_t* uaddr, int op, uint32_t val,
                            unsigned long timeout_ticks) {
      return __syscall(SYS_FUTEX, (long)uaddr, (long)op,
                       (long)val, (long)timeout_ticks, 0, 0);
}

static inline pid_t gettid() {
    return _SC0(SYS_GETTID);
}

static inline int sched_yield() {
    return _SC0(SYS_SCHED_YIELD);
}

static inline int nanosleep(const struct timespec* req, struct timespec* rem) {
    return (int)_SC2(SYS_NANOSLEEP, req, rem);
}

static inline int clock_getres(int clockid, struct timespec* res) {
    return (int)_SC2(SYS_CLOCK_GETRES, clockid, res);
}

static inline pid_t getppid(void) {
    return (pid_t)_SC0(SYS_GETPPID);
}

static inline int gettimeofday(struct timeval* tv, void* tz) {
    return (int)_SC2(SYS_GETTIMEOFDAY, tv, tz);
}

static inline int getrusage(int who, struct rusage* usage) {
    return (int)_SC2(SYS_GETRUSAGE, who, usage);
}

static inline uint32_t umask(uint32_t mask) {
    return (uint32_t)_SC1(SYS_UMASK, mask);
}

static inline int fcntl(int fd, int cmd, int arg) {
    return (int)_SC3(SYS_FCNTL, fd, cmd, arg);
}

static inline int dup(int fd) {
    return (int)_SC1(SYS_DUP, fd);
}

static inline int dup2(int oldfd, int newfd) {
    return (int)_SC3(SYS_DUP3, oldfd, newfd, 0);
}

static inline int dup3(int oldfd, int newfd, int flags) {
    return (int)_SC3(SYS_DUP3, oldfd, newfd, flags);
}

static inline int pipe2(int fds[2], int flags) {
    return (int)_SC2(SYS_PIPE2, fds, flags);
}

static inline int pipe(int fds[2]) {
    return pipe2(fds, 0);
}

static inline int ftruncate(int fd, int64_t length) {
    return (int)_SC2(SYS_FTRUNCATE, fd, length);
}

static inline ssize_t pread(int fd, void* buf, size_t count, int64_t offset) {
    return (ssize_t)_SC4(SYS_PREAD64, fd, buf, count, offset);
}

static inline ssize_t pwrite(int fd, const void* buf, size_t count, int64_t offset) {
    return (ssize_t)_SC4(SYS_PWRITE64, fd, buf, count, offset);
}

static inline ssize_t readlinkat(int dirfd, const char* path, char* buf, size_t bufsiz) {
    return (ssize_t)_SC4(SYS_READLINKAT, dirfd, path, buf, bufsiz);
}

static inline ssize_t readlink(const char* path, char* buf, size_t bufsiz) {
    return readlinkat(AT_FDCWD, path, buf, bufsiz);
}

static inline int newfstatat(int dirfd, const char* path, struct stat* st, int flags) {
    return (int)_SC4(SYS_NEWFSTATAT, dirfd, path, st, flags);
}

static inline int stat(const char* path, struct stat* st) {
    return newfstatat(AT_FDCWD, path, st, 0);
}

static inline int lstat(const char* path, struct stat* st) {
    return newfstatat(AT_FDCWD, path, st, 0);
}

static inline ssize_t getdents64(int fd, struct linux_dirent64* buf, size_t count) {
    return (ssize_t)_SC3(SYS_GETDENTS64, fd, buf, count);
}

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1  10
#define SIGSEGV  11
#define SIGUSR2  12
#define SIGPIPE  13
#define SIGALRM  14
#define SIGTERM  15
#define SIGCHLD  17
#define SIGCONT  18
#define SIGSTOP  19
#define NSIG     32

#define SIG_DFL  ((sighandler_t)0)
#define SIG_IGN  ((sighandler_t)1)

#define SA_RESTORER  (1UL << 26)
#define SA_RESTART   (1UL << 24)
#define SA_NOCLDSTOP (1UL << 0)

typedef void (*sighandler_t)(int);
typedef uint64_t sigset_t;

struct sigaction_t {
    uint64_t  sa_handler;
    uint64_t  sa_flags;
    uint64_t  sa_restorer;
    sigset_t  sa_mask;
};

__attribute__((naked, noinline, section(".text.sigreturn")))
static void __sigreturn_trampoline(void) {
    __asm__ volatile (
        "li a7, 139\n"
        "ecall\n"
    );
}


static inline int kill(pid_t pid, int sig) {
    return (int)_SC2(SYS_KILL, pid, sig);
}

static inline int raise(int sig) {
    return kill(getpid(), sig);
}

static inline int sigaction(int sig, const struct sigaction_t* act,
                             struct sigaction_t* oldact) {
    return (int)_SC4(SYS_RT_SIGACTION, sig, act, oldact, 8 );
}

static inline int signal(int sig, sighandler_t handler) {
    struct sigaction_t act = {
        .sa_handler  = (uint64_t)handler,
        .sa_flags    = SA_RESTORER,
        .sa_restorer = (uint64_t)__sigreturn_trampoline,
        .sa_mask     = 0,
    };
    return sigaction(sig, &act, (struct sigaction_t*)0);
}

static inline int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    return (int)_SC4(SYS_RT_SIGPROCMASK, how, set, oldset, 8);
}

static inline int sigpending(sigset_t* set) {
    return (int)_SC2(SYS_RT_SIGPENDING, set, 8);
}

static inline void sigemptyset(sigset_t* s) { *s = 0; }
static inline void sigfillset(sigset_t* s)  { *s = ~(sigset_t)0; }
static inline void sigaddset(sigset_t* s, int sig) {
    if (sig >= 1 && sig < NSIG) *s |= (1ULL << (sig - 1));
}
static inline void sigdelset(sigset_t* s, int sig) {
    if (sig >= 1 && sig < NSIG) *s &= ~(1ULL << (sig - 1));
}
static inline int sigismember(const sigset_t* s, int sig) {
    return (sig >= 1 && sig < NSIG) ? ((*s >> (sig - 1)) & 1) : 0;
}

#define F_DUPFD    0
#define F_GETFD    1
#define F_SETFD    2
#define F_GETFL    3
#define F_SETFL    4
#define FD_CLOEXEC 1

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)

static inline unsigned int sleep(unsigned int seconds) {
    struct timespec req = { (int64_t)seconds, 0 };
    struct timespec rem = { 0, 0 };
    nanosleep(&req, &rem);
    return (unsigned int)rem.tv_sec;
}

static inline int usleep(unsigned int usecs) {
    struct timespec req = { 0, (int64_t)usecs * 1000 };
    return nanosleep(&req, (struct timespec*)0);
}

#define STACK_SIZE (4096 * 4)

__attribute__((noinline))
static long __clone_impl(unsigned long flags,
                          void*         child_sp,
                          int*          ptid,
                          unsigned long tls,
                          int*          ctid)
{
    register long a0 __asm__("a0") = (long)flags;
    register long a1 __asm__("a1") = (long)child_sp;
    register long a2 __asm__("a2") = (long)ptid;
    register long a3 __asm__("a3") = (long)tls;
    register long a4 __asm__("a4") = (long)ctid;
    register long a7 __asm__("a7") = 220;

    __asm__ volatile (
        "ecall\n"

        "bnez  a0, 1f\n"

        "ld    t0,  0(sp)\n"
        "ld    a0,  8(sp)\n"
        "addi  sp, sp, 16\n"

        "jalr  t0\n"

        "ld    t0,  0(sp)\n"
        "sw    zero, 0(t0)\n"
        "mv    a0,  t0\n"
        "li    a1,  1\n"
        "li    a2,  0x7fffffff\n"
        "li    a3,  0\n"
        "li    a4,  0\n"
        "li    a5,  0\n"
        "li    a7,  98\n"
        "ecall\n"

        "li    a0,  0\n"
        "li    a7,  93\n"
        "ecall\n"
        "j     .\n"

        "1:\n"

        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7)
        : "t0", "a5", "memory"
    );

    return a0;
}


typedef struct {
    int       tid;
    uint32_t* join_word;
    void*     stack_base;
} thread_t;

typedef void(*thread_func_t)(void*);

static inline int thread_create(thread_t* t, thread_func_t func, void* arg) {
    void* stack = mmap((void*)0, STACK_SIZE,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) return -1;

    uint32_t* jw = (uint32_t*)mmap((void*)0, 4096,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (jw == MAP_FAILED) { munmap(stack, STACK_SIZE); return -1; }
    *jw = 1;

    t->stack_base = stack;
    t->join_word  = jw;

    uint64_t top   = ((uint64_t)stack + STACK_SIZE) & ~0xFULL;
    uint64_t* slot = (uint64_t*)(top - 32);
    slot[0] = (uint64_t)func;
    slot[1] = (uint64_t)arg;
    slot[2] = (uint64_t)jw;
    slot[3] = 0;
    void* child_sp = (void*)(top - 32);

    int parent_tid = 0;
    int child_tid  = 0;

    unsigned long flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
                        | CLONE_THREAD | CLONE_SETTLS
                        | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID
                        | CLONE_CHILD_SETTID;

    long ret = __clone_impl(flags, child_sp, &parent_tid, 0, &child_tid);
    if (ret < 0) {
        munmap(stack, STACK_SIZE);
        munmap(jw, 4096);
        return -1;
    }

    t->tid = (int)ret;
    return 0;
}

static inline int thread_join(thread_t* t) {
    uint32_t v;
    while ((v = *t->join_word) != 0) {
        futex(t->join_word, FUTEX_WAIT, v, 0);
    }
    munmap(t->stack_base, STACK_SIZE);
    munmap(t->join_word, 4096);
    return 0;
}

typedef struct { uint32_t state; } mutex_t;
#define MUTEX_INIT { 0 }

static inline void mutex_lock(mutex_t* m) {
    uint32_t c = __sync_val_compare_and_swap(&m->state, 0, 1);
    if (c == 0) return;

    do {
        if (c == 2 || __sync_val_compare_and_swap(&m->state, 1, 2) != 0)
            futex(&m->state, FUTEX_WAIT, 2, 0);
        c = __sync_val_compare_and_swap(&m->state, 0, 2);
    } while (c != 0);
}

static inline void mutex_unlock(mutex_t* m) {
    uint32_t old = __sync_fetch_and_sub(&m->state, 1);
    if (old != 1) {
        __sync_lock_release(&m->state);
        futex(&m->state, FUTEX_WAKE, 1, 0);
    }
}

typedef struct { uint32_t count; } sem_t;

static inline void sem_init(sem_t* s, uint32_t val) { s->count = val; }

static inline void sem_wait(sem_t* s) {
    while (1) {
        uint32_t c = s->count;
        if (c > 0 && __sync_bool_compare_and_swap(&s->count, c, c-1))
            return;
        futex(&s->count, FUTEX_WAIT, c, 0);
    }
}

static inline void sem_post(sem_t* s) {
    __sync_fetch_and_add(&s->count, 1);
    futex(&s->count, FUTEX_WAKE, 1, 0);
}


struct statx_timestamp {
    int64_t  tv_sec;
    uint32_t tv_nsec;
    uint32_t _pad;
};

struct statx {
    uint32_t stx_mask;
    uint32_t stx_blksize;
    uint64_t stx_attributes;
    uint32_t stx_nlink;
    uint32_t stx_uid;
    uint32_t stx_gid;
    uint16_t stx_mode;
    uint16_t _pad1[3];
    uint64_t stx_ino;
    uint64_t stx_size;
    uint64_t stx_blocks;
    uint64_t stx_attributes_mask;
    struct statx_timestamp stx_atime;
    struct statx_timestamp stx_btime;
    struct statx_timestamp stx_ctime;
    struct statx_timestamp stx_mtime;
    uint32_t stx_rdev_major;
    uint32_t stx_rdev_minor;
    uint32_t stx_dev_major;
    uint32_t stx_dev_minor;
    uint64_t _spare[14];
};

struct rlimit {
    uint64_t rlim_cur;
    uint64_t rlim_max;
};

static inline int madvise(void* addr, size_t len, int advice) {
    return (int)_SC3(SYS_MADVISE, addr, len, advice);
}

static inline int prlimit(pid_t pid, int resource,
                           const struct rlimit* new_lim,
                           struct rlimit* old_lim) {
    return (int)_SC4(SYS_PRLIMIT64, pid, resource, new_lim, old_lim);
}

static inline ssize_t getrandom(void* buf, size_t len, unsigned int flags) {
    return (ssize_t)_SC3(SYS_GETRANDOM, buf, len, flags);
}

static inline int statx(int dirfd, const char* path, int flags,
                         unsigned int mask, struct statx* buf) {
    return (int)_SC5(SYS_STATX, dirfd, path, flags, mask, buf);
}

static inline int membarrier(int cmd, int flags, int cpu_id) {
    return (int)_SC3(SYS_MEMBARRIER, cmd, flags, cpu_id);
}

static inline int ioctl(int fd, unsigned long req, unsigned long arg) {
    return (int)_SC3(SYS_IOCTL, fd, req, arg);
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

static inline char* strcat(char* dst, const char* src) {
    char* p = dst + strlen(dst);
    while ((*p++ = *src++));
    return dst;
}

static inline char* strncat(char* dst, const char* src, size_t n) {
    char* p = dst + strlen(dst);
    size_t i = 0;
    for (; i < n && src[i]; i++) p[i] = src[i];
    p[i] = '\0';
    return dst;
}

static inline const char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return s;
        s++;
    }
    return (c == 0) ? s : (const char*)0;
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

static inline void _putc(char c) {
    write(STDOUT_FILENO, &c, 1);
}

static inline void _puts(const char* s) {
    write(STDOUT_FILENO, s, strlen(s));
}

static inline void _putulong(unsigned long v, int base) {
    const char* digits = "0123456789abcdef";
    char buf[64];
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
        int is_long = 0;
        if (*p == 'l') { is_long = 1; p++; }
        switch (*p) {
            case 'd': {
                long v = is_long ? __builtin_va_arg(args, long)
                                 : (long)__builtin_va_arg(args, int);
                if (v < 0) { _putc('-'); v = -v; }
                _putulong((unsigned long)v, 10);
                break;
            }
            case 'u': {
                unsigned long v = is_long ? __builtin_va_arg(args, unsigned long)
                                          : (unsigned long)__builtin_va_arg(args, unsigned int);
                _putulong(v, 10);
                break;
            }
            case 'x': {
                unsigned long v = is_long ? __builtin_va_arg(args, unsigned long)
                                          : (unsigned long)__builtin_va_arg(args, unsigned int);
                _putulong(v, 16);
                break;
            }
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

struct malloc_header {
    size_t size;
    int    is_free;
};

#define MALLOC_HEADER_SIZE sizeof(struct malloc_header)

static void* _heap_start    = (void*)0;
static void* _current_break = (void*)0;

static inline void* malloc(size_t size) {
    if (size == 0) return (void*)0;
    size = (size + 7) & ~(size_t)7;
    size_t total = size + MALLOC_HEADER_SIZE;
    if (_heap_start == (void*)0) {
        _heap_start    = brk((void*)0);
        _current_break = _heap_start;
    }
    struct malloc_header* h = (struct malloc_header*)_heap_start;
    while ((void*)h < _current_break) {
        if (h->is_free && h->size >= total) {
            h->is_free = 0;
            return (void*)(h + 1);
        }
        h = (struct malloc_header*)((char*)h + h->size);
    }
    void* nb = brk((char*)_current_break + total);
    if (nb == (void*)-1) return (void*)0;
    h           = (struct malloc_header*)_current_break;
    h->size     = total;
    h->is_free  = 0;
    _current_break = (char*)_current_break + total;
    return (void*)(h + 1);
}

static inline void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void*  p     = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

static inline void* realloc(void* ptr, size_t size) {
    if (!ptr)   return malloc(size);
    if (!size)  { /* free(ptr); */ return (void*)0; }
    struct malloc_header* h = (struct malloc_header*)ptr - 1;
    size_t old_data = h->size - MALLOC_HEADER_SIZE;
    if (old_data >= size) return ptr;
    void* np = malloc(size);
    if (!np) return (void*)0;
    memcpy(np, ptr, old_data);
    h->is_free = 1;
    return np;
}

static inline void free(void* ptr) {
    if (!ptr) return;
    struct malloc_header* h = (struct malloc_header*)ptr - 1;
    h->is_free = 1;
}

static inline int abs(int v) { return v < 0 ? -v : v; }

static inline long labs(long v) { return v < 0 ? -v : v; }

static inline int itoa(long v, char* buf, int base) {
    const char* digits = "0123456789abcdef";
    char tmp[64];
    int  i = 0;
    int  neg = 0;
    if (v < 0 && base == 10) { neg = 1; v = -v; }
    unsigned long u = (unsigned long)v;
    if (u == 0) { tmp[i++] = '0'; }
    while (u > 0) { tmp[i++] = digits[u % base]; u /= base; }
    if (neg) tmp[i++] = '-';
    int len = i;
    for (int j = 0; j < len; j++) buf[j] = tmp[len - 1 - j];
    buf[len] = '\0';
    return len;
}

static inline long atol(const char* s) {
    long v = 0;
    int neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return neg ? -v : v;
}

static inline int atoi(const char* s) { return (int)atol(s); }

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

static inline int ioctl_winsz(int fd, struct winsize* ws) {
    return ioctl(fd, TIOCGWINSZ, (unsigned long)ws);
}

#endif