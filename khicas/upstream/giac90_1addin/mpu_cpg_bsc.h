#ifndef GINT_MPU_CPG_BSC
#define GINT_MPU_CPG_BSC

#include <stdint.h>
#include <stdbool.h>

#define GPACKED(x)  __attribute__((packed, aligned(x)))

#define pad_nam2(c)  _ ## c
#define pad_name(c)  pad_nam2(c)
#define pad(bytes)   uint8_t pad_name(__COUNTER__)[bytes]

#define lword_union(name, fields)		\
	union {					\
		uint32_t lword;			\
		struct { fields } GPACKED(4);	\
	} GPACKED(4) name

//---
// SH7305 But State Controller. Refer to:
//   Renesas SH7730 Group Hardware Manual
//   Section 11: Bus State Controller (BSC)
//---

typedef volatile lword_union(sh7305_bsc_CSnBCR_t,
	uint32_t :1;
	uint32_t IWW   :3;
	uint32_t IWRWD :3;
	uint32_t IWRWS :3;
	uint32_t IWRRD :3;
	uint32_t IWRRS :3;
	uint32_t TYPE  :4;
	uint32_t :1;
	uint32_t BSZ   :2;
	uint32_t :9;
);

typedef volatile lword_union(sh7305_bsc_CSnWCR_06A6B_t,
	uint32_t :11;
	uint32_t BAS   :1;
	uint32_t :1;
	uint32_t WW :3;
	uint32_t ADRSFIX:1;
	uint32_t :2;
	uint32_t SW :2;
	uint32_t WR :4;
	uint32_t WM :1;
	uint32_t :4;
	uint32_t HW :2;
);

typedef volatile lword_union(sh7305_bsc_CSnWCR_45A5B_t,
	uint32_t :11;
	uint32_t BAS   :1;
	uint32_t :1;
	uint32_t WW :3;
	uint32_t :3;
	uint32_t SW :2;
	uint32_t WR :4;
	uint32_t WM :1;
	uint32_t :4;
	uint32_t HW :2;
);

typedef volatile struct
{
	lword_union(CMNCR,
		uint32_t :6;
		uint32_t CKOSTP   :1;
		uint32_t CKODRV   :1;
		uint32_t :7;
		uint32_t DMSTP :1;
		uint32_t :1;
		uint32_t BSD   :1;
		uint32_t MAP   :2;
		uint32_t BLOCK :1;
		uint32_t :7;
		uint32_t ENDIAN   :1;
		uint32_t :1;
		uint32_t HIZMEM   :1;
		uint32_t HIZCNT   :1;
	);

	sh7305_bsc_CSnBCR_t CS0BCR;
	sh7305_bsc_CSnBCR_t CS2BCR;
	sh7305_bsc_CSnBCR_t CS3BCR;
	sh7305_bsc_CSnBCR_t CS4BCR;
	sh7305_bsc_CSnBCR_t CS5ABCR;
	sh7305_bsc_CSnBCR_t CS5BBCR;
	sh7305_bsc_CSnBCR_t CS6ABCR;
	sh7305_bsc_CSnBCR_t CS6BBCR;

	sh7305_bsc_CSnWCR_06A6B_t  CS0WCR;
	lword_union(CS2WCR,
		uint32_t :8;
		uint32_t BW :2;
		uint32_t PMD   :1;
		uint32_t BAS   :1;
		uint32_t :1;
		uint32_t WW :3;
		uint32_t :3;
		uint32_t SW :2;
		uint32_t WR :4;
		uint32_t WM :1;
		uint32_t :4;
		uint32_t HW :2;
	);
	lword_union(CS3WCR,
		uint32_t :17;
		uint32_t TRP   :2;
		uint32_t :1;
		uint32_t TRCD  :2;
		uint32_t :1;
		uint32_t A3CL  :2;
		uint32_t :2;
		uint32_t TRWL  :2;
		uint32_t :1;
		uint32_t TRC   :2;
	);
	sh7305_bsc_CSnWCR_45A5B_t  CS4WCR;
	sh7305_bsc_CSnWCR_45A5B_t  CS5AWCR;
	sh7305_bsc_CSnWCR_45A5B_t  CS5BWCR;
	sh7305_bsc_CSnWCR_06A6B_t  CS6AWCR;
	sh7305_bsc_CSnWCR_06A6B_t  CS6BWCR;

	lword_union(SDCR,
		uint32_t :11;
		uint32_t A2ROW :2;
		uint32_t :1;
		uint32_t A2COL :2;
		uint32_t :4;
		uint32_t RFSH  :1;
		uint32_t RMODE :1;
		uint32_t PDOWN :1;
		uint32_t BACTV :1;
		uint32_t :3;
		uint32_t A3ROW :2;
		uint32_t :1;
		uint32_t A3COL :2;
	);

	uint32_t RTCSR;
	uint32_t RTCNT;
	uint32_t RTCOR;

} GPACKED(4) sh7305_bsc_t;

