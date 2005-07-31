/***************************************************************************

    HAR MadMax hardware

****************************************************************************

    Known bugs:
        * not done yet!

***************************************************************************

    Double Cheese (c) 1993 HAR

    CPU: 68000
    Sound: 6809, BSMt2000 D15505N
    RAM: 84256 (x2), 5116
    Other: TRW9312HH (x2), LSI L1A6017 (MAX1 EXIT)

    Notes: PCB labeled "Exit Entertainment MADMAX version 5". Title screen reports
    (c)1993 Midway Manufacturing. ROM labels (c) 1993 HAR

**************************************************************************/


#include "driver.h"
#include "machine/eeprom.h"
#include "vidhrdw/generic.h"
#include "sound/bsmt2000.h"


void dcheese_signal_irq(int which);

/* constants */
#define SRCBITMAP_WIDTH		4096

#define DSTBITMAP_WIDTH		512
#define DSTBITMAP_HEIGHT	512

static data16_t blitter_control[0x40];
static data16_t blitter_color[2];

static UINT8 *srcbitmap;
static UINT8 *dstbitmap;
static UINT32 srcbitmap_height_mask;



/*************************************
 *
 *  Palette translation
 *
 *************************************/

PALETTE_INIT( dcheese )
{
	UINT16 *src = (UINT16 *)memory_region(REGION_USER1);
	int i;

//FILE *f = fopen("palette.txt", "w");

	for (i = 0; i < 65534; i++)
	{
		int data = *src++;
		int r = (data >> 0) & 0x3f;
		int b = (data >> 11) & 0x1f;
		int g = (data >> 6) & 0x1f;

//if (i % 16 == 0) fprintf(f, "\n%04X: ", i);
//fprintf(f, "%04X ", data);

		palette_set_color(i, r << 2, g << 3, b << 3);
	}

//fclose(f);

//f = fopen("pix.txt", "w");
//for (i = 0; i < memory_region_length(REGION_GFX1); i++)
//{
//  if (i % 512 == 0) fprintf(f, "\n%05X: ", i / 512);
//  fprintf(f, "%02X", memory_region(REGION_GFX1)[i]);
//  if (i % 8 == 0) fprintf(f, "-");
//}
//fclose(f);
}



/*************************************
 *
 *  Video start
 *
 *************************************/

VIDEO_START( dcheese )
{
	/* the source bitmap is in ROM */
	srcbitmap = memory_region(REGION_GFX1);

	/* compute the height */
	srcbitmap_height_mask = (memory_region_length(REGION_GFX1) / SRCBITMAP_WIDTH) - 1;

	/* the destination bitmap is not directly accessible to the CPU */
	dstbitmap = auto_malloc(DSTBITMAP_WIDTH * DSTBITMAP_HEIGHT);
	if (!dstbitmap)
		return 1;

	return 0;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( dcheese )
{
static int testx, testy;
	int x, y;

if (code_pressed(KEYCODE_UP) && testx < 512 - 1) testx++;
if (code_pressed(KEYCODE_DOWN) && testx > 0) testx--;
if (code_pressed(KEYCODE_LEFT) && testy < 512 - 1) testy++;
if (code_pressed(KEYCODE_RIGHT) && testy > 0) testy--;
if (code_pressed(KEYCODE_A) || code_pressed(KEYCODE_S) || code_pressed(KEYCODE_D))
{
	UINT8 *src = memory_region(REGION_GFX1);
	int which = code_pressed(KEYCODE_A) ? 0 : code_pressed(KEYCODE_S) ? 1 : 2;
	usrintf_showmessage("%d,%X,%X", which, testx, testy);

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = src[which * 0x40000 + ((testx + x) % 512) * 512 + ((testy - y) & 511)];
	}
	return;
}

	logerror("---- UPDATE ----\n");


	/* update the pixels */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];
		UINT8 *src = &dstbitmap[((y + blitter_control[0x20 + 0x28/2]) % DSTBITMAP_HEIGHT) * DSTBITMAP_WIDTH];

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = src[x];
	}
}



/*************************************
 *
 *  Blitter implementation
 *
 *************************************/

static void do_clear(void)
{
	int y;

	/* clear the requested scanlines */
	for (y = blitter_control[0x20+0x2c/2]; y < blitter_control[0x20+0x2a/2]; y++)
		memset(&dstbitmap[(y % DSTBITMAP_HEIGHT) * DSTBITMAP_WIDTH], 0, DSTBITMAP_WIDTH);

	timer_set(cpu_getscanlineperiod(), 1, dcheese_signal_irq);
}


