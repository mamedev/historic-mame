#define SYS16_CREDITS "Mirko Buffoni (Mame Driver)\nThierry Lescot & Nao (Hardware Info)\nPhil Stroffolino"

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/2151intf.h"

#define MRA_WORKINGRAM	MRA_BANK1
#define MWA_WORKINGRAM	MWA_BANK1
#define MRA_SPRITERAM	MRA_BANK2
#define MWA_SPRITERAM	MWA_BANK2
#define MRA_BGRAM		MRA_BANK3
#define MWA_BGRAM		MWA_BANK3
#define MRA_FGRAM		MRA_BANK4
#define MWA_FGRAM		MWA_BANK4
#define MRA_SRAM		MRA_BANK5
#define MWA_SRAM		MWA_BANK5

extern int sys16_vh_start( void );
extern void sys16_vh_stop( void );
extern void sys16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* video driver constants (vary with game) */
extern int sys16_spritesystem;
extern int sys16_sprxoffset;
extern int *sys16_obj_bank;
void (* sys16_update_proc)( void );

/* video driver registers */
extern int sys16_refreshenable;
extern int sys16_tile_bank0;
extern int sys16_tile_bank1;
extern int sys16_bg_scrollx, sys16_bg_scrolly;
extern int sys16_bg_page[4];
extern int sys16_fg_scrollx, sys16_fg_scrolly;
extern int sys16_fg_page[4];

unsigned char *sys16_backgroundram; /* background pages */
unsigned char *sys16_videoram; /* text layer */
unsigned char *sys16_spriteram; /* sprite ram */
unsigned char *sys16_refreshregister;

static unsigned char *sys16_workingram;
static unsigned char *sys16_scrollram;
static unsigned char *sys16_pageselram;
static unsigned char *sys16_sram;

/*   The VID buffer is an area of 4096 bytes * 16 pages.  Each page is
 *   composed of 64x32 2-bytes tiles.
 *
 *   REGPS (Page Select register) is a word that select a set of 4 screens
 *   out of 16 possible.  For instance, if the value (BIG Endian) is 0123h
 *   a scrolling background like the following will be selected:
 *
 *                          Page0  Page1
 *                          Page2  Page3
 *
 *   REGHS and REGVS (Horizontal and Vertical scrolling registers) are used
 *   to select the offsets of the background to be displayed.  The offsets
 *   are zero referenced at top-left point in scrolling background and
 *   represents the dots to be skipped.
 */

void sys16_paletteram_w(int offset, int data);
void sys16_paletteram_w(int offset, int data){
	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	if( oldword!=newword ){	/* we can do this because we initialized the palette */
							/* to black in vh_start() */
		/*	   byte 0    byte 1 */
		/*	GBGR BBBB GGGG RRRR */
		/*	5444 3210 3210 3210 */

		int r = (newword >> 0) & 0x0f;
		int g = (newword >> 4) & 0x0f;
		int b = (newword >> 8) & 0x0f;

		r = (r << 1) | ((newword >> 12) & 1);
		g = (g << 1) | ((newword >> 13) & 1);
		b = (b << 1) | ((newword >> 14) & 1);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_change_color( offset/2, r,g,b );

		WRITE_WORD (&paletteram[offset], newword);
	}
}

int sys16_interrupt( void ){
	return 4;       /* Interrupt vector 4, used by VBlank */
}

void sys16_refreshenable_w(int offset, int data){
	sys16_refreshenable = data & 0x20;
}

void sys16_scroll_w(int offset, int data);
void sys16_scroll_w(int offset, int data){
	COMBINE_WORD_MEM(&sys16_scrollram[offset], data);
	data = READ_WORD(&sys16_scrollram[offset]);

	switch (offset){
		case 0x0: sys16_fg_scrolly = data; break;
		case 0x2: sys16_bg_scrolly = data; break;
		case 0x8: sys16_fg_scrollx = data; break;
		case 0xa: sys16_bg_scrollx = data; break;
	}
}

void sys16_pagesel_w(int offset, int data);
void sys16_pagesel_w(int offset, int data){
	COMBINE_WORD_MEM(&sys16_pageselram[offset], data);
	data = READ_WORD(&sys16_pageselram[offset]);

	switch (offset){
		case 0x0:
		case 0x1:
		sys16_fg_page[0] = data>>12;
		sys16_fg_page[1] = (data>>8) & 0xf;
		sys16_fg_page[2] = (data>>4) & 0xf;
		sys16_fg_page[3] = data & 0xf;
		break;

		case 0x2:
		case 0x3:
		sys16_bg_page[0] = data>>12;
		sys16_bg_page[1] = (data>>8) & 0xf;
		sys16_bg_page[2] = (data>>4) & 0xf;
		sys16_bg_page[3] = data & 0xf;
		break;
	}
}

int shinobi_control_r (int offset){
	switch (offset){
		case 0: return input_port_2_r (offset);    /* GEN */
		case 2: return input_port_0_r (offset);    /* PL1 */
		case 6: return input_port_1_r (offset);    /* PL2 */
	}
	return 0x00;
}

