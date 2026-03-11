.equ STACK_SIZE, 8192

.global _start

_start:
    la   sp, stacks + STACK_SIZE    
    call start
    li t0, 0x100000
    li t1, 0x5555
    sw t1, (t0)

stacks:
    .skip STACK_SIZE