static void do_blit(void)
{
	INT32 srcminx = blitter_control[0x00] << 12;
	INT32 srcmaxx = blitter_control[0x01] << 12;
	INT32 srcminy = blitter_control[0x10] << 12;
	INT32 srcmaxy = blitter_control[0x11] << 12;
	INT32 srcx = ((blitter_control[0x02] & 0x0fff) | ((blitter_control[0x03] & 0x0fff) << 12)) << 7;
	INT32 srcy = ((blitter_control[0x12] & 0x0fff) | ((blitter_control[0x13] & 0x0fff) << 12)) << 7;
	INT32 dxdx = (INT32)(((blitter_control[0x04] & 0x0fff) | ((blitter_control[0x05] & 0x0fff) << 12)) << 12) >> 12;
	INT32 dxdy = (INT32)(((blitter_control[0x06] & 0x0fff) | ((blitter_control[0x07] & 0x0fff) << 12)) << 12) >> 12;
	INT32 dydx = (INT32)(((blitter_control[0x14] & 0x0fff) | ((blitter_control[0x15] & 0x0fff) << 12)) << 12) >> 12;
	INT32 dydy = (INT32)(((blitter_control[0x16] & 0x0fff) | ((blitter_control[0x17] & 0x0fff) << 12)) << 12) >> 12;
	UINT8 *src = memory_region(REGION_GFX1);
	int xstart = blitter_control[0x0e];
	int xend = blitter_control[0x0f] + 1;
	int ystart = blitter_control[0x1e];
	int yend = blitter_control[0x1f];
	int color = (blitter_color[0] << 8);
	int mask = (blitter_color[0] >> 8) & 0xff;
	int x, y, page = -1;

	if ((srcx & 0x00600000) == 0x00400000 && (srcy & 0x00600000) == 0x00200000) page = 0;
	if ((srcx & 0x00600000) == 0x00400000 && (srcy & 0x00600000) == 0x00600000) page = 1;
	if ((srcx & 0x00600000) == 0x00600000) page = 2;

	if (page != -1)
		src += 0x40000 * page;
//  else
//      printf("srcx,y = %08X, %08X\n", srcx, srcy);

	logerror("srcx=%08X srcy=%08X dxdx=%08X dxdy=%08X dydx=%08X dydy=%08X\n",
		srcx, srcy, dxdx, dxdy, dydx, dydy);

	for (y = ystart; y <= yend; y++)
	{
		UINT8 *dst = &dstbitmap[(y % DSTBITMAP_HEIGHT) * DSTBITMAP_WIDTH];

		if (page != -1)
		{
			for (x = xstart; x <= xend; x++)
			{
				int sx = (srcx + dxdx * (x - xstart) + dxdy * (y - ystart)) & 0xffffff;
				int sy = (srcy + dydx * (x - xstart) + dydy * (y - ystart)) & 0xffffff;
				if (sx >= srcminx && sx <= srcmaxx && sy >= srcminy && sy <= srcmaxy)
				{
					int pix = src[((sy >> 12) & 0x1ff) * 512 + ((sx >> 12) & 0x1ff)];
					if (pix)
						dst[x % DSTBITMAP_WIDTH] = (pix & mask) | color;
				}
			}
		}
		else
		{
			if (y == ystart || y == yend)
			{
				for (x = xstart; x <= xend; x++)
					dst[x % DSTBITMAP_WIDTH] = 0xff;
			}
			else
			{
				dst[xstart % DSTBITMAP_WIDTH] = 0xff;
				dst[xend % DSTBITMAP_WIDTH] = 0xff;
			}
		}
	}

	timer_set(cpu_getscanlineperiod(), 2, dcheese_signal_irq);
}



/*************************************
 *
 *  Blitter read/write
 *
 *************************************/

static READ16_HANDLER( _260000_r )
{
	logerror("%06X:read from %06X\n", activecpu_get_pc(), 0x260000 + 2 * offset);
	return 0xffff;
}

static READ16_HANDLER( _280000_r )
{
	logerror("%06X:read from %06X\n", activecpu_get_pc(), 0x280000 + 2 * offset);
	return 0xffff;
}

static READ16_HANDLER( _2a0000_r )
{
	logerror("%06X:read from %06X\n", activecpu_get_pc(), 0x2a0000 + 2 * offset);
	if (offset * 2 == 0x36)
		return 0xffff ^ (1 << 5);
	return 0xffff;
}