int passshot_control_r (int offset){
	switch (offset){
		case 0: return input_port_2_r (offset);    /* GEN */
		case 2: return input_port_0_r (offset);    /* PL1 */
		case 4: return input_port_1_r (offset);    /* PL2 */
	}
	return 0x00;
}

int shinobi_dsw_r (int offset){
	switch (offset){
		case 0: return input_port_4_r (offset);    /* DS2 */
		case 2: return input_port_3_r (offset);    /* DS1 */
	}
	return 0x00;
}

void shinobi_scroll_w( int offset, int data ){
	COMBINE_WORD_MEM(&sys16_videoram[0x0e90+offset], data);
	data = READ_WORD(&sys16_videoram[0x0e90+offset]);

	switch (offset){
		case 0x0: sys16_fg_scrolly = data; break;
		case 0x2: sys16_bg_scrolly = data; break;
		case 0x8: sys16_fg_scrollx = data; break;
		case 0xa: sys16_bg_scrollx = data; break;
	}
}

void shinobi_pagesel_w(int offset, int data){
	COMBINE_WORD_MEM(&sys16_videoram[0x0e80+offset], data);
	data = READ_WORD(&sys16_videoram[0x0e80+offset]);

	switch (offset){
		case 0x0:
		case 0x1:
			sys16_fg_page[0] = data>>12;
			sys16_fg_page[1] = (data>>8) & 0xf;
			sys16_fg_page[2] = (data>>4) & 0xf;
			sys16_fg_page[3] = data & 0xf;
			break;

		case 0x2:
		case 0x3:
			sys16_bg_page[0] = data>>12;
			sys16_bg_page[1] = (data>>8) & 0xf;
			sys16_bg_page[2] = (data>>4) & 0xf;
			sys16_bg_page[3] = data & 0xf;
			break;
	}
}

void shinobi_refreshenable_w(int offset, int data){
	COMBINE_WORD_MEM(&sys16_refreshregister[offset], data);
	data = READ_WORD(&sys16_refreshregister[offset]);
	sys16_refreshenable = (data >> 8) & 0x20;
}

int goldnaxe_mirror1_r (int offset){
	return (input_port_0_r(offset) << 8) + input_port_1_r(offset);
}

static unsigned short mirror2[2];
void goldnaxe_mirror2_w( int offset, int data );
void goldnaxe_mirror2_w( int offset, int data ){
	COMBINE_WORD_MEM( mirror2, data );
	if( offset==0 ) sys16_tile_bank1 = mirror2[0]&0xf;
}
int goldnaxe_mirror2_r (int offset){
	if( offset==0 )
		return mirror2[0];
	else
		return (input_port_2_r(2)<<8) | (mirror2[1]&0xff);
}

void goldnaxe_refreshenable_w(int offset, int data){
	if( offset == 0 ) sys16_refreshenable = (data & 0x20);
}

void tetris_fgpagesel_w( int offset, int data ){
	switch (offset){
		case 0x0:
		case 0x1:
		sys16_fg_page[0] = data>>12;
		sys16_fg_page[1] = (data>>8) & 0xf;
		sys16_fg_page[2] = (data>>4) & 0xf;
		sys16_fg_page[3] = data & 0xf;
		break;
	}
}

void tetris_bgpagesel_w( int offset, int data ){
	switch (offset){
		case 0x0:
		case 0x1:
		sys16_bg_page[0] = data>>12;
		sys16_bg_page[1] = (data>>8) & 0xf;
		sys16_bg_page[2] = (data>>4) & 0xf;
		sys16_bg_page[3] = data & 0xf;
		break;
	}
}

void passshot_scroll_w(int offset, int data){
	COMBINE_WORD_MEM(&sys16_scrollram[offset], data);
	data = READ_WORD(&sys16_scrollram[offset]);

//	sys16_ram[0xf4bc+offset] = data;
	switch (offset)
	{
		case 0x0:  sys16_fg_scrolly = data; break;
		case 0x2:  sys16_fg_scrollx = data; break;
		case 0x4:  sys16_bg_scrolly = data; break;
		case 0x6:  sys16_bg_scrollx = data; break;
	}
}

void passshot_pagesel_w( int offset, int data ){
	COMBINE_WORD_MEM(&sys16_pageselram[offset], data);
	data = READ_WORD(&sys16_pageselram[offset]);

	switch (offset){
		case 0x0:
		case 0x1:
		sys16_bg_page[0] = data>>12;
		sys16_bg_page[1] = (data>>8) & 0xf;
		sys16_bg_page[2] = (data>>4) & 0xf;
		sys16_bg_page[3] = data & 0xf;
		break;

		case 0x2:
		case 0x3:
		sys16_fg_page[0] = data>>12;
		sys16_fg_page[1] = (data>>8) & 0xf;
		sys16_fg_page[2] = (data>>4) & 0xf;
		sys16_fg_page[3] = data & 0xf;
		break;
	}
}

