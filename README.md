# RISC-V Hobby OS

A Unix-like operating system kernel built from scratch in C++ and Assembly, targeting the RISC-V 64-bit architecture (QEMU `virt` machine). The goal is Linux-syscall-compatible, POSIX-style behavior — capable of running real, unmodified userspace software such as musl libc programs and a BusyBox shell.

This is a personal systems-programming project, not a university assignment.

## Highlights

- Boots on QEMU (`virt` machine) with a VirtIO block device and UART console
- Runs **musl libc** binaries and a full **BusyBox `ash`** shell
- Implements a growing subset of the **Linux syscall ABI**
- SV39 virtual memory with Copy-on-Write `fork()`
- A real filesystem (ext2) backed by a VirtIO block driver

## Architecture

### Memory Management
- SV39 three-level paging with higher-half kernel mapping
- Buddy allocator (page-granularity) + slab allocator (`kmalloc`/`KMemCache`) for kernel objects
- Copy-on-Write `fork()`, including COW-aware `mmap()` handling for shared/private mappings
- `mmap()` / `munmap()` / `mprotect()` with anonymous and file-backed, shared and private mappings
- Reference-counted physical pages (`PageRefCount`) for COW and shared mappings

### Process & Thread Model
- ELF64 loader with musl libc compatibility (program headers, AT_* auxv entries, argv/envp setup)
- POSIX process model: `fork`, `execve`, `wait4`/`waitpid`, process groups, sessions (`setpgid`, `setsid`)
- `clone()`-based threading (`CLONE_VM`, `CLONE_THREAD`, `CLONE_FILES`, `CLONE_SIGHAND`, TLS via `CLONE_SETTLS`)
- Futex-based synchronization primitives exposed to userspace (mutexes, semaphores, barriers)
- Round-robin scheduler with sleep/wake queues and per-thread time slicing
- POSIX signals: `sigaction`, `sigprocmask`, `sigpending`, `sigreturn`, process-group signal delivery, `SIGSTOP`/`SIGCONT` job control

### Filesystem
- Virtual File System (VFS) layer with inode and path caching (LRU-based)
- Full **ext2** driver: direct/indirect/double-indirect/triple-indirect block mapping, inode and block bitmaps
- Symlinks (fast symlinks via `i_block`), `readlink`, depth-limited path resolution
- File descriptor table shared/cloned across `fork()`/`clone()` per Linux semantics
- Pipes, `dup`/`dup2`/`dup3`, `fcntl`

### I/O & Drivers
- Interrupt-driven VirtIO block device driver with a block cache
- UART console driver with an interrupt-driven RX buffer
- TTY layer: termios (`TCGETS`/`TCSETS`), window size (`TIOCGWINSZ`), foreground process group (`TIOCGPGRP`/`TIOCSPGRP`), canonical/raw line discipline, job-control signal generation (`INTR`, `QUIT`, `SUSP`)

### Syscall Surface

A steadily growing Linux-compatible syscall table, including (non-exhaustive): `read`/`write`/`readv`/`writev`, `openat`/`close`, `mmap`/`munmap`/`mprotect`/`brk`, `clone`/`execve`/`wait4`/`exit`/`exit_group`, `fork` via `clone`, `futex`, `getdents64`, `newfstatat`/`statx`, `pread64`/`pwrite64`, `ftruncate`, `symlinkat`/`readlinkat`/`faccessat`, `renameat2`, `rt_sigaction`/`rt_sigprocmask`/`rt_sigreturn`, `kill`/`tkill`/`tgkill`, `setpgid`/`getpgid`/`setsid`, `ioctl`, `pipe2`, `prlimit64`, `getrandom`.

## Building & Running

### Prerequisites
- `riscv64-unknown-elf` GCC/binutils toolchain (kernel build)
- `qemu-system-riscv64`
- `mkfs.ext2`

### Userspace Toolchain Setup

Before the first build, set up the musl cross-compiler and BusyBox — **`install_musl.sh` must be run before `install_shell.sh`**, since the shell build depends on the musl toolchain it produces.

```bash
./scripts/install_musl.sh
```
Builds a `riscv64-linux-musl` GCC 12.4.0 / binutils 2.44 / musl 1.2.4 toolchain via `musl-cross-make` and installs it into `libc/` at the project root. This toolchain is used by `user/Makefile` to build musl-linked binaries (e.g. `hello_musl.elf`).

```bash
./scripts/install_shell.sh
```
Builds BusyBox against the toolchain installed above.

### Build the kernel and disk image
```bash
make
```
This builds `kernel.elf`, builds the userspace test binaries and BusyBox shell, and assembles an ext2 `disk.img` containing `/bin`, symlinked BusyBox applets, and test data files.

### Run in QEMU
```bash
make qemu
```

### Debug with GDB
```bash
make qemu-gdb   # in one terminal
make gdb-client # in another
```

## Testing

`user/init.c` is a large, sequentially numbered regression test suite (run as PID 1) exercising most of the syscall surface: memory management, filesystem correctness, fork/exec semantics, signals, futex/threading, and edge-case/bad-pointer handling. Each new userspace program ported to the kernel (musl hello world → musl test suite → BusyBox `ash`) has served as an additional real-world regression harness that has surfaced and driven fixes for numerous kernel bugs.

## Supported Shell Commands

The disk image symlinks the following BusyBox applets to `/bin/sh` (see `BUSYBOX_CMDS` in the top-level `Makefile`), in addition to the `ash` shell itself:

```
ls cat echo mkdir rm rmdir touch pwd true false cp mv stat chmod kill sleep
time grep head tail wc sort uniq tr cut cmp ln readlink tee xargs
```

## Acknowledgments / Third-Party Licenses

This project builds and runs third-party software as part of its userspace, each under its own license:

- **[musl libc](https://musl.libc.org/)** — MIT License. Used as the C library for userspace binaries (via `riscv64-linux-musl`, built by `scripts/install_musl.sh`). See the [musl source repository](https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT) for the full license text.
- **[BusyBox](https://busybox.net/)** — GNU General Public License v2.0. Provides the `ash` shell and the userland applets listed above. `scripts/install_shell.sh` clones and builds BusyBox from its upstream source ([mirror/busybox](https://github.com/mirror/busybox)) rather than vendoring a prebuilt binary, so source availability is preserved as required by the GPL. See the [BusyBox license](https://busybox.net/license.html) for details.

Neither musl nor BusyBox is linked into the kernel itself — they run as ordinary userspace programs communicating with the kernel through the syscall interface.

## Project Structure

```
kernel/       # kernel sources (mm, proc, fs, io, trap, hw)
user/         # userspace test suite, musl test program, syscall.h ABI shim
shell/        # BusyBox source/build output
scripts/      # toolchain setup scripts (musl cross-compiler, BusyBox build)
libc/         # riscv64-linux-musl cross toolchain, created by scripts/install_musl.sh
kernel.ld     # kernel linker script
Makefile      # top-level build (kernel + userspace + disk image)
```