ENTRY(_start)
SECTIONS {
    . = 0x300000;
    .text : { *(.text.start) *(.text) *(.text.*) }
    .rodata : { *(.rodata) *(.rodata.*) }
    .data : { *(.data) *(.data.*) }
    .bss : { *(.bss) *(.bss.*) *(COMMON) }
}