/***************************************************************************

Altered Beast Memory Map:  Preliminary

000000-03ffff  ROM
ff0000-ffffff  RAM
fe0000-feffff  SRAM
400000-40ffff  Foreground and Background RAM
410000-410fff  VideoRAM
440000-440fff  SpriteRAM
840000-840fff  PaletteRAM

READ:
c41003  Player 1 Joystick
c41007  Player 2 Joystick
c41001  General control buttons
c42003  DSW1 :  Used for coin selection
c42001  DSW2 :  Used for Dip Switch settings

WRITE:
410e98-410e99  Foreground horizontal scroll
410e9a-410e9b  Background horizontal scroll
410e91-410e92  Foreground vertical scroll
410e93-410e94  Background vertical scroll
410e81-410e82  Foreground Page selector     \   There are 16 pages, of 1000h
410e83-410e84  Background Page selector     /   bytes long, for background
												and foreground on System16
Golden Axe Memory Map:  Preliminary

000000-0bffff  ROM
ff0000-ffffff  RAM
fe0000-feffff  SRAM
100000-10ffff  Foreground and Background RAM
110000-110fff  VideoRAM
200000-200fff  SpriteRAM
140000-140fff  PaletteRAM

READ:
c41003  Player 1 Joystick
c41007  Player 2 Joystick
c41001  General control buttons
c42003  DSW1 :  Used for coin selection
c42001  DSW2 :  Used for Dip Switch settings

MIRRORS:
0xffecd0 -> Mirror of 0xc41003
0xffecd1 -> Mirror of 0xc41007
0xffec96 -> Mirror of 0xc41001

WRITE:
110e98-110e99  Foreground horizontal scroll
110e9a-110e9b  Background horizontal scroll
110e91-110e92  Foreground vertical scroll
110e93-110e94  Background vertical scroll
110e81-110e82  Foreground Page selector     \   There are 16 pages, of 1000h
110e83-110e84  Background Page selector     /   bytes long, for background
												and foreground on System16
Shinobi Memory Map:  Preliminary

000000-03ffff  ROM
ff0000-ffffff  RAM
fe0000-feffff  SRAM
400000-40ffff  Foreground and Background RAM
410000-410fff  VideoRAM
440000-440fff  SpriteRAM
840000-840fff  PaletteRAM

READ:
c41003  Player 1 Joystick
c41007  Player 2 Joystick
c41001  General control buttons
c42003  DSW1 :  Used for coin selection
c42001  DSW2 :  Used for Dip Switch settings

WRITE:
410e98-410e99  Foreground horizontal scroll
410e9a-410e9b  Background horizontal scroll
410e91-410e92  Foreground vertical scroll
410e93-410e94  Background vertical scroll
410e81-410e82  Foreground Page selector     \   There are 16 pages, of 1000h
410e83-410e84  Background Page selector     /   bytes long, for background



***************************************************************************/

void sys16_sprite_decode( int num_banks, int bank_size );
void sys16_sprite_decode( int num_banks, int bank_size ){
	unsigned char *base = Machine->memory_region[2];
	unsigned char *temp = malloc( bank_size );
	int i;

	if( !temp ) return;

	for( i = num_banks; i >0; i-- ){
		unsigned char *finish	= base + 2*bank_size*i;
		unsigned char *dest = finish - 2*bank_size;

		unsigned char *p1 = temp;
		unsigned char *p2 = temp+bank_size/2;

		unsigned char data;

		memcpy (temp, base+bank_size*(i-1), bank_size);

		do {
			data = *p2++;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;

			data = *p1++;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;
		} while( dest<finish );
	}

	free( temp );
}

void sys16_soundcommand_w(int offset, int data);
void sys16_soundcommand_w(int offset, int data){
	switch (offset){
		case 0x2:
			soundlatch_w(0, data);
			break;
	}
}

static void sys16_irq_handler_mus(void);
static void sys16_irq_handler_mus(void){
	cpu_cause_interrupt (1, 0);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4096000,	/* 3.58 MHZ ? */
	{ 255 },
	{ sys16_irq_handler_mus }
};


/***************************************************************************/

void altbeast_update_proc( void );
void altbeast_update_proc( void ){
	unsigned short data = READ_WORD( &sys16_workingram[0xf094] );
	sys16_tile_bank1 = data&0xf;
	sys16_tile_bank0 = (data>>4)&0xf;
}
static void altbeast_init_machine(void);
static void altbeast_init_machine(void){
	static int bank[16] = { 0,0,2,0,4,0,6,0,8,0,10,0,12,0,0,0 };
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0xb8;
	sys16_update_proc = altbeast_update_proc;
}

static void goldnaxe_init_machine(void);
static void goldnaxe_init_machine(void){
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	static int bank[16] = { 0,2,8,10,16,18,0,0,4,6,12,14,20,22,0,0 };
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0xb8;

	/* Patch the code */
	WRITE_WORD (&RAM[0x003cb2], 0x4e75);
	WRITE_WORD (&RAM[0x0055bc], 0x6604);
	WRITE_WORD (&RAM[0x0055c6], 0x6604);
	WRITE_WORD (&RAM[0x0055d0], 0x6602);

	/* fix character select screen */
	memcpy( &RAM[0x60000], &RAM[0xa0000], 0x20000 );
}

