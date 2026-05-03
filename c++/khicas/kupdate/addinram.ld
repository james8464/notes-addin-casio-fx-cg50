OUTPUT_FORMAT(elf32-sh)
OUTPUT_ARCH(sh3)

ENTRY(initialize)
MEMORY
{
        rom  : o = 0x00300000, l = 2M
        ram  : o = 0x08100000, l = 64k  /* pretty safe guess */
        r8c2 (rx) : o = 0xac200000, l = 3M-4
}

SECTIONS
{
	/* 2nd part of code at 0x8c20000 */
	 
        .rominram : {
                r8c2.o(.text)
                r8c2.o (.text.*)
                r8c2.o (.rodata)
                r8c2.o (.rodata.*)
        }  > r8c2 
       	
        /* Code, in ROM */
        .text : {
                *(.pretext)     /* init stuff */
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
