/**************************************************************************
Blood Bros, West Story.  (preliminary)
TAD Corporation 1990/Datsu 1991
68000 + Z80 + YM3931 + YM3812

TODO:

(*) Global
- Sprites priorities/clip.
- Fix timing?? Foreground layer flickers during the game!!!!
- Add sound.
- hiscore.

(*) Blood Bros

(*) West Story
- Sprites problems. (decode problems?)


Notes on sound by Bryan:
Interesting trivia - take a look in the sound cpu code and you'll see
it's credited (c) 1986 Seibu!

And that leads to the fact I spent a good amount of the afternoon
working on the damn Raiden sound cpu...Ê Which should be identical to
the West Story sound hardware.

The interface is more complicated than usual, the soundlatch seems to
be 16 bits and in the case of Raiden the sound cpu controls the coin
inputs and uses shared memory with the main cpu to signal coin
inserts, I don't know if West Story does the same..Ê Anyway, plug
this lot in:

static void sound_bank_w(int offset,int data){
	unsigned char *RAM = Machine->memory_region[2];
	if (data&1) { cpu_setbank(1,&RAM[0x0000]); }
	else { cpu_setbank(1,&RAM[0x8000]); }
}

static struct MemoryReadAddress sound_readmem[] = {
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM3812_status_port_0_r },
	{ 0x4010, 0x4011, maincpu_data_r }, // Soundlatch (16 bits)
	{ 0x4013, 0x4013, input_port_4_r }, // Coins
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem[] = {
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, sound_clear_w }, // Or command ack??
	{ 0x4002, 0x4002, MWA_NOP }, // RST 10h interrupt ack
	{ 0x4003, 0x4003, MWA_NOP }, // RST 18h interrupt ack
	{ 0x4007, 0x4007, sound_bank_w },
	{ 0x4008, 0x4008, YM3812_control_port_0_w },
	{ 0x4009, 0x4009, YM3812_write_port_0_w },
	{ 0x4019, 0x401f, maincpu_data_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ -1 }
}


Alter the YM3812 to generate:

static void YM3812_irqhandler(void){
	cpu_cause_interrupt(2,0xd7); // RST 10h
}

And use:

cpu_cause_interrupt(2,0xdf); // RST 18

when the soundlatch is written to.Ê (Remove the old interrupt handler
in the machine structure).

As I say, I haven't actually compiled it to try it in West Story but
it looks about right ;)

Bryan

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

extern void bloodbro_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
extern void weststry_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
extern int bloodbro_vh_start(void);
extern void bloodbro_vh_stop(void);

extern int bloodbro_background_r( int offset );
extern void bloodbro_background_w( int offset, int data );
extern int bloodbro_foreground_r( int offset );
extern void bloodbro_foreground_w( int offset, int data );

extern unsigned char *bloodbro_videoram2;
extern unsigned char *textlayoutram;
extern unsigned char *bloodbro_scroll;

/***************************************************************************/

void bloodbro_playsample(int offset, int number){
	/*unsigned char *RAM = Machine->memory_region[5];*/
	int start,end;

        /* List getten from offset 0 of the sample file */
        static int sam[29] = {0x000400, 0x002660, 0x0061e0, 0x0065b0, 0x006c90,
                       0x00aa90, 0x00b200, 0x00bad0, 0x00c2c0, 0x00d070,
                       0x00ecb0, 0x00ff80, 0x010ee0, 0x011950, 0x013710,
                       0x0143c0, 0x015230, 0x017240, 0x018470, 0x019ba0,
                       0x01a050, 0x01b280, 0x01c320, 0x01c960, 0x01ce70,
                       0x01d2e0, 0x01daf0, 0x01e230, 0x1fffff };

        if ((number > 0) && (number<29)) { /* 28 samples */
          start = sam[number-1];
          end = sam[number];
	  ADPCM_play( 0, start, end);}
}

/***************************************************************************/

