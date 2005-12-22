/* Little Robin */

/* driver by
Pierpaolo Prazzoli
David Haywood
*/

/* little robin uses some kind of vdp device
  gfx data (and other data, spriteram content etc.?) is written to offset 3
*/

#include "driver.h"
#include "vidhrdw/generic.h"

static UINT16 *blitter_regs;
static mame_bitmap *littlerb_bitmap;

static READ16_HANDLER( blitter_r )
{
	/* not understood either */

	if(offset == 2)
		return 0;
	else
		return blitter_regs[offset];
}

static int littlerb_pos = 0;

//static FILE *fp_0, *fp_1, *fp_2, *fp_3, *fp_4;

//static int count[4] = {0,0,0,0};
//static int old_offset = -1;

//static int pal_pos=0;

/*

offset 3:
    - 0x0000
    - 0x2000
    - 0x3800
    - 0xe000 -> only twice?
    - 0xf800 -> only once?


*/

static WRITE16_HANDLER( blitter_w )
{
	/* messy.. this is just a record of what it writes to the command ports.. */

	if (offset==3)  // some kind of command / mode?
	{
	//  if(
	//      (data!=0x0000) &&
	//      (data!=0x2000) &&
	//      (data!=0x3800)
	//      )
			//  printf("offset3 %06x, %04x\n",activecpu_get_pc(), data);
	}

	if (offset==0)
	{
		if( // lets see what it writes here too..  some kind of offsets?
			(data!=0x0000) &&
			(data!=0x0010) &&
			(data!=0x0020) &&
			(data!=0x0030) &&
			(data!=0x0040) &&
			(data!=0x0050) &&
			(data!=0x0060) &&
			(data!=0x0070) &&
			(data!=0x0080) &&
			(data!=0x0090) &&
			(data!=0x00a0) && // often
			(data!=0x00b0) &&
			(data!=0x0150) &&
			(data!=0x0160) &&
			(data!=0x01b0) &&
			(data!=0x01e0) &&
			(data!=0x0740) &&
			(data!=0x1cc0) &&
			(data!=0x2180) &&
			(data!=0x2900) &&
			(data!=0x3140) &&
			(data!=0x3400) &&
			(data!=0x36e0) &&
			(data!=0x3bc0) &&
			(data!=0x4000) &&
			(data!=0x4010) &&
			(data!=0x4020) && // often
			(data!=0x4030) && // often
			(data!=0x44c0) &&
			(data!=0x4c80) &&
			(data!=0x5120) &&
			(data!=0x52c0) &&
			(data!=0x54c0) &&
			(data!=0x5c00) &&
			(data!=0x5e00) &&
			(data!=0x6000) &&
			(data!=0x65a0) &&
			(data!=0x7880) &&
			(data!=0x7d40) &&
			(data!=0x8480) &&
			(data!=0x8ba0) &&
			(data!=0x9100) &&
			(data!=0x9680) &&
			(data!=0x9a00) &&
			(data!=0xa480) &&
			(data!=0xa600) &&
			(data!=0xaa40) &&
			(data!=0xaac0) &&
			(data!=0xb300) &&
			(data!=0xbbc0) &&
			(data!=0xde00) &&
			(data!=0xdec0) &&
			(data!=0xe840) &&
			(data!=0xebc0) &&
			(data!=0xed00) &&
			(data!=0xf300) &&
			(data!=0xf580) &&
			(data!=0xfcc0) &&
			(data!=0xffe0)
			)
		printf("offset0 %06x, %04x\n",activecpu_get_pc(), data);
	}

	if (offset==1)
	{
		if( // i don't know if these are somehow related to the upload width? probably not.. but i'm just checking to see how many values it writes for now
			(data!=0x0000) &&

			(data!=0x0038) &&
			(data!=0x0039) &&
			(data!=0x003a) &&
			(data!=0x003b) &&
			(data!=0x003c) &&

			(data!=0x0400) &&
			(data!=0xc000) &&

			(data!=0xffc0) &&
			(data!=0xffc1) &&  // uploads some font gfx 16 pixels wide?  -- not if test mode is on, some other gfx?

			(data!=0xffc2) &&
			(data!=0xffc4) &&
			(data!=0xffc6) &&
			(data!=0xffc8) &&
			(data!=0xffc9) &&

			(data!=0xffcc) &&
			(data!=0xffcd) &&
			(data!=0xffce) &&
			(data!=0xffcf) &&


			(data!=0xffd0) &&
			(data!=0xffd1) &&  // uploads some gfx 32 pixels wide?

			(data!=0xffd4) &&

			(data!=0xffd7) &&
			(data!=0xffd8) &&
			(data!=0xffd9) &&

			(data!=0xffda) &&
			(data!=0xffdc) &&
			(data!=0xffdd) &&

			(data!=0xffe0) &&
			(data!=0xffe1) &&  // uploads some gfx 96 pixels wide?.. hmm

			(data!=0xffe4) &&
			(data!=0xffe6) &&
			(data!=0xffe8) &&

			(data!=0xffff)
			)
			printf("offset1 %06x, %04x\n",activecpu_get_pc(), data);

	}

	COMBINE_DATA(&blitter_regs[offset]);

//  if(offset == 2 && blitter_regs[1] == 0xffe1)
//  if(offset == 2 && blitter_regs[1] == 0xffd1)
	if(offset == 2 && blitter_regs[1] == 0xffc1)
	{
		//printf("data_write %04x\n",data);

		int x,y;

		x = littlerb_pos % 8; // game mode, font?
		y = (littlerb_pos&0x7ff) / 8;
		x += ((littlerb_pos &0xff00)>>11)*16;

		// ffd1 -- clouds??  -- ffc1 test mode, player 1, on, off etc.?
//      x = littlerb_pos % 16;
//      y = (littlerb_pos&0xfff) / 16;
//      x += ((littlerb_pos &0xff00)>>12)*24;

		// ffe1
//      x = littlerb_pos % 48;
//      y = (littlerb_pos&0x3fff) / 48;
//      x += ((littlerb_pos &0xff00)>>12)*64;

		littlerb_pos++;
		littlerb_pos &= 0x3fff;

		plot_pixel(littlerb_bitmap,(x*2+0)&0xff,(y&0xff),Machine->pens[data & 0xff]);
		plot_pixel(littlerb_bitmap,(x*2+1)&0xff,(y&0xff),Machine->pens[(data>>8) & 0xff]);



	}




}

