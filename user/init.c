#include "syscall.h"

static void demo_fs_ops(void) {
    printf("=== FS operations: mkdir, chdir, getcwd ===\n");

    char buf[128];

    if (getcwd(buf, sizeof(buf)) == 0) {
        printf("Current dir: %s\n", buf);
    } else {
        printf("getcwd failed\n");
    }

    if (mkdir("/test_dir", 0) == 0) {
        printf("mkdir /test_dir: OK\n");
    } else {
        printf("mkdir /test_dir: FAIL (možda već postoji?)\n");
    }

    if (chdir("/subdir") == 0) {
        printf("chdir /subdir: OK\n");
        if (getcwd(buf, sizeof(buf)) == 0) {
            printf("Current dir now: %s\n", buf);
        }
    } else {
        printf("chdir /subdir: FAIL\n");
    }

    int fd = open("../readme.txt", O_RDONLY);
    if (fd < 0) printf("Could not open readme.txt\n");
    else {
        read(fd, &buf, sizeof(buf));
        printf("readme.txt: %s\n", buf);
        close(fd);
    }

    if (chdir("..") == 0) {
        printf("chdir ..: OK\n");
        getcwd(buf, sizeof(buf));
        printf("Back to: %s\n", buf);
    }

    if (chdir("/test_dir") == 0) {
        printf("chdir /test_dir: OK\n");
        getcwd(buf, sizeof(buf));
        printf("Final path: %s\n", buf);
    } else {
        printf("chdir /test_dir: FAIL\n");
    }
}

void print_mode(uint32_t mode) {
    printf("File Type and Permissions: ");
    if ((mode & 0xF000) == 0x4000) printf("Directory ");
    else if ((mode & 0xF000) == 0x8000) printf("Regular File ");
    else printf("Unknown Type ");

    printf("(Octal: %x)\n", mode & 0xFFF);
}

void run_fstat_test(const char* filename) {
    struct stat st;
    int fd = open(filename, O_RDONLY);

    printf("--- FSTAT TEST: %s ---\n", filename);

    if (fd < 0) {
        printf("Error: Could not open file '%s'\n", filename);
        return;
    }

    if (fstat(fd, &st) != 0) {
        printf("Error: fstat failed for descriptor %d\n", fd);
        close(fd);
        return;
    }

    // Ispisivanje svih polja Inode stat strukture
    printf("Device ID:      %u\n", (uint32_t)st.st_dev);
    printf("Inode Number:   %u\n", (uint32_t)st.st_ino);
    print_mode(st.st_mode);
    printf("Links count:    %u\n", st.st_nlink);
    printf("Owner UID:      %u\n", st.st_uid);
    printf("Owner GID:      %u\n", st.st_gid);
    printf("File Size:      %d bytes\n", (long)st.st_size);
    printf("Block Size:     %d bytes\n", st.st_blksize);
    printf("Blocks (512B):  %d\n", (long)st.st_blocks);

    printf("\nTimestamps (Unix Epoch):\n");
    printf("Access Time:    %d\n", (long)st.st_atime_sec);
    printf("Modify Time:    %d\n", (long)st.st_mtime_sec);
    printf("Change Time:    %d\n", (long)st.st_ctime_sec);

    printf("---------------------------\n\n");

    close(fd);
}


void _start(void) {
    printf("Hello from userspace! pid=%ld\n", getpid());

    pid_t child = fork();

    if (child == 0) {
        demo_fs_ops();
        run_fstat_test("/");
        run_fstat_test("/readme.txt");
        run_fstat_test("/subdir/nested.txt");
        printf("child done, exiting\n");
        exit(0);
    } else {
        int status = 0;
        waitpid(child, &status);
        printf("parent: child exited, status=%d\n", WEXITSTATUS(status));
    }

    exit(0);
}