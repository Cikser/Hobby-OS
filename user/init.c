void _start() {
    __asm__ volatile("li a7, 1");
    __asm__ volatile("ecall");
    while (1) {}
}