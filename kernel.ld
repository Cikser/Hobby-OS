OUTPUT_ARCH( "riscv" )

ENTRY( _start )

KERNEL_PHYS = 0x80000000;
KERNEL_VIRT = 0xFFFFFFC080000000;
KERNEL_OFFSET = KERNEL_VIRT - KERNEL_PHYS;

MEMORY
{
  ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

PHDRS
{
  text PT_LOAD;
  data PT_LOAD;
  bss  PT_LOAD;
}

SECTIONS
{
  . = KERNEL_PHYS;

  .text : {
    PROVIDE(_text_start = .);
    *(.text.init) *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(_text_end = .);
  } >ram :text

  .rodata : {
    . = ALIGN(4096);
    PROVIDE(_rodata_start = .);
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >ram :text

  .data : {
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*) *(.data .data.*)
    *(.init_array .init_array.*)
    *(.fini_array .fini_array.*)
    . = ALIGN(4096);
    PROVIDE(_data_end = .);
  } >ram :data

.bss : {
    . = ALIGN(4096);
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    . = ALIGN(4096);
    PROVIDE(_bss_end = .);
  } >ram :bss

  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
  PROVIDE(_heap_start = _bss_end);
  PROVIDE(_heap_size = _memory_end - _heap_start);

  PROVIDE(_kernel_phys_start = KERNEL_PHYS);
  PROVIDE(_kernel_phys_end = _bss_end);

  PROVIDE(_kernel_virt_base = KERNEL_VIRT);
  PROVIDE(_kernel_offset = KERNEL_OFFSET);

  PROVIDE(_text_start_virt = _text_start + KERNEL_OFFSET);
  PROVIDE(_text_end_virt = _text_end + KERNEL_OFFSET);
  PROVIDE(_bss_start_virt = _bss_start + KERNEL_OFFSET);
  PROVIDE(_bss_end_virt = _bss_end + KERNEL_OFFSET);
  PROVIDE(_heap_start_virt = _heap_start + KERNEL_OFFSET);
  PROVIDE(_memory_end_virt = _memory_end + KERNEL_OFFSET);
}