static WRITE16_HANDLER( _300000_w )
{
	// written to just before the blitter command register is written
//  logerror("%06X:write to %06X = %04X & %04X\n", activecpu_get_pc(), 0x300000 + 2 * offset, data, mem_mask ^ 0xffff);
}

static WRITE16_HANDLER( _220000_w )
{
	COMBINE_DATA(&blitter_color[offset]);
//  logerror("%06X:write to %06X = %04X & %04X\n", activecpu_get_pc(), 0x220000 + 2 * offset, data, mem_mask ^ 0xffff);
}

static WRITE16_HANDLER( _260000_w )
{
	COMBINE_DATA(&blitter_control[0x00 + offset]);
//  logerror("%06X:write to %06X = %04X & %04X\n", activecpu_get_pc(), 0x260000 + 2 * offset, data, mem_mask ^ 0xffff);
}

static WRITE16_HANDLER( _280000_w )
{
	COMBINE_DATA(&blitter_control[0x10 + offset]);
//  logerror("%06X:write to %06X = %04X & %04X\n", activecpu_get_pc(), 0x280000 + 2 * offset, data, mem_mask ^ 0xffff);
}

static WRITE16_HANDLER( _2a0000_w )
{
	COMBINE_DATA(&blitter_control[0x20 + offset]);
	if (offset == 0x28/2)
	{
		logerror("%06X:pageswap to %04X!\n", activecpu_get_pc(), blitter_control[0x20 + offset]);
	}
	else if (offset == 0x38/2)
	{
		do_blit();
		logerror("%06X:blit! (%04X)\n", activecpu_get_pc(), blitter_color[0]);
		logerror("   %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X\n",
				blitter_control[0x00], blitter_control[0x01], blitter_control[0x02], blitter_control[0x03],
				blitter_control[0x04], blitter_control[0x05], blitter_control[0x06], blitter_control[0x07],
				blitter_control[0x08], blitter_control[0x09], blitter_control[0x0a], blitter_control[0x0b],
				blitter_control[0x0c], blitter_control[0x0d], blitter_control[0x0e], blitter_control[0x0f]);
		logerror("   %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X - %04X %04X %04X %04X\n",
				blitter_control[0x10], blitter_control[0x11], blitter_control[0x12], blitter_control[0x13],
				blitter_control[0x14], blitter_control[0x15], blitter_control[0x16], blitter_control[0x17],
				blitter_control[0x18], blitter_control[0x19], blitter_control[0x1a], blitter_control[0x1b],
				blitter_control[0x1c], blitter_control[0x1d], blitter_control[0x1e], blitter_control[0x1f]);
	}
	else if (offset == 0x3e/2)
	{
		do_clear();
		logerror("%06X:clear!\n", activecpu_get_pc());
		logerror("   %04X-%04X\n", blitter_control[0x20+0x2c/2], blitter_control[0x20+0x2a/2]);
	}
	else
		logerror("%06X:write to %06X = %04X & %04x\n", activecpu_get_pc(), 0x2a0000 + 2 * offset, data, mem_mask ^ 0xffff);
}



/*************************************
 *
 *  Interrupts
 *
 *************************************/

static UINT8 irq_state[5];

static void update_irq_state(void)
{
	int i;

	/* loop from high priority to low; if we find a live IRQ, assert it */
	for (i = 4; i >= 0; i--)
		if (irq_state[i])
		{
			cpunum_set_input_line(0, i, ASSERT_LINE);
			return;
		}

	/* otherwise, clear them all */
	cpunum_set_input_line(0, 7, CLEAR_LINE);
}

static int irq_callback(int which)
{
	/* auto-ack the IRQ */
	irq_state[which] = 0;
	update_irq_state();

	/* vector is 0x40 + index */
	return 0x40 + which;
}

static INTERRUPT_GEN( dcheese_vblank )
{
	logerror("---- VBLANK ----\n");
	dcheese_signal_irq(4);
}


void dcheese_signal_irq(int which)
{
	irq_state[which] = 1;
	update_irq_state();
}



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_INIT( dcheese )
{
	EEPROM_init(&eeprom_interface_93C46);
	cpu_set_irq_callback(0, irq_callback);
}



/*************************************
 *
 *  I/O ports
 *
 *************************************/

static READ16_HANDLER( port_0_r )
{
	return (readinputport(0) & 0xff7f) | (EEPROM_read_bit() << 7);
}


