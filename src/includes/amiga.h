/***************************************************************************
Amiga Computer / Arcadia Game System

Driver by:

Ernesto Corvi & Mariusz Wojcieszek

***************************************************************************/

#ifndef __AMIGA_H__
#define __AMIGA_H__

#define MAX_PLANES 6 /* 0 to 6, inclusive ( but we count from 0 to 5 ) */

struct amiga_machine_interface
{
	int (*cia_0_portA_r)(void);
	int (*cia_0_portB_r)(void);
	void (*cia_0_portA_w)(int data);
	void (*cia_0_portB_w)(int data);

	int (*cia_1_portA_r)(void);
	int (*cia_1_portB_r)(void);
	void (*cia_1_portA_w)(int data);
	void (*cia_1_portB_w)(int data);

	UINT16 (*read_joy0dat)(void);
	UINT16 (*read_joy1dat)(void);
	UINT16 (*read_dskbytr)(void);
	void (*write_dsklen)(UINT16 data);

	void (*interrupt_callback)(void);
	void (*reset_callback)(void);
};



typedef struct {
	unsigned short INTENA; /* Interrupt Enable */
	unsigned short INTREQ; /* Interrupt Request */
	unsigned short BPLCON0; /* Bitplane Control 0 */
	unsigned short BPLCON1; /* Bitplane Control 1 */
	unsigned short BPLCON2; /* Bitplane Control 2 */
	unsigned long  BPLPTR[MAX_PLANES]; /* Bitplane pointer */
	unsigned short BPLxDAT[MAX_PLANES]; /* Bitplane data */
	unsigned short COPLCH[2]; /* Copper Location Hi */
	unsigned short COPLCL[2]; /* Copper Location Lo */
	unsigned short COPCON; /* Copper Control */
	unsigned short DMACON; /* DMA Control */
	unsigned short BLTCON0; /* Blitter Control 0 */
	unsigned short BLTCON1; /* Blitter Control 1 */
	unsigned short BLTAFWM; /* Blitter A First Word Mask */
	unsigned short BLTALWM; /* Blitter A Last Word Mask */
	unsigned short BLTxPTH[4]; /* Blitter Pointer's Hi */
	unsigned short BLTxPTL[4]; /* Blitter Pointer's Lo */
	unsigned short BLTSIZE; /* Blitter Start and Size */
	signed short   BLTxMOD[4]; /* Blitter Modulo's */
	unsigned short BLTxDAT[3]; /* Blitter Data Register's */
	unsigned long  SPRxPT[8]; /* Sprite pointers */
	unsigned short POTGO; /* Potentiometers start */
	unsigned short POTGOR; /* Potentiometers read */
	unsigned short DSKSYNC; /* Disk sync word */
	unsigned short DSKPTH; /* Disk pointer Hi */
	unsigned short DSKPTL; /* Disk pointer Lo */
	unsigned short DSKLEN; /* Disk Start and Size */
	unsigned short ADKCON; /* Audio and Disk Control */
	signed short   BPL1MOD; /* Odd planes modulo */
	signed short   BPL2MOD; /* Even planes modulo */
	unsigned short COLOR[32]; /* Color registers */
	unsigned short DIWSTRT; /* Display window start */
	unsigned short DIWSTOP; /* Display window stop */
	unsigned short DDFSTRT; /* Display data fetch start */
	unsigned short DDFSTOP; /* Display data fetch stop */
} custom_regs_def;

/* DMACON bit layout */
#define DMACON_AUD0EN	0x0001
#define DMACON_AUD1EN	0x0002
#define DMACON_AUD2EN	0x0004
#define DMACON_AUD3EN	0x0008
#define DMACON_DSKEN	0x0010
#define DMACON_SPREN	0x0020
#define DMACON_BLTEN	0x0040
#define DMACON_COPEN	0x0080
#define DMACON_BPLEN	0x0100
#define DMACON_DMAEN	0x0200
#define DMACON_BLTPRI	0x0400
#define DMACON_RSVED1	0x0800
#define DMACON_RSVED2	0x1000
#define DMACON_BZERO	0x2000
#define DMACON_BBUSY	0x4000
#define DMACON_SETCLR	0x8000

/* BPLCON0 bit layout */
#define BPLCON0_RSVED1	0x0001
#define BPLCON0_ERSY	0x0002
#define BPLCON0_LACE	0x0004
#define BPLCON0_LPEN	0x0008
#define BPLCON0_RSVED2	0x0010
#define BPLCON0_RSVED3	0x0020
#define BPLCON0_RSVED4	0x0040
#define BPLCON0_RSVED5	0x0080
#define BPLCON0_GAUD	0x0100
#define BPLCON0_COLOR	0x0200
#define BPLCON0_DBLPF	0x0400
#define BPLCON0_HOMOD	0x0800
#define BPLCON0_BPU0	0x1000
#define BPLCON0_BPU1	0x2000
#define BPLCON0_BPU2	0x4000
#define BPLCON0_HIRES	0x8000

/* prototypes */
/* machine */
extern void amiga_machine_config(const struct amiga_machine_interface *intf);
extern void copper_setpc( unsigned long pc );
extern WRITE16_HANDLER(amiga_custom_w);
extern void amiga_reload_sprite_info( int spritenum );
extern READ16_HANDLER(amiga_cia_r);
extern WRITE16_HANDLER(amiga_cia_w);
extern READ16_HANDLER(amiga_custom_r);
extern WRITE16_HANDLER(amiga_custom_w);
extern MACHINE_RESET(amiga);
extern void amiga_cia_issue_index( void );

extern UINT16 *amiga_expansion_ram;
extern UINT16 *amiga_autoconfig_mem;

/* vidhrdw */
extern INTERRUPT_GEN(amiga_vblank_irq);
extern INTERRUPT_GEN(amiga_irq);
extern VIDEO_UPDATE(amiga);
extern VIDEO_START(amiga);
extern PALETTE_INIT(amiga);
extern void amiga_prepare_frame(void);
extern void amiga_render_scanline(int scanline);

#endif /* __AMIGA_H__ */