static void shinobi_init_machine(void);
static void shinobi_init_machine(void){
	static int bank[16] = { 0,0,0,0,0,0,0,6,0,0,0,4,0,2,0,0 };
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0xb8;
}

static void tetris_init_machine(void);
static void tetris_init_machine(void){
	unsigned char *RAM = Machine->memory_region[0];
	static int bank[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0x40;

	/*	Patch the audio CPU */
	RAM = Machine->memory_region[3];
	RAM[0x020f] = 0xc7;
}

static void passshot_init_machine(void);
static void passshot_init_machine(void){
	static int bank[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,3 };
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0x48;
	sys16_spritesystem = 0;
}

/***************************************************************************/

static void altbeast_sprite_decode (void);
static void altbeast_sprite_decode (void){
	sys16_sprite_decode( 7, 0x20000 );
}

static void goldnaxe_sprite_decode (void);
static void goldnaxe_sprite_decode (void){
	sys16_sprite_decode( 3, 0x80000 );
}

static void shinobi_sprite_decode (void);
static void shinobi_sprite_decode (void){
	sys16_sprite_decode( 4, 0x20000 );
}

static void tetrisbl_sprite_decode (void);
static void tetrisbl_sprite_decode (void){
	sys16_sprite_decode( 1, 0x20000 );
}


/***************************************************************************/

static struct MemoryReadAddress altbeast_readmem[] =
{
	{ 0x410000, 0x410fff, MRA_FGRAM },
	{ 0x400000, 0x40ffff, MRA_BGRAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, paletteram_word_r },
	{ 0xc41000, 0xc41007, shinobi_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xfe0000, 0xfeffff, MRA_SRAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress altbeast_writemem[] =
{
	{ 0x410e90, 0x410e9f, sys16_scroll_w, &sys16_scrollram },
	{ 0x410e80, 0x410e87, sys16_pagesel_w, &sys16_pageselram },
	{ 0x410000, 0x410fff, MWA_FGRAM, &sys16_videoram },
	{ 0x400000, 0x40ffff, MWA_BGRAM, &sys16_backgroundram },
	{ 0x440000, 0x440fff, MWA_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, sys16_paletteram_w, &paletteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w },
	{ 0xc40004, 0xc4000f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xfe0000, 0xfeffff, MWA_SRAM, &sys16_sram },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM, &sys16_workingram },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress goldnaxe_readmem[] =
{
	{ 0x000000, 0x0bffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BGRAM },
	{ 0x110000, 0x110fff, MRA_FGRAM },
	{ 0x140000, 0x140fff, paletteram_word_r },
	{ 0x200000, 0x200fff, MRA_SPRITERAM },
	{ 0xc41000, 0xc41007, shinobi_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xfe0000, 0xfeffff, MRA_SRAM },
	{ 0xffecd0, 0xffecd3, goldnaxe_mirror1_r },
	{ 0xffec94, 0xffec97, goldnaxe_mirror2_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress goldnaxe_writemem[] =
{
	{ 0x000000, 0x0bffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BGRAM, &sys16_backgroundram },
	{ 0x110e90, 0x110e9f, sys16_scroll_w, &sys16_scrollram },
	{ 0x110e80, 0x110e87, sys16_pagesel_w, &sys16_pageselram },
	{ 0x110000, 0x110fff, MWA_FGRAM, &sys16_videoram },
	{ 0x140000, 0x140fff, sys16_paletteram_w, &paletteram },
	{ 0x1f0000, 0x1f0003, MWA_NOP }, /* IO Ctrl: Unknown */
	{ 0x200000, 0x200fff, MWA_SPRITERAM, &sys16_spriteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w },
	{ 0xc40008, 0xc4000f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xfe0000, 0xfeffff, MWA_SRAM, &sys16_sram },
	{ 0xffec94, 0xffec97, goldnaxe_mirror2_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM, &sys16_workingram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress shinobi_readmem[] =
{
	{ 0x410000, 0x410fff, MRA_FGRAM },
	{ 0x400000, 0x40ffff, MRA_BGRAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, paletteram_word_r },
	{ 0xc41000, 0xc41007, shinobi_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress shinobi_writemem[] =
{
	{ 0x410e90, 0x410e9f, shinobi_scroll_w },
	{ 0x410e80, 0x410e83, shinobi_pagesel_w },
	{ 0x410000, 0x410fff, MWA_FGRAM, &sys16_videoram },
	{ 0x418028, 0x41802b, tetris_bgpagesel_w },
	{ 0x418038, 0x41803b, tetris_fgpagesel_w },
	{ 0x400000, 0x40ffff, MWA_BGRAM, &sys16_backgroundram },
	{ 0x440000, 0x440fff, MWA_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, sys16_paletteram_w, &paletteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w, &sys16_refreshregister }, /* this is valid only for aliensyndrome, but other games should not use it */
	{ 0xc40004, 0xc4000f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xfe0004, 0xfe0007, sys16_soundcommand_w },
	{ 0xfff018, 0xfff018, shinobi_refreshenable_w, &sys16_refreshregister },  /* this is valid for Shinobi and Tetris */
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM, &sys16_workingram },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress passshot_readmem[] =
{
	{ 0x410000, 0x410fff, MRA_FGRAM },
	{ 0x400000, 0x40ffff, MRA_BGRAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, paletteram_word_r },
	{ 0xc41000, 0xc41007, passshot_control_r },
	{ 0xc42000, 0xc42007, shinobi_dsw_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress passshot_writemem[] =
{
	{ 0x410ff4, 0x410ff7, passshot_pagesel_w, &sys16_pageselram },
	{ 0x410000, 0x410fff, MWA_FGRAM, &sys16_videoram },
	{ 0x400000, 0x40ffff, MWA_BGRAM, &sys16_backgroundram },
	{ 0x440000, 0x440fff, MWA_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, sys16_paletteram_w, &paletteram },
	{ 0xc40000, 0xc40003, goldnaxe_refreshenable_w, &sys16_refreshregister }, /* this is valid only for aliensyndrome, but other games should not use it */
	{ 0xc40004, 0xc4000f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xc43000, 0xc4300f, MWA_NOP }, /* IO Ctrl:  Unknown */
	{ 0xc42004, 0xc42007, sys16_soundcommand_w },
	{ 0xfe0004, 0xfe0007, sys16_soundcommand_w },
	{ 0xfff4bc, 0xfff4c3, passshot_scroll_w, &sys16_scrollram },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM, &sys16_workingram },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};
static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};
static struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
	{ -1 }	/* end of table */
};
static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, IOWP_NOP },   /* adpcm */
	{ -1 }	/* end of table */
};

/***************************************************************************/

INPUT_PORTS_START( altbeast_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Credits needed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1 to start, 1 to continue")
	PORT_DIPSETTING(    0x00, "2 to start, 1 to continue")
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( goldnaxe_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_START1 , "Left Player Start", IP_KEY_DEFAULT, IP_JOY_NONE, 0 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_START2 , "Right Player Start", IP_KEY_DEFAULT, IP_JOY_NONE, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Credits needed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1 to start, 1 to continue")
	PORT_DIPSETTING(    0x00, "2 to start, 1 to continue")
	PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( shinobi_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright")
	PORT_DIPSETTING(    0x01, "Cocktail")
	PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Enemy's Bullet Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
INPUT_PORTS_END

INPUT_PORTS_START( aliensyn_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPSETTING(    0x01, "Off")
	PORT_DIPNAME( 0x02, 0x02, "Demo Sound?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Free (127?)", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "150" )
	PORT_DIPSETTING(    0x20, "140" )
	PORT_DIPSETTING(    0x10, "130" )
	PORT_DIPSETTING(    0x00, "120" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( tetrisbl_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )      /* Player Service in Alien Storm */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )      /* Player Service in Alien Storm */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright")
	PORT_DIPSETTING(    0x00, "Cocktail")
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "FREE" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Bullets Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

INPUT_PORTS_END

INPUT_PORTS_START( passshot_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0: coins per credit selection */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x04, "2/1 4/3")
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x01, "1/1 2/3")
	PORT_DIPSETTING(    0x02, "1/1 4/5")
	PORT_DIPSETTING(    0x03, "1/1 5/6")
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1")
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4")
	PORT_DIPSETTING(    0x40, "2/1 4/3")
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x10, "1/1 2/3")
	PORT_DIPSETTING(    0x20, "1/1 4/5")
	PORT_DIPSETTING(    0x30, "1/1 5/6")
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits")
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits")
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Attract Mode Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0e, 0x0e, "Initial Point", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "2000" )
	PORT_DIPSETTING(    0x0a, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPSETTING(    0x0e, "5000" )
	PORT_DIPSETTING(    0x08, "6000" )
	PORT_DIPSETTING(    0x04, "7000" )
	PORT_DIPSETTING(    0x02, "8000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x30, 0x30, "Point Table", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Game Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	3,	/* 3 bits per pixel */
	{ 0x40000*8, 0x20000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,	0, 256 },
	{ -1 } /* end of array */
};

/***************************************************************************/

static struct MachineDriver altbeast_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			altbeast_readmem,altbeast_writemem,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,	/* Monoprocessor for now */
	altbeast_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_vh_start,
	sys16_vh_stop,
	sys16_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};

static struct MachineDriver goldnaxe_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			goldnaxe_readmem,goldnaxe_writemem,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* Monoprocessor for now */
	goldnaxe_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_vh_start,
	sys16_vh_stop,
	sys16_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};

static struct MachineDriver shinobi_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			shinobi_readmem,shinobi_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,	/* 3 Mhz? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* Monoprocessor for now */
	shinobi_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_vh_start,
	sys16_vh_stop,
	sys16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};

static struct MachineDriver tetris_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			shinobi_readmem,shinobi_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,	/* 4 Mhz? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	tetris_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_vh_start,
	sys16_vh_stop,
	sys16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};

static struct MachineDriver passshot_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz */
			0,
			passshot_readmem,passshot_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,	/* 4 Mhz? */
			3,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	2,	/* Monoprocessor for now */
	passshot_init_machine,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_vh_start,
	sys16_vh_stop,
	sys16_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};

/***************************************************************************/

ROM_START( altbeast_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_ODD ( "ab11739.bin", 0x00000, 0x20000, 0x2a86ef80 , 0xe466eb65 )
	ROM_LOAD_EVEN( "ab11740.bin", 0x00000, 0x20000, 0xee618577 , 0xce227542 )

	ROM_REGION_DISPOSE(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ab11674.bin", 0x00000, 0x20000, 0x11a5b38d , 0xa57a66d5 )        /* 8x8 0 */
	ROM_LOAD( "ab11675.bin", 0x20000, 0x20000, 0xb4da9b5c , 0x2ef2f144 )        /* 8x8 1 */
	ROM_LOAD( "ab11676.bin", 0x40000, 0x20000, 0xe29679fe , 0x0c04acac )        /* 8x8 2 */

	ROM_REGION(0x0e0000 * 2)
	ROM_LOAD( "ab11677.bin", 0x000000, 0x10000, 0x4790062c , 0xf8b3684e )     /* sprites */
	ROM_LOAD( "ab11681.bin", 0x010000, 0x10000, 0x64ba3d78 , 0xae3c2793 )
	ROM_LOAD( "ab11726.bin", 0x020000, 0x10000, 0x92b73325 , 0x3cce5419 )
	ROM_LOAD( "ab11730.bin", 0x030000, 0x10000, 0x1206c29c , 0x3af62b55 )
	ROM_LOAD( "ab11678.bin", 0x040000, 0x10000, 0xd7e015d2 , 0xb0390078 )
	ROM_LOAD( "ab11682.bin", 0x050000, 0x10000, 0x24a06d36 , 0x2a87744a )
	ROM_LOAD( "ab11728.bin", 0x060000, 0x10000, 0xce35e3ed , 0xf3a43fd8 )
	ROM_LOAD( "ab11732.bin", 0x070000, 0x10000, 0xbe1f7251 , 0x2fb3e355 )
	ROM_LOAD( "ab11679.bin", 0x080000, 0x10000, 0x047b56d5 , 0x676be0cb )
	ROM_LOAD( "ab11683.bin", 0x090000, 0x10000, 0xcd0f3973 , 0x802cac94 )
	ROM_LOAD( "ab11718.bin", 0x0a0000, 0x10000, 0x084baced , 0x882864c2 )
	ROM_LOAD( "ab11734.bin", 0x0b0000, 0x10000, 0x60de3d94 , 0x76c704d2 )
	ROM_LOAD( "ab11680.bin", 0x0c0000, 0x10000, 0x8a3b1011 , 0x339987f7 )
	ROM_LOAD( "ab11684.bin", 0x0d0000, 0x10000, 0x594c9fdc , 0x4fe406aa )
ROM_END

ROM_START( goldnaxe_rom )
	ROM_REGION(0xc0000)	/* 12*64k for 68000 code */
	ROM_LOAD_ODD ( "ga12522.bin", 0x00000, 0x20000, 0xbc78cf80 , 0xb6c35160 )
	ROM_LOAD_EVEN( "ga12523.bin", 0x00000, 0x20000, 0xc4cd57c9 , 0x8e6128d7 )
	ROM_LOAD_ODD ( "ga12544.bin", 0x40000, 0x40000, 0x0b92254c , 0x5e38f668 )
	ROM_LOAD_EVEN( "ga12545.bin", 0x40000, 0x40000, 0x83bcc58e , 0xa97c4e4d )

	ROM_REGION_DISPOSE(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ga12385.bin", 0x00000, 0x20000, 0xd774ab10 , 0xb8a4e7e0 )        /* 8x8 0 */
	ROM_LOAD( "ga12386.bin", 0x20000, 0x20000, 0x08e5b5e3 , 0x25d7d779 )        /* 8x8 1 */
	ROM_LOAD( "ga12387.bin", 0x40000, 0x20000, 0x49afcab3 , 0xc7fcadf3 )        /* 8x8 2 */

	ROM_REGION(0x180000*2)
	ROM_LOAD( "ga12378.bin", 0x000000, 0x40000, 0x3bbf4177 , 0x119e5a82 )     /* sprites */
	ROM_LOAD( "ga12379.bin", 0x040000, 0x40000, 0x06616abf , 0x1a0e8c57 )
	ROM_LOAD( "ga12380.bin", 0x080000, 0x40000, 0x92c8cf4c , 0xbb2c0853 )
	ROM_LOAD( "ga12381.bin", 0x0c0000, 0x40000, 0x9c0347cb , 0x81ba6ecc )
	ROM_LOAD( "ga12382.bin", 0x100000, 0x40000, 0xb0ca8be0 , 0x81601c6f )     /* sprites */
	ROM_LOAD( "ga12383.bin", 0x140000, 0x40000, 0x4db9deb3 , 0x5dbacf7a )

ROM_END

ROM_START( shinobi_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_ODD ( "shinobi.a1", 0x00000, 0x10000, 0xfe8b9d89 , 0x343f4c46 )
	ROM_LOAD_EVEN( "shinobi.a4", 0x00000, 0x10000, 0x8f1814ee , 0xb930399d )
	ROM_LOAD_ODD ( "shinobi.a2", 0x20000, 0x10000, 0xc27fc83f , 0x7961d07e )
	ROM_LOAD_EVEN( "shinobi.a5", 0x20000, 0x10000, 0x24ed7c53 , 0x9d46e707 )

	ROM_REGION_DISPOSE(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "shinobi.b9", 0x00000, 0x10000, 0x48f08ef6 , 0x5f62e163 )        /* 8x8 0 */
	ROM_LOAD( "shinobi.b10", 0x20000, 0x10000, 0x68400bb0 , 0x75f8fbc9 )        /* 8x8 1 */
	ROM_LOAD( "shinobi.b11", 0x40000, 0x10000, 0x28d8e20e , 0x06508bb9 )        /* 8x8 2 */

	ROM_REGION(0x80000 * 2) /* sprites */
	ROM_LOAD( "shinobi.b1", 0x00000, 0x10000, 0xcffe9a7c , 0x611f413a )
	ROM_LOAD( "shinobi.b5", 0x10000, 0x10000, 0xc08a5da8 , 0x5eb00fc1 )
	ROM_LOAD( "shinobi.b2", 0x20000, 0x10000, 0xd8d607c2 , 0x3c0797c0 )
	ROM_LOAD( "shinobi.b6", 0x30000, 0x10000, 0x97cf47a1 , 0x25307ef8 )
	ROM_LOAD( "shinobi.b3", 0x40000, 0x10000, 0xed9c64e2 , 0xc29ac34e )
	ROM_LOAD( "shinobi.b7", 0x50000, 0x10000, 0x760836ba , 0x04a437f8 )
	ROM_LOAD( "shinobi.b4", 0x60000, 0x10000, 0xd8b2b390 , 0x41f41063 )
	ROM_LOAD( "shinobi.b8", 0x70000, 0x10000, 0x8e3087a8 , 0xb6e1fd72 )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	ROM_LOAD( "shinobi.a7", 0x0000, 0x8000, 0x74c6c582 , 0x2457a7cf )
ROM_END


ROM_START( aliensyn_rom )
	ROM_REGION(0x40000)     /* 6*32k for 68000 code */
	ROM_LOAD_ODD ( "11080.a1", 0x00000, 0x8000, 0xb9cab5ae , 0xfe7378d9 )
	ROM_LOAD_EVEN( "11083.a4", 0x00000, 0x8000, 0x1eff7093 , 0xcb2ad9b3 )
	ROM_LOAD_ODD ( "11081.a2", 0x10000, 0x8000, 0x35b95ec1 , 0x1308ee63 )
	ROM_LOAD_EVEN( "11084.a5", 0x10000, 0x8000, 0xaf10166e , 0x2e1ec7b1 )
	ROM_LOAD_ODD ( "11082.a3", 0x20000, 0x8000, 0xe00daaa3 , 0x9cdc2a14 )
	ROM_LOAD_EVEN( "11085.a6", 0x20000, 0x8000, 0x41bce6e2 , 0xcff78f39 )

	ROM_REGION_DISPOSE(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "10702.b9", 0x00000, 0x10000, 0xe7889186 , 0x393bc813 )        /* 8x8 0 */
	ROM_LOAD( "10703.b10", 0x20000, 0x10000, 0x13ac0520 , 0x6b6dd9f5 )        /* 8x8 1 */
	ROM_LOAD( "10704.b11", 0x40000, 0x10000, 0x93bef886 , 0x911e7ebc )        /* 8x8 2 */

	ROM_REGION(0x80000*2)
	ROM_LOAD( "10709.b1", 0x00000, 0x10000, 0x24e86498 , 0xaddf0a90 )     /* sprites */
	ROM_LOAD( "10713.b5", 0x10000, 0x10000, 0x93353427 , 0xececde3a )
	ROM_LOAD( "10710.b2", 0x20000, 0x10000, 0xb3c675d6 , 0x992369eb )
	ROM_LOAD( "10714.b6", 0x30000, 0x10000, 0x77704f2e , 0x91bf42fb )
	ROM_LOAD( "10711.b3", 0x40000, 0x10000, 0x7636e106 , 0x29166ef6 )     /* sprites */
	ROM_LOAD( "10715.b7", 0x50000, 0x10000, 0x069913ad , 0xa7c57384 )
	ROM_LOAD( "10712.b4", 0x60000, 0x10000, 0x2eb0fade , 0x876ad019 )
	ROM_LOAD( "10716.b8", 0x70000, 0x10000, 0xfe474ccd , 0x40ba1d48 )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	/* not supported yet! */
ROM_END


ROM_START( tetrisbl_rom )
	ROM_REGION(0x40000)     /* 2*32k for 68000 code */
	ROM_LOAD_ODD ( "rom1.bin", 0x00000, 0x8000, 0xcf652215 , 0x1e912131 )
	ROM_LOAD_EVEN( "rom2.bin", 0x00000, 0x8000, 0xc170a0f6 , 0x4d165c38 )

	ROM_REGION_DISPOSE(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scr01.rom", 0x00000, 0x10000, 0x65864110 , 0x62640221 )        /* 8x8 0 */
	ROM_LOAD( "scr02.rom", 0x20000, 0x10000, 0x24f4f4b4 , 0x9abd183b )        /* 8x8 1 */
	ROM_LOAD( "scr03.rom", 0x40000, 0x10000, 0xa19cfd40 , 0x2495fd4e )        /* 8x8 2 */

	ROM_REGION(0x20000*2)
	ROM_LOAD( "obj0-o.rom", 0x00000, 0x10000, 0x54463eb2 , 0x2fb38880 )     /* sprites */
	ROM_LOAD( "obj0-e.rom", 0x10000, 0x10000, 0x5ef58537 , 0xd6a02cba )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	ROM_LOAD( "s-prog.rom", 0x0000, 0x8000, 0x614b77e5 , 0xbd9ba01b )
ROM_END


ROM_START( passshtb_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_ODD ( "pass4_2p.bin", 0x00000, 0x10000, 0x9d3fc8bd , 0x06ac6d5d )
	ROM_LOAD_EVEN( "pass3_2p.bin", 0x00000, 0x10000, 0x82700ff0 , 0x26bb9299 )

	ROM_REGION_DISPOSE(0x60000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "passshot.b9", 0x00000, 0x10000, 0xd87a3c78 , 0xd31c0b6c )        /* 8x8 0 */
	ROM_LOAD( "passshot.b10", 0x20000, 0x10000, 0xcb2fb653 , 0xb78762b4 )        /* 8x8 1 */
	ROM_LOAD( "passshot.b11", 0x40000, 0x10000, 0x5981f12f , 0xea49f666 )        /* 8x8 2 */

	ROM_REGION(0x80000*2)
	ROM_LOAD( "passshot.b1", 0x00000, 0x10000, 0xcae4a044 , 0xb6e94727 )     /* sprites */
	ROM_LOAD( "passshot.b5", 0x10000, 0x10000, 0xb8bc0832 , 0x17e8d5d5 )
	ROM_LOAD( "passshot.b2", 0x20000, 0x10000, 0x8fb69228 , 0x3e670098 )
	ROM_LOAD( "passshot.b6", 0x30000, 0x10000, 0xa77128e5 , 0x50eb71cc )
	ROM_LOAD( "passshot.b3", 0x40000, 0x10000, 0x1375d339 , 0x05733ca8 )     /* sprites */
	ROM_LOAD( "passshot.b7", 0x50000, 0x10000, 0x4dfcbd52 , 0x81e49697 )

	ROM_REGION(0x10000)		/* 64k for audio cpu */
	ROM_LOAD( "passshot.a7", 0x0000, 0x8000, 0x37f388e5 , 0x789edc06 )
ROM_END

/***************************************************************************/

struct GameDriver altbeast_driver =
{
	__FILE__,
	0,
	"altbeast",
	"Altered Beast",
	"1988",
	"Sega",
	SYS16_CREDITS,
	0,
	&altbeast_machine_driver,

	altbeast_rom,
	altbeast_sprite_decode, 0,
	0,
	0,

	altbeast_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver goldnaxe_driver =
{
	__FILE__,
	0,
	"goldnaxe",
	"Golden Axe",
	"1989",
	"Sega",
	SYS16_CREDITS,
	0,
	&goldnaxe_machine_driver,

	goldnaxe_rom,
	goldnaxe_sprite_decode, 0,
	0,
	0,

	goldnaxe_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver shinobi_driver =
{
	__FILE__,
	0,
	"shinobi",
	"Shinobi",
	"1987",
	"Sega",
	SYS16_CREDITS,
	0,
	&shinobi_machine_driver,

	shinobi_rom,
	shinobi_sprite_decode, 0,
	0,
	0,

	shinobi_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};


struct GameDriver aliensyn_driver =
{
	__FILE__,
	0,
	"aliensyn",
	"Alien Syndrome",
	"1987",
	"Sega",
	SYS16_CREDITS,
	0,
	&shinobi_machine_driver,

	aliensyn_rom,
	shinobi_sprite_decode, 0,
	0,
	0,

	aliensyn_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver tetrisbl_driver =
{
	__FILE__,
	0,
	"tetrisbl",
	"Tetris (Sega, bootleg)",
	"1988",
	"Sega",
	SYS16_CREDITS,
	0,
	&tetris_machine_driver,

	tetrisbl_rom,
	tetrisbl_sprite_decode, 0,
	0,
	0,

	tetrisbl_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};


struct GameDriver passshtb_driver =
{
	__FILE__,
	0,
	"passshtb",
	"Passing Shot (bootleg)",
	"????",
	"bootleg",
	SYS16_CREDITS,
	0,
	&passshot_machine_driver,

	passshtb_rom,
	shinobi_sprite_decode, 0,
	0,
	0,

	passshot_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
//	ORIENTATION_ROTATE_270,
	0, 0
};