static WRITE16_HANDLER( eeprom_control_w )
{
	if (ACCESSING_LSB)
	{
		EEPROM_set_cs_line(data & 1);	// ?
		EEPROM_write_bit(data & 2);
		EEPROM_set_clock_line(data & 4);
	}
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_cpu_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )

	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x200000, 0x200003) AM_READWRITE(port_0_r, watchdog_reset16_w)
	AM_RANGE(0x220000, 0x220003) AM_READWRITE(input_port_1_word_r, _220000_w)
	AM_RANGE(0x240000, 0x240003) AM_READ(input_port_2_word_r)
	AM_RANGE(0x240000, 0x240003) AM_WRITE(eeprom_control_w)
	AM_RANGE(0x260000, 0x26001f) AM_READWRITE(_260000_r, _260000_w)
	AM_RANGE(0x280000, 0x28001f) AM_READWRITE(_280000_r, _280000_w)
	AM_RANGE(0x2a0000, 0x2a003f) AM_READWRITE(_2a0000_r, _2a0000_w)
	AM_RANGE(0x300000, 0x300003) AM_WRITE(_300000_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )

	AM_RANGE(0x0100, 0x0100) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1002, 0x1002) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1003, 0x1003) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x1fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Input port definitions
 *
 *************************************/

INPUT_PORTS_START( dcheese )
	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct BSMT2000interface bsmt2000_interface =
{
	12,
	REGION_SOUND1
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( dcheese )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(main_cpu_map,0)
	MDRV_CPU_VBLANK_INT(dcheese_vblank,1)

	MDRV_CPU_ADD(M6809, 1250000)
	MDRV_CPU_PROGRAM_MAP(sound_cpu_map,0)

	MDRV_MACHINE_INIT(dcheese)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 256)
	MDRV_VISIBLE_AREA(0, 383, 0, 255)

	MDRV_PALETTE_LENGTH(65534)

	MDRV_PALETTE_INIT(dcheese)
	MDRV_VIDEO_START(dcheese)
	MDRV_VIDEO_UPDATE(dcheese)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(BSMT2000, 12000000/2)
	MDRV_SOUND_CONFIG(bsmt2000_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( dcheese )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68k */
	ROM_LOAD16_BYTE( "dchez.104", 0x00000, 0x20000, CRC(5b6233d8) SHA1(7fdb606b5780dd8f45db07d3ee50e14a27f39533) )
	ROM_LOAD16_BYTE( "dchez.103", 0x00001, 0x20000, CRC(599c73ff) SHA1(f33e617ab7e9489c52b2434cfc61a5e1696e9400) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* M6809 */
	ROM_LOAD( "dchez.102", 0x8000, 0x8000, CRC(5d110061) SHA1(10d852a408a75979b8e8843afc7b39737ca2c6c8) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "dchez.123", 0x00000, 0x40000, CRC(2293dd9a) SHA1(3f0550c2a6f59a233c5b1010cecdb19404170dc0) )
	ROM_LOAD( "dchez.125", 0x40000, 0x40000, CRC(ddf28bab) SHA1(0f3bc86d0db7afebf8c6094b8337e5f343a82f29) )
	ROM_LOAD( "dchez.127", 0x80000, 0x40000, CRC(372f9d67) SHA1(74f73f0344bfb890b5e457fcde3d82c9106e7edd) )

	ROM_REGION( 0xc0000, REGION_SOUND1, 0 )
	ROM_LOAD( "dchez.ar0", 0x00000, 0x40000, CRC(6a9e2b12) SHA1(f7cb4d6b4a459682a68f734b2b2e27e3639b9ed5) )
	ROM_LOAD( "dchez.ar1", 0x40000, 0x40000, CRC(5f3a5f41) SHA1(30e0c7b2ab43a3224432204a9388d509a6a06a11) )
	ROM_LOAD( "dchez.ar2", 0x80000, 0x20000, CRC(d79b0d41) SHA1(cc84ddf6635097ba0aad2f1838ad0606c5bb8166) )
	ROM_LOAD( "dchez.ar3", 0xa0000, 0x20000, CRC(2056c1fd) SHA1(4c44930fb87ea6ad71326cc29313f3b817919d08) )

	ROM_REGION16_LE( 0x20000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "dchez.144", 0x00000, 0x10000, CRC(52c96252) SHA1(46de465c25e4602aa360336315b3c8e1a9a0b5f3) )
	ROM_LOAD16_BYTE( "dchez.145", 0x00001, 0x10000, CRC(a11b92d0) SHA1(265f93cb3657910aabca21ed8afbb55bdc86a964) )
ROM_END



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAMEX( 1993, dcheese, 0, dcheese, dcheese, 0, ROT90, "HAR", "Double Cheese", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS )
