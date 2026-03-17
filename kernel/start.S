.equ STACK_SIZE, 8192

.section .text.init
.global _start
.global vm_bootstrap
.global boot_stack_top

_start:
    csrr t0, mhartid
    bnez t0, .halt
    csrw sie, zero

    la   sp, boot_stack_top

    call vm_bootstrap

    csrr t0, mstatus
    li   t1, ~(3 << 11)
    and  t0, t0, t1
    li   t1, (1 << 11)
    or   t0, t0, t1
    li   t1, (1 << 5)
    not  t1, t1
    and  t0, t0, t1
    li   t1, (1 << 1)
    not  t1, t1
    and  t0, t0, t1
    csrw mstatus, t0

    la   t0, s_mode_entry
    csrw mepc, t0

    li   t0, 0xffff
    csrw medeleg, t0
    csrw mideleg, t0

    csrw sie, zero

    li   t0, 0x3fffffffffffff
    csrw pmpaddr0, t0
    li   t0, 0xf
    csrw pmpcfg0, t0

    mret

.halt:
    wfi
    j .halt

.section .bss.init
.align 12
boot_stack:
    .skip STACK_SIZE
boot_stack_top: