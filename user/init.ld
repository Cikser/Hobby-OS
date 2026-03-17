/* userspace/init.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
    . = 0x10000;  /* userspace pocinje od 0x10000 */

    .text : {
        *(.text .text.*)
    }

    .rodata : {
        . = ALIGN(4096);
        *(.rodata .rodata.*)
    }

    .data : {
        . = ALIGN(4096);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(4096);
        *(.bss .bss.*)
    }
}