int bloodbro_r_read(int offset) {
     //if( errorlog ) fprintf( errorlog, "INPUT e000[%x] \n", offset);
     switch (offset) {
       case 0x0: /* DIPSW 1&2 */
                 return readinputport(4) + readinputport(5)*256;
       case 0x2: /* IN1-IN2 */
                 return readinputport(0) + readinputport(1)*256;
       case 0x4: /* ???-??? */
                 return readinputport(2) + readinputport(3)*256;
       default: return (0xffff);
     }
}

int bloodbro_r_read_a0000(int offset) { /*SOUNDLATCH RELATED SEE 68000 CODE - 000854*/
      //if( errorlog ) fprintf( errorlog, "INPUT a000[%x] \n", offset);
      return (0x0060);
}

void bloodbro_w_write(int offset,int data) {
	switch (offset) {
		case 0x0: /* Sound latch write */
		soundlatch_w(offset,data);
		return;

//		default:
//		if( errorlog ) fprintf( errorlog, "OUTPUT a000[%x] %02x \n", offset,data);
//		break;
	}
}

/**** Blood Bros Memory Map  *******************************************/

static struct MemoryReadAddress readmem_cpu[] = {
	{ 0x00000, 0x7ffff, MRA_ROM },
	{ 0x80000, 0x8afff, MRA_RAM },
	{ 0x8b000, 0x8bfff, MRA_RAM, &spriteram },
	{ 0x8c000, 0x8c3ff, bloodbro_background_r },
	{ 0x8c400, 0x8cfff, MRA_RAM },
	{ 0x8d000, 0x8d3ff, bloodbro_foreground_r },
	{ 0x8d400, 0x8d7ff, MRA_RAM },
	{ 0x8d800, 0x8dfff, MRA_RAM, &textlayoutram },
	{ 0x8e000, 0x8e7ff, MRA_RAM },
	{ 0x8e800, 0x8f7ff, paletteram_word_r },
	{ 0x8f800, 0x8ffff, MRA_RAM },
	{ 0xa0000, 0xa0012, bloodbro_r_read_a0000 },
	{ 0xc0000, 0xc007f, MRA_BANK1, &bloodbro_scroll },
	{ 0xe0000, 0xe000f, bloodbro_r_read },
	{ -1 }
};

static struct MemoryWriteAddress writemem_cpu[] = {
	{ 0x00000, 0x7ffff, MWA_ROM },
	{ 0x80000, 0x8bfff, MWA_RAM },
	{ 0x8c000, 0x8c3ff, bloodbro_background_w, &videoram },   /* Background RAM */
	{ 0x8c400, 0x8cfff, MWA_RAM },
	{ 0x8d000, 0x8d3ff, bloodbro_foreground_w, &bloodbro_videoram2 }, /* Foreground RAM */
	{ 0x8d400, 0x8e7ff, MWA_RAM },
	{ 0x8e800, 0x8f7ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x8f800, 0x8ffff, MWA_RAM },
	{ 0xa0000, 0xa000f, bloodbro_w_write },
	{ 0xc0000, 0xc007f, MWA_BANK1 },
	{ 0xc0080, 0xc0081, MWA_NOP }, /* IRQ Ack VBL? */
	{ 0xc00c0, 0xc00c1, MWA_NOP }, /* watchdog? */
	//{ 0xc0100, 0xc0100, MWA_NOP }, /* ?? Written 1 time */
	{ -1 }
};

/**** West Story Memory Map ********************************************/

static struct MemoryReadAddress weststry_readmem_cpu[] = {
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x08afff, MRA_RAM },
	{ 0x08b000, 0x08bfff, MRA_RAM, &spriteram },
	{ 0x08c000, 0x08c3ff, bloodbro_background_r },
	{ 0x08c400, 0x08cfff, MRA_RAM },
	{ 0x08d000, 0x08d3ff, bloodbro_foreground_r },
	{ 0x08d400, 0x08dfff, MRA_RAM },
	{ 0x08d800, 0x08dfff, MRA_RAM, &textlayoutram },
	{ 0x08e000, 0x08ffff, MRA_RAM },
	{ 0x0c1000, 0x0c100f, bloodbro_r_read },
	{ 0x0c1010, 0x0c17ff, MRA_BANK1 },
	{ 0x128000, 0x1287ff, paletteram_word_r },
	{ 0x120000, 0x128fff, MRA_BANK2 },
	{ -1 }
};