VIDEO_START(littlerb)
{
	littlerb_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
#if 0
	fp_0 = fopen("log_0000.txt","w+");
	fp_1 = fopen("log_2000.txt","w+");
	fp_2 = fopen("log_3800.txt","w+");
	fp_3 = fopen("log_E000.txt","w+");
	fp_4 = fopen("log_F800.txt","w+");
#endif
	return 0;
}

VIDEO_UPDATE(littlerb)
{
#if 0
if(code_pressed_memory(KEYCODE_Q))
{
fclose(fp_0);
fclose(fp_1);
fclose(fp_2);
fclose(fp_3);
fclose(fp_4);
}
#endif

	copybitmap(bitmap, littlerb_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
}


static ADDRESS_MAP_START( littlerb_main, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000008, 0x000017) AM_WRITENOP
	AM_RANGE(0x000020, 0x00002f) AM_WRITENOP
	AM_RANGE(0x000070, 0x000073) AM_WRITENOP
	AM_RANGE(0x060004, 0x060007) AM_WRITENOP

	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x200000, 0x203fff) AM_RAM

	AM_RANGE(0x7c0000, 0x7c0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x7e0000, 0x7e0001) AM_READ(input_port_1_word_r)
	AM_RANGE(0x7e0002, 0x7e0003) AM_READ(input_port_2_word_r)


	AM_RANGE(0x700000, 0x700007) AM_WRITE(blitter_w) AM_READ(blitter_r) AM_BASE(&blitter_regs)

//  AM_RANGE(0x740000, 0x740001) AM_WRITE(MWA16_NOP)
//  AM_RANGE(0x760000, 0x760001) AM_WRITE(MWA16_NOP)
//  AM_RANGE(0x740000, 0x740001) AM_WRITE(MWA16_NOP)
//  AM_RANGE(0x7a0000, 0x7a0001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x780000, 0x780001) AM_WRITE(MWA16_NOP)

ADDRESS_MAP_END


INPUT_PORTS_START( littlerb )
	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "GAME/TEST??" ) // changes what gets uploaded
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x1000, 0x1000, "???"  )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

PALETTE_INIT( littlerb )
{
	int i;
	for(i = 0; i < 256; i++)
		palette_set_color(i,i,i,i);
}

static MACHINE_DRIVER_START( littlerb )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(littlerb_main, 0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_PALETTE_LENGTH(256)

//  MDRV_PALETTE_INIT(littlerb)
	MDRV_VIDEO_START(littlerb)
	MDRV_VIDEO_UPDATE(littlerb)
MACHINE_DRIVER_END


ROM_START( littlerb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "little robin_a_27c040.bin", 0x00001, 0x80000, CRC(172fbc13) SHA1(cd165ca0d0546e2634cf182dc98004cbfb02cf9f) )
	ROM_LOAD16_BYTE( "little robin_b_27c040.bin", 0x00000, 0x80000, CRC(b2fb1d61) SHA1(9a9d7176c241928d07af651e5f7f21d4f019701d) )

		ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD16_BYTE( "little robin_c_27c020.bin", 0x00001, 0x40000, CRC(f193c5b6) SHA1(95548a40e2b5064c558b36cabbf507d23678b1b2) )
	ROM_LOAD16_BYTE( "little robin_d_27c020.bin", 0x00000, 0x40000, CRC(d6b81583) SHA1(b7a63d18a41ccac4d3db9211de0b0cdbc914317a) )
ROM_END


GAME( 1993, littlerb, 0, littlerb, littlerb, 0, ROT0, "TCH", "Little Robin", GAME_NOT_WORKING|GAME_NO_SOUND )