#define SH7305_BSC (*(sh7305_bsc_t *)0xfec10000)

//---
// SH7305 Clock Pulse Generator. Refer to:
//   "Renesas SH7724 User's Manual: Hardware"
//   Section 17: "Clock Pulse Generator (CPG)"
//---

/* sh7305_cpg_t - Clock Pulse Generator registers
	Fields marked with [*] don't have the meaning described in the SH7724
	documentation. */
typedef volatile struct
{
	lword_union(FRQCR,
		uint32_t KICK  :1;   /* Flush FRQCRA modifications */
		uint32_t :1;
		uint32_t STC   :6;   /* PLL multiplication [*] */
		uint32_t IFC   :4;   /* Iphi divider 1 [*] */
		uint32_t :4;
		uint32_t SFC   :4;   /* Sphi divider 1 [*] */
		uint32_t BFC   :4;   /* Bphi divider 1 [*] */
		uint32_t :4;
		uint32_t P1FC  :4;   /* Pphi divider 1 [*] */
	);
	pad(0x4);

	lword_union(FSICLKCR,
		uint32_t :16;
		uint32_t DIVB  :6;   /* Division ratio for port B */
		uint32_t :1;
		uint32_t CLKSTP   :1;   /* Clock Stop */
		uint32_t SRC   :2;   /* Clock source select */
		uint32_t DIVA  :6;   /* Division ratio for port A */
	);
	pad(0x04);

	lword_union(DDCLKCR,
		uint32_t :23;
		uint32_t CLKSTP   :1;   /* Clock Stop */
		uint32_t _  :1;   /* Unknown */
		uint32_t :1;
		uint32_t DIV   :6;
	);

	lword_union(USBCLKCR,
		uint32_t :23;
		uint32_t CLKSTP   :1;   /* Clock Stop */
		uint32_t :8;
	);
	pad(0x0c);

	lword_union(PLLCR,
		uint32_t :17;
		uint32_t PLLE  :1;   /* PLL Enable */
		uint32_t :1;
		uint32_t FLLE  :1;   /* FLL Enable */
		uint32_t :10;
		uint32_t CKOFF :1;   /* CKO Output Stop */
		uint32_t :1;
	);

	uint32_t PLL2CR;
	pad(0x10);

	lword_union(SPUCLKCR,
		uint32_t :23;
		uint32_t CLKSTP   :1;   /* Clock Stop */
		uint32_t _  :1;   /* Unknown */
		uint32_t :1;
		uint32_t DIV   :6;   /* Division ratio */
	);
	pad(0x4);

	lword_union(SSCGCR,
		uint32_t SSEN  :1;   /* Spread Spectrum Enable */
		uint32_t :31;
	);
	pad(0x8);

	lword_union(FLLFRQ,
		uint32_t :16;
		uint32_t SELXM :2;   /* FLL output division */
		uint32_t :3;
		uint32_t FLF   :11;  /* FLL Multiplication Ratio */
	);
	pad(0x0c);

	uint32_t LSTATS;

} GPACKED(4) sh7305_cpg_t;

#define SH7305_CPG (*((sh7305_cpg_t *)0xa4150000))

#endif /* GINT_MPU_CPG_BSC */