static struct MemoryWriteAddress weststry_writemem_cpu[] = {
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080000, 0x08bfff, MWA_RAM },
	{ 0x08c000, 0x08c3ff, bloodbro_background_w, &videoram },   /* Background RAM */
	{ 0x08c400, 0x08cfff, MWA_RAM },
	{ 0x08d000, 0x08d3ff, bloodbro_foreground_w, &bloodbro_videoram2 }, /* Foreground RAM */
	{ 0x08d400, 0x08ffff, MWA_RAM },
	{ 0x0c1010, 0x0c17ff, MWA_BANK1 },
	{ 0x128000, 0x1287ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x120000, 0x128fff, MWA_BANK2 },
	{ -1 }
};

/**** Blood Bros Sound Memory Map **************************************/

int return_fe(int offset) { return (0xfe); }

static struct MemoryReadAddress readmem_sound[] = {
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM3812_status_port_0_r },
	{ 0x4010, 0x4012, return_fe },
	{ 0x4013, 0x4013, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_sound[] = {
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
//{ 0x4000, 0x4007, MWA_NOP },
	{ 0x4008, 0x4008, YM3812_control_port_0_w }, //??
	{ 0x4009, 0x4009, YM3812_write_port_0_w },   //??
//{ 0x4018, 0x4018, bloodbro_playsample },   //ADPCM??
//{ 0x4019, 0x4019, MWA_NOP },               //ADPCM??
//{ 0x401b, 0x401b, MWA_NOP },               //ADPCM??
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};

/**** Blood Bros Input Ports *******************************************/

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x01, "Free Play" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x20, 0x20, "Starting Coin" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "x2" )
	PORT_DIPNAME( 0x40, 0x40, "Unused 1" )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Unused 2" )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "300K 500K" )
	PORT_DIPSETTING(    0x08, "500K 500K" )
	PORT_DIPSETTING(    0x04, "500K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
INPUT_PORTS_END

/**** West Story Input Ports *******************************************/

INPUT_PORTS_START( weststry_input_ports )
	PORT_START	/* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1)
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/**** Blood Bros gfx decode ********************************************/

static struct GfxLayout textlayout = {
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x10000*8, 0x10000*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout backlayout = {
	16,16,	/* 16*16 sprites  */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout = {
	16,16,	/* 16*16 sprites  */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo bloodbro_gfxdecodeinfo[] = {
	{ 2,      0x00000, &textlayout,   0x70*16,  0x10 }, /* Text */
	{ 3,      0x00000, &backlayout,   0x40*16,  0x10 }, /* Background */
	{ 3,      0x80000, &backlayout,   0x50*16,  0x10 }, /* Foreground */
	{ 4,      0x00000, &spritelayout, 0x00*16,  0x10 }, /* Sprites */
	{ -1 }
};

/**** West Story gfx decode *********************************************/

static struct GfxLayout weststry_textlayout = {
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 0x8000*8, 2*0x8000*8, 3*0x8000*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout weststry_backlayout = {
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
         	16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout weststry_spritelayout = {
	16,16,	/* 16*16 sprites */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 0*0x40000*8, 1*0x40000*8, 2*0x40000*8, 3*0x40000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
         	16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo weststry_gfxdecodeinfo[] = {
	{ 2,      0x00000, &weststry_textlayout,     16*16,  0x10 },
	{ 3,      0x00000, &weststry_backlayout,     48*16,  0x10 },
	{ 3,      0x80000, &weststry_backlayout,     32*16,  0x10 },
	{ 4,      0x00000, &weststry_spritelayout,    0*16,  0x10 },
	{ -1 }
};

/**** Blood Bros Interrupt & Driver Machine  ****************************/

static int snd_interrupt(void) {
	if (cpu_getiloops() == 0)
	   return 0xd7;		/* RST 10h */
	else
   	   return 0xdf;		/* RST 18h */
}

static struct YM3812interface ym3812_interface = {
	1,			/* 1 chip (no more supported) */
	3250000,	/* 3.25 MHz ? (hand tuned) */
	{ 255 },	/* (not supported) */
	{ 0 },
};

static struct ADPCMinterface adpcm_interface = {
	1,			/* 1 channel */
	8000,		        /* 8000Hz playback */
	5,
	0,			/* init function */
	{ 128,128 }		/* volume */
};

int bloodbro_interrupt(void) { /* 4: 045E */
      return 4;  /* VBlank */
}

static struct MachineDriver bloodbro_machine_driver = {
	{
		{
			CPU_M68000,
			12000000, /* 12 Mhz */
			0,
			readmem_cpu,writemem_cpu,0,0,
		        bloodbro_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ? */
			1,
			readmem_sound,writemem_sound,0,0,
			snd_interrupt,2
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	bloodbro_gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,

	0,
	bloodbro_vh_start,
	bloodbro_vh_stop,
	bloodbro_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

/**** Weststry Interrupt & Driver Machine  ******************************/

int weststry_interrupt(void) {
        return (6);
}

static struct MachineDriver weststry_machine_driver = {
	{
		{
			CPU_M68000,
			10000000, /* 12 Mhz */
			0,
			weststry_readmem_cpu,weststry_writemem_cpu,0,0,
		        weststry_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ? */
			1,
			readmem_sound,writemem_sound,0,0,
			snd_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* CPU slices per frame */
	0, /* init machine */

	/* video hardware */
	256, 256, { 0, 255, 16, 239 },

	weststry_gfxdecodeinfo,
        1024,1024,
        0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,

	0,
        bloodbro_vh_start,
        bloodbro_vh_stop,
	weststry_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0

};

ROM_START( bloodbro_rom )
	ROM_REGION(0x90000)	/* 64k for cpu code */
	ROM_LOAD_ODD ( "bb_02.bin" ,   0x00000, 0x20000, 0xc0fdc3e4 )
	ROM_LOAD_EVEN( "bb_01.bin" ,   0x00000, 0x20000, 0x2d7e0fdf )
	ROM_LOAD_ODD ( "bb_04.bin" ,   0x40000, 0x20000, 0xfd951c2c )
	ROM_LOAD_EVEN( "bb_03.bin" ,   0x40000, 0x20000, 0x18d3c460 )

	ROM_REGION(0x10000)	/* 64k for sound cpu code */
	ROM_LOAD( "bb_07.bin" ,   0x00000, 0x10000, 0x411b94e8 )

	ROM_REGION_DISPOSE(0x20000)	/* characters */
	ROM_LOAD( "bb_05.bin" ,   0x00000, 0x10000, 0x04ba6d19 )
	ROM_LOAD( "bb_06.bin" ,   0x10000, 0x10000, 0x7092e35b )

	ROM_REGION_DISPOSE(0x100000) /* gfx */
	ROM_LOAD( "bloodb.bk",   0x00000, 0x100000, 0x1aa87ee6 )

	ROM_REGION_DISPOSE(0x100000) /* sprites */
	ROM_LOAD( "bloodb.obj",   0x00000, 0x100000, 0xd27c3952 )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "bb_08.bin" ,   0x00000, 0x20000, 0xdeb1b975 )
ROM_END

ROM_START( weststry_rom )
	ROM_REGION(0x90000)	/* 64k for cpu code */
	ROM_LOAD_ODD ( "ws13.bin" ,   0x00000, 0x20000, 0x158e302a )
	ROM_LOAD_EVEN( "ws15.bin" ,   0x00000, 0x20000, 0x672e9027 )
	ROM_LOAD_ODD ( "bb_04.bin" ,   0x40000, 0x20000, 0xfd951c2c )
	ROM_LOAD_EVEN( "bb_03.bin" ,   0x40000, 0x20000, 0x18d3c460 )

	ROM_REGION(0x30000)	/* 64k for sound cpu code */
	ROM_LOAD( "ws17.bin" ,   0x00000, 0x10000, 0xe00a8f09 )

	ROM_REGION_DISPOSE(0x20000)	/* characters */
	ROM_LOAD ( "ws09.bin" ,   0x00000, 0x08000, 0xf05b2b3e )
	ROM_CONTINUE ( 0x00000, 0x8000 )
	ROM_LOAD ( "ws11.bin" ,   0x08000, 0x08000, 0x2b10e3d2 )
	ROM_CONTINUE ( 0x08000, 0x8000 )
	ROM_LOAD ( "ws10.bin" ,   0x10000, 0x08000, 0xefdf7c82 )
	ROM_CONTINUE ( 0x10000, 0x8000 )
	ROM_LOAD ( "ws12.bin" ,   0x18000, 0x08000, 0xaf993578 )
	ROM_CONTINUE ( 0x18000, 0x8000 )

	ROM_REGION_DISPOSE(0x100000) /* gfx */
	/* Background */
	ROM_LOAD ( "ws05.bin" ,   0x00000, 0x20000, 0x007c8dc0 )
	ROM_LOAD ( "ws07.bin" ,   0x20000, 0x20000, 0x0f0c8d9a )
	ROM_LOAD ( "ws06.bin" ,   0x40000, 0x20000, 0x459d075e )
	ROM_LOAD ( "ws08.bin" ,   0x60000, 0x20000, 0x4d6783b3 )

	/* Foreground */
	ROM_LOAD ( "ws01.bin" ,   0x80000, 0x20000, 0x32bda4bc )
	ROM_LOAD ( "ws03.bin" ,   0xa0000, 0x20000, 0x046b51f8 )
	ROM_LOAD ( "ws02.bin" ,   0xc0000, 0x20000, 0xed9d682e )
	ROM_LOAD ( "ws04.bin" ,   0xe0000, 0x20000, 0x75f082e5 )

	ROM_REGION_DISPOSE(0x100000) /* sprites */
	ROM_LOAD ( "ws25.bin" ,   0x00000, 0x20000, 0x8092e8e9 )
	ROM_LOAD ( "ws26.bin" ,   0x20000, 0x20000, 0xf6a1f42c )
	ROM_LOAD ( "ws23.bin" ,   0x40000, 0x20000, 0x43d58e24 )
	ROM_LOAD ( "ws24.bin" ,   0x60000, 0x20000, 0x20a867ea )
	ROM_LOAD ( "ws21.bin" ,   0x80000, 0x20000, 0xe23d7296 )
	ROM_LOAD ( "ws22.bin" ,   0xa0000, 0x20000, 0x7150a060 )
	ROM_LOAD ( "ws19.bin" ,   0xc0000, 0x20000, 0xc5dd0a96 )
	ROM_LOAD ( "ws20.bin" ,   0xe0000, 0x20000, 0xf1245c16 )

	ROM_REGION(0x20000)	/* ADPCM samples */
	ROM_LOAD( "bb_08.bin" ,   0x00000, 0x20000, 0xdeb1b975 )
ROM_END

static void gfx_untangle( void ){
       unsigned char *gfx = Machine->memory_region[4];
       int i;
       for( i=0; i< 0x100000; i++ ){
            gfx[i] = ~gfx[i];
       }
}

struct GameDriver bloodbro_driver =
{
	__FILE__,
	0,
	"bloodbro",
	"Blood Bros.",
	"1990",
	"Tad",
	"Carlos A. Lozano Baides\nRichard Bush\n",
	0,
	&bloodbro_machine_driver,
	0,

	bloodbro_rom,
	0,0,
	0,
	0, /* sound_prom */

	input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver weststry_driver =
{
	__FILE__,
	&bloodbro_driver,
	"weststry",
	"West Story",
	"1990",
	"bootleg",
	"Carlos A. Lozano Baides\nRichard Bush\n",
	0,
	&weststry_machine_driver,
	0,

	weststry_rom,
	gfx_untangle,0,
	0,
	0, /* sound_prom */

	weststry_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,

	0,0
};
