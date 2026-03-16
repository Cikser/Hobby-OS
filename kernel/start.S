.equ STACK_SIZE, 8192

.section .text.init
.global _start

_start:
    csrr t0, mhartid
    bnez t0, .halt
    csrw sie, zero

    la   sp, boot_stack_top
    call start

.halt:
    wfi
    j .halt

.section .bss
.align 12
boot_stack:
    .skip STACK_SIZE
boot_stack_top: