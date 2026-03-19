.section .text
.global _trap_kernel_entry
.global _trap_user_entry
.global _ZN11TrapHandler4trapEv

.align 2
_trap_kernel_entry:
_ZN11TrapHandler4trapEv:
    sd x2,  -240(sp)
    addi sp, sp, -256

    sd x1,   8(sp)
    sd x3,   24(sp)
    sd x4,   32(sp)
    sd x5,   40(sp)
    sd x6,   48(sp)
    sd x7,   56(sp)
    sd x8,   64(sp)
    sd x9,   72(sp)
    sd x10,  80(sp)
    sd x11,  88(sp)
    sd x12,  96(sp)
    sd x13,  104(sp)
    sd x14,  112(sp)
    sd x15,  120(sp)
    sd x16,  128(sp)
    sd x17,  136(sp)
    sd x18,  144(sp)
    sd x19,  152(sp)
    sd x20,  160(sp)
    sd x21,  168(sp)
    sd x22,  176(sp)
    sd x23,  184(sp)
    sd x24,  192(sp)
    sd x25,  200(sp)
    sd x26,  208(sp)
    sd x27,  216(sp)
    sd x28,  224(sp)
    sd x29,  232(sp)
    sd x30,  240(sp)
    sd x31,  248(sp)

    csrr t0, sepc
    sd   t0, 0(sp)

    mv   a0, sp
    call _ZN11TrapHandler10handleTrapEP9TrapFrame

    j _trap_restore_kernel

.align 2
_trap_user_entry:
    csrrw sp, sscratch, sp
    addi  sp, sp, -264

    sd x1,   8(sp)
    sd x3,   24(sp)
    sd x4,   32(sp)
    sd x5,   40(sp)
    sd x6,   48(sp)
    sd x7,   56(sp)
    sd x8,   64(sp)
    sd x9,   72(sp)
    sd x10,  80(sp)
    sd x11,  88(sp)
    sd x12,  96(sp)
    sd x13,  104(sp)
    sd x14,  112(sp)
    sd x15,  120(sp)
    sd x16,  128(sp)
    sd x17,  136(sp)
    sd x18,  144(sp)
    sd x19,  152(sp)
    sd x20,  160(sp)
    sd x21,  168(sp)
    sd x22,  176(sp)
    sd x23,  184(sp)
    sd x24,  192(sp)
    sd x25,  200(sp)
    sd x26,  208(sp)
    sd x27,  216(sp)
    sd x28,  224(sp)
    sd x29,  232(sp)
    sd x30,  240(sp)
    sd x31,  248(sp)

    csrr t0, sscratch
    sd   t0, 16(sp)
    addi t0, sp, 264
    sd   t0, 256(sp)

    csrr t0, sepc
    sd   t0, 0(sp)

    la   t0, _trap_kernel_entry
    csrw stvec, t0

    mv   a0, sp
    call _ZN11TrapHandler10handleTrapEP9TrapFrame

    j _trap_restore_user

.global _trap_restore_user
_trap_restore_user:
    ld   t0, 0(sp)
    csrw sepc, t0

    ld x1,   8(sp)
    ld x3,   24(sp)
    ld x4,   32(sp)
    ld x6,   48(sp)
    ld x7,   56(sp)
    ld x8,   64(sp)
    ld x9,   72(sp)
    ld x10,  80(sp)
    ld x11,  88(sp)
    ld x12,  96(sp)
    ld x13,  104(sp)
    ld x14,  112(sp)
    ld x15,  120(sp)
    ld x16,  128(sp)
    ld x17,  136(sp)
    ld x18,  144(sp)
    ld x19,  152(sp)
    ld x20,  160(sp)
    ld x21,  168(sp)
    ld x22,  176(sp)
    ld x23,  184(sp)
    ld x24,  192(sp)
    ld x25,  200(sp)
    ld x26,  208(sp)
    ld x27,  216(sp)
    ld x28,  224(sp)
    ld x29,  232(sp)
    ld x30,  240(sp)
    ld x31,  248(sp)

    la   t0, _trap_user_entry
    csrw stvec, t0

    ld   t0, 256(sp)
    csrw sscratch, t0

    ld x5,   40(sp)
    ld x2,   16(sp)

    sret


.global _trap_restore_kernel
_trap_restore_kernel:
    ld   t0, 0(sp)
    csrw sepc, t0

    ld x1,   8(sp)
    ld x3,   24(sp)
    ld x4,   32(sp)
    ld x5,   40(sp)
    ld x6,   48(sp)
    ld x7,   56(sp)
    ld x8,   64(sp)
    ld x9,   72(sp)
    ld x10,  80(sp)
    ld x11,  88(sp)
    ld x12,  96(sp)
    ld x13,  104(sp)
    ld x14,  112(sp)
    ld x15,  120(sp)
    ld x16,  128(sp)
    ld x17,  136(sp)
    ld x18,  144(sp)
    ld x19,  152(sp)
    ld x20,  160(sp)
    ld x21,  168(sp)
    ld x22,  176(sp)
    ld x23,  184(sp)
    ld x24,  192(sp)
    ld x25,  200(sp)
    ld x26,  208(sp)
    ld x27,  216(sp)
    ld x28,  224(sp)
    ld x29,  232(sp)
    ld x30,  240(sp)
    ld x31,  248(sp)

    ld x2,   16(sp)

    sret

.global _ZN11TrapHandler9timerTrapEv
.align 2
_ZN11TrapHandler9timerTrapEv:
    csrrw sp, mscratch, sp

    addi sp, sp, -16
    sd   t0, 0(sp)
    sd   t1, 8(sp)

    li   t1, 0x200BFF8
    ld   t1, 0(t1)
    li   t0, 1000000
    add  t1, t1, t0
    li   t0, 0x2004000
    sd   t1, 0(t0)

    li   t0, (1 << 5)

    ld   t0, 0(sp)
    ld   t1, 8(sp)
    addi sp, sp, 16

    csrrw sp, mscratch, sp
    mret