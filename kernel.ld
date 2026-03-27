OUTPUT_ARCH( "riscv" )

ENTRY( _start )

KERNEL_PHYS = 0x80000000;
KERNEL_VIRT = 0xFFFFFFC080000000;
KERNEL_OFFSET = KERNEL_VIRT - KERNEL_PHYS;

MEMORY
{
  ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
  kvirt (wxa!ri) : ORIGIN = KERNEL_VIRT, LENGTH = 128M
}

PHDRS
{
  init PT_LOAD;
  text PT_LOAD;
  data PT_LOAD;
  bss  PT_LOAD;
}

SECTIONS
{
  . = KERNEL_PHYS;

  .text.init : {
    *(.text.init)
    . = ALIGN(0x1000);
  } >ram :init

  .bss.init : {
    *(.bss.init)
    . = ALIGN(0x1000);
  } >ram :init

  /* Link the rest of the kernel in the higher half. */
  KERNEL_LMA_BASE = LOADADDR(.bss.init) + SIZEOF(.bss.init);
  KERNEL_VMA_BASE = KERNEL_VIRT + (KERNEL_LMA_BASE - KERNEL_PHYS);
  . = KERNEL_VMA_BASE;

  .text KERNEL_VMA_BASE : AT(KERNEL_LMA_BASE) {
    PROVIDE(_text_start = .);
    *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(_text_end = .);
  } >kvirt :text

  .rodata : AT(LOADADDR(.text) + SIZEOF(.text)) {
    . = ALIGN(4096);
    PROVIDE(_rodata_start = .);
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >kvirt :text

  .data : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*) *(.data .data.*)
    *(.init_array .init_array.*)
    *(.fini_array .fini_array.*)
    . = ALIGN(4096);
    PROVIDE(_data_end = .);
  } >kvirt :data

.bss : AT(ALIGN(LOADADDR(.data) + SIZEOF(.data), 4096)) {
    . = ALIGN(4096);
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    . = ALIGN(4096);
    PROVIDE(_bss_end = .);
  } >kvirt :bss

  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
  PROVIDE(_heap_start = _bss_end);
  PROVIDE(_heap_size = _memory_end - _heap_start);

  PROVIDE(_kernel_phys_start = KERNEL_PHYS);
  PROVIDE(_kernel_phys_end = _bss_end - KERNEL_OFFSET);

  PROVIDE(_bss_start_pa = LOADADDR(.bss));
  PROVIDE(_bss_end_pa   = LOADADDR(.bss) + SIZEOF(.bss));

  PROVIDE(_boot_pmt_pa  = LOADADDR(.bss) + (_ZN2VM9s_bootPmtE - ADDR(.bss)));

  PROVIDE(_start_pa = LOADADDR(.text) + (start - ADDR(.text)));

  PROVIDE(_main_pa = LOADADDR(.text) + (main - ADDR(.text)));
  PROVIDE(_text_pa = LOADADDR(.text));

  PROVIDE(_kernel_virt_base = KERNEL_VIRT);
  PROVIDE(_kernel_offset = KERNEL_OFFSET);

  PROVIDE(_text_start_virt = _text_start);
  PROVIDE(_text_end_virt = _text_end);
  PROVIDE(_bss_start_virt = _bss_start);
  PROVIDE(_bss_end_virt = _bss_end);
  PROVIDE(_heap_start_virt = _heap_start);
  PROVIDE(_memory_end_virt = _memory_end + KERNEL_OFFSET);
}