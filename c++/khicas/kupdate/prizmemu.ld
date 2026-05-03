OUTPUT_FORMAT(elf32-sh)
OUTPUT_ARCH(sh3)
ENTRY(initialize)
/* ENTRY(_main) */
 
MEMORY
{
        /* Loads code at 300000, skips g3a header */
        rom (rx) : o = 0x00300000, l = 2M-32000
        ram (rwx) : o = 0x08101000, l = 432k  /* skip 4k at start */
	/* 0x88160000 virtualized to 0x08100000 */
        ilram (rwx) : o = 0xE5200000, l = 4k
	/* RAM is available at 0x8c200000, 3M maybe more */
        r8c2 (rx) : o = 0x88200000, l = 2500K-4
}
 
SECTIONS
{
	.rominram : {
		z*.o(.text)
		z*.o(.text.*)
		z*.o(.rodata)
		z*.o(.rodata.*)
	} > r8c2

        /* Code, in ROM */
        .text : {
		*(.pretext)
                *(.text)
                *(.text.*)
        } > rom

        /* Read-only data, in ROM */
        .rodata : {
                *(.rodata)
                *(.rodata.*)
        } > rom
       
        /* RW initialized data, VMA in RAM but LMA in ROM */
        .data : ALIGN(4) {
                _datald = LOADADDR(.data) ;
                _sdata = . ;
                *(.data)
                *(.data.*);
                _edata = . ;
        } >ram AT>rom
       
        /* Uninitialized data (fill with 0), in RAM */
        .bss ALIGN(4) : ALIGN(4) {
                _bbss = . ;
                *(.bss)
                *(.bss*)
                *(COMMON)
                _ebss = . ;

		/* Extra unused static space */
		_sextra = ALIGN(32);
		_eextra = ORIGIN(ram) + LENGTH(ram);
        } >ram

}
