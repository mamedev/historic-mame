/*  Konami GTI Club System

    Driver by Ville Linde
*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/powerpc/ppc.h"
#include "cpu/sharc/sharc.h"
#include "machine/konppc.h"

static UINT8 led_reg0 = 0x7f, led_reg1 = 0x7f;

int K001604_vh_start(void);
void K001604_tile_update(void);
void K001604_tile_draw(mame_bitmap *bitmap, const rectangle *cliprect);
READ32_HANDLER(K001604_tile_r);
READ32_HANDLER(K001604_char_r);
WRITE32_HANDLER(K001604_tile_w);
WRITE32_HANDLER(K001604_char_w);
WRITE32_HANDLER(K001604_reg_w);
READ32_HANDLER(K001604_reg_r);


static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	b = ((data >> 0) & 0x1f);
	g = ((data >> 5) & 0x1f);
	r = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);

	palette_set_color(offset, r, g, b);
}


static UINT16 texture_ram0[0x100000];
static UINT16 texture_ram1[0x100000];
static int texture_ram0_ptr = 0;
static int texture_ram0_page = 0;
static int texture_ram1_ptr = 0;
static int texture_ram1_page = 0;

static READ32_HANDLER(texture0_r)
{
//  logerror("texture0_r: %08X, %08X\n", offset, mem_mask);
	if (offset == 1) {
		return texture_ram0[(texture_ram0_page * 0x10000) + texture_ram0_ptr++];
	}
	return 0;
}

static WRITE32_HANDLER(texture0_w)
{
//  logerror("texture0_w: %08X, %08X, %08X\n", data, offset, mem_mask);
	if (offset == 0) {
		COMBINE_DATA(&texture_ram0_ptr);
	}
	else if (offset == 1) {
		texture_ram0[(texture_ram0_page * 0x10000) + texture_ram0_ptr++] = data & 0xffff;
	}
	else if (offset == 2) {
		if (!(mem_mask & 0xffff0000)) {
			texture_ram0_page = (data >> 16) & 0xf;
		}
	}
}

static READ32_HANDLER(texture1_r)
{
	if (offset == 1) {
		return texture_ram1[(texture_ram1_page * 0x10000) + texture_ram1_ptr++];
	}
	return 0;
}

static WRITE32_HANDLER(texture1_w)
{
	if (offset == 0) {
		COMBINE_DATA(&texture_ram1_ptr);
	}
	else if (offset == 1) {
		texture_ram1[(texture_ram1_page * 0x10000) + texture_ram1_ptr++] = data & 0xffff;
	}
	else if (offset == 2) {
		if (!(mem_mask & 0xffff0000)) {
			texture_ram1_page = (data >> 16) & 0xf;
		}
	}
}




VIDEO_START( gticlub )
{
	return K001604_vh_start();
}

VIDEO_UPDATE( gticlub )
{
	fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	K001604_tile_update();
	K001604_tile_draw(bitmap, cliprect);

	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
}

/******************************************************************/

/* 93C56 EEPROM */
static struct EEPROM_interface eeprom_interface =
{
	8,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxxxx",	/* lock command */
	"*10011xxxxxx",	/* unlock command */
	1,				/* enable_multi_read */
	0				/* reset_delay */
};

static void eeprom_handler(mame_file *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file)
		{
			EEPROM_load(file);
		}
	}
}

//UINT32 eeprom_bit = 0;
static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	if (offset == 0)
	{
		if (!(mem_mask & 0xff000000))
		{
			r |= readinputport(0) << 24;
		}
		if (!(mem_mask & 0x00ff0000))
		{
			r |= readinputport(1) << 16;
		}
		if (!(mem_mask & 0x0000ff00))
		{
			r |= readinputport(2) << 8;
		}
		if (!(mem_mask & 0x000000ff))
		{
			r |= readinputport(3) << 0;
		}
	}
	else if (offset == 1) {
		if (!(mem_mask & 0xff000000) )
		{
			UINT32 eeprom_bit = (EEPROM_read_bit() << 1);
			r |= eeprom_bit << 24;
		}
		return r;
	}
	return r;
}

static WRITE32_HANDLER( sysreg_w )
{
	if (offset == 0) {
		if( !(mem_mask & 0xff000000) )
		{
			led_reg0 = (data >> 24) & 0xff;
		}
		if( !(mem_mask & 0x00ff0000) )
		{
			led_reg1 = (data >> 16) & 0xff;
		}
		if( !(mem_mask & 0x000000ff) )
		{
			EEPROM_write_bit((data & 0x01) ? 1 : 0);
			EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
		}
	}
	if (offset == 1)
		return;
}

static UINT8 sndto68k[16], sndtoppc[16];	/* read/write split mapping */

static READ32_HANDLER( ppc_sound_r )
{
	UINT32 reg, w[4], rv = 0;

	reg = offset * 4;

	if (!(mem_mask & 0xff000000))
	{
		w[0] = sndtoppc[reg];
		if (reg == 2) w[0] &= ~3; // supress VOLWR busy flags
		rv |= w[0]<<24;
	}

	if (!(mem_mask & 0x00ff0000))
	{
		w[1] = sndtoppc[reg+1];
		rv |= w[1]<<16;
	}

	if (!(mem_mask & 0x0000ff00))
	{
		w[2] = sndtoppc[reg+2];
		rv |= w[2]<<8;
	}

	if (!(mem_mask & 0x000000ff))
	{
		w[3] = sndtoppc[reg+3];
		rv |= w[3]<<0;
	}

	return(rv);
}

INLINE void write_snd_ppc(int reg, int val)
{
	sndto68k[reg] = val;

	if (reg == 7)
	{
		cpunum_set_input_line(1, 1, HOLD_LINE);
	}
}

static WRITE32_HANDLER( ppc_sound_w )
{
	int reg=0, val=0;

	if (!(mem_mask & 0xff000000))
	{
		reg = offset * 4;
		val = data >> 24;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x00ff0000))
	{
		reg = (offset * 4) + 1;
		val = (data >> 16) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x0000ff00))
	{
		reg = (offset * 4) + 2;
		val = (data >> 8) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x000000ff))
	{
		reg = (offset * 4) + 3;
		val = (data >> 0) & 0xff;
		write_snd_ppc(reg, val);
	}
}

/******************************************************************/

static ADDRESS_MAP_START( gticlub_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x74000000, 0x740000ff) AM_READWRITE(K001604_reg_r, K001604_reg_w)
	AM_RANGE(0x74010000, 0x7401ffff) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74020000, 0x7403ffff) AM_READWRITE(K001604_tile_r, K001604_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_READWRITE(K001604_char_r, K001604_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x78040000, 0x7804000f) AM_READWRITE(texture0_r, texture0_w)
	AM_RANGE(0x78080000, 0x7808000f) AM_READWRITE(texture1_r, texture1_w)
	AM_RANGE(0x780c0000, 0x780c0003) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x7e00a000, 0x7e00bfff) AM_RAM
	AM_RANGE(0x7e00c000, 0x7e00c007) AM_WRITE(ppc_sound_w)
	AM_RANGE(0x7e00c008, 0x7e00c00f) AM_READ(ppc_sound_r)
	AM_RANGE(0x80000000, 0x800fffff) AM_RAM	AM_SHARE(3)	/* Work RAM */
	AM_RANGE(0xff000000, 0xff3fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0xff800000, 0xff9fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END



/**********************************************************************/

static READ16_HANDLER( sndcomm68k_r )
{
	return sndto68k[offset];
}

static WRITE16_HANDLER( sndcomm68k_w )
{
//  logerror("68K: write %x to %x\n", data, offset);
	sndtoppc[offset] = data;
}

static ADDRESS_MAP_START( sound_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM		/* Work RAM */
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x400000, 0x40000f) AM_WRITE(sndcomm68k_w)
	AM_RANGE(0x400010, 0x40001f) AM_READ(sndcomm68k_r)
	AM_RANGE(0x400020, 0x4fffff) AM_RAM
	AM_RANGE(0x580000, 0x580001) AM_WRITENOP
ADDRESS_MAP_END

/*****************************************************************************/

static UINT32 *sharc_dataram;

static READ32_HANDLER( dsp_dataram_r )
{
	return sharc_dataram[offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram_w )
{
	sharc_dataram[offset] = data;
}

/* Konami K001005 Custom 3D Pixel Renderer chip (KS10071) */
UINT32 K001005_ram[2][0x140000];
int K001005_ram_ptr = 0;
UINT32 K001005_fifo[0x800];
int K001005_fifo_read_ptr = 0;
int K001005_fifo_write_ptr = 0;

READ32_HANDLER( K001005_r )
{
	switch(offset)
	{
		case 0x000:			// FIFO read, high 16 bits
		{
			UINT16 value = K001005_fifo[K001005_fifo_read_ptr] >> 16;
		//  printf("FIFO_r0: %08X\n", K001005_fifo_ptr);
			return value;
		}

		case 0x001:			// FIFO read, low 16 bits
		{
			UINT16 value = K001005_fifo[K001005_fifo_read_ptr] & 0xffff;
		//  printf("FIFO_r1: %08X\n", K001005_fifo_ptr);

			if (K001005_fifo_read_ptr < 0x3ff)
			{
				cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);
			}
			else
			{
				cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
			}

			K001005_fifo_read_ptr++;
			K001005_fifo_read_ptr &= 0x7ff;
			return value;
		}

		case 0x11b:			// status ?
			return 0x8000;

		case 0x11c:			// slave status ?
			return 0x8000;

		case 0x11f:
			if (K001005_ram_ptr >= 0x400000)
			{
				return K001005_ram[1][(K001005_ram_ptr++) & 0x3fffff];
			}
			else
			{
				return K001005_ram[0][(K001005_ram_ptr++) & 0x3fffff];
			}

		default:
			printf("K001005_r: %08X, %08X\n", offset, mem_mask);
			break;
	}
	return 0;
}

WRITE32_HANDLER( K001005_w )
{
	switch(offset)
	{
		case 0x000:			// FIFO write
		{
			if (K001005_fifo_write_ptr < 0x400)
			{
				cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
			}
			else
			{
				cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);
			}
			K001005_fifo[K001005_fifo_write_ptr] = data;
			K001005_fifo_write_ptr++;
			K001005_fifo_write_ptr &= 0x7ff;

			break;
		}

		case 0x11d:
			K001005_fifo_write_ptr = 0;
			K001005_fifo_read_ptr = 0;
			break;

		case 0x11e:
			K001005_ram_ptr = data;
			break;

		case 0x11f:
			if (K001005_ram_ptr >= 0x400000)
			{
				K001005_ram[1][(K001005_ram_ptr++) & 0x3fffff] = data & 0xffff;
			}
			else
			{
				K001005_ram[0][(K001005_ram_ptr++) & 0x3fffff] = data & 0xffff;
			}
			break;

		default:
			printf("K001005_w: %08X, %08X, %08X\n", data, offset, mem_mask);
			break;
	}

}

static ADDRESS_MAP_START( sharc_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x400000, 0x41ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x500000, 0x5fffff) AM_READWRITE(dsp_dataram_r, dsp_dataram_w)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(K001005_r, K001005_w)
	AM_RANGE(0x700000, 0x7000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
ADDRESS_MAP_END

/*****************************************************************************/

static NVRAM_HANDLER(gticlub)
{
	eeprom_handler(file, read_or_write);
}


INPUT_PORTS_START( gticlub )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Button") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x08, 0x00, "DIP3" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIP2" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP1" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "DIP0" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x01, DEF_STR( On ) )
INPUT_PORTS_END

/* PowerPC interrupts

    IRQ0:  Vblank
    IRQ2:  LANC
    DMA0

*/
static INTERRUPT_GEN( gticlub_vblank )
{
	cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
}


static ppc_config gticlub_ppc_cfg =
{
	PPC_MODEL_403GA
};

static sharc_config sharc_cfg =
{
	BOOT_MODE_EPROM
};

static MACHINE_INIT( gticlub )
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_DRIVER_START( gticlub )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(gticlub_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(gticlub_map, 0)
	MDRV_CPU_VBLANK_INT(gticlub_vblank, 1)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(gticlub)
	MDRV_MACHINE_INIT(gticlub)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(gticlub)
	MDRV_VIDEO_UPDATE(gticlub)
MACHINE_DRIVER_END

/*************************************************************************/

ROM_START(gticlub)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688aaa01.21u", 0x000003, 0x80000, CRC(06a56474) SHA1(3a457b885a35e3ee030fd51d847bcf75fce46208))
	ROM_LOAD32_BYTE("688aaa02.19u", 0x000002, 0x80000, CRC(3c1e714a) SHA1(557f8542b855b2b35f242c8db7396017aca6dbd8))
	ROM_LOAD32_BYTE("688aaa03.21r", 0x000001, 0x80000, CRC(e060580b) SHA1(50242f3f3b949cc03082e4e75d9dcc89e17f0a75))
	ROM_LOAD32_BYTE("688aaa04.19r", 0x000000, 0x80000, CRC(928c23cd) SHA1(cce54398e1e5b98bfb717839cc422f1f60502788))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d))
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423))

	ROM_REGION(0x40000, REGION_CPU2, 0)	/* 68k program */
        ROM_LOAD16_WORD_SWAP( "688a07.13k",   0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x1000000, REGION_USER3, 0)	/* other roms */
        ROM_LOAD( "688a09.9s",    0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
        ROM_LOAD( "688a10.7s",    0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
        ROM_LOAD( "688a11.5s",    0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
        ROM_LOAD( "688a12.2s",    0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )
        ROM_LOAD( "688a13.18d",   0x800000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
        ROM_LOAD( "688a14.13d",   0xa00000, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
        ROM_LOAD( "688a15.9d",    0xc00000, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
        ROM_LOAD( "688a16.4d",    0xe00000, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )
ROM_END

ROM_START(gticlubj)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688jaa01.bin", 0x000003, 0x80000, CRC(1492059c) SHA1(176dbd87f23f4cd8e1397e67da501738e20e5a57))
	ROM_LOAD32_BYTE("688jaa02.bin", 0x000002, 0x80000, CRC(7896dd69) SHA1(a3ab7b872132a5e66238e414f4b497cf7beb8b1c))
	ROM_LOAD32_BYTE("688jaa03.bin", 0x000001, 0x80000, CRC(94e2be50) SHA1(f206ac201903f3aae29196ab6fccdef104859346))
	ROM_LOAD32_BYTE("688jaa04.bin", 0x000000, 0x80000, CRC(ff539bb6) SHA1(1a225eca4377d82a2b6cb99c1d16580b9ccf2f08))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d))
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423))

	ROM_REGION(0x40000, REGION_CPU2, 0)	/* 68k program */
        ROM_LOAD16_WORD_SWAP( "688a07.13k",   0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x1000000, REGION_USER3, 0)	/* other roms */
        ROM_LOAD( "688a09.9s",    0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
        ROM_LOAD( "688a10.7s",    0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
        ROM_LOAD( "688a11.5s",    0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
        ROM_LOAD( "688a12.2s",    0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )
        ROM_LOAD( "688a13.18d",   0x800000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
        ROM_LOAD( "688a14.13d",   0xa00000, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
        ROM_LOAD( "688a15.9d",    0xc00000, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
        ROM_LOAD( "688a16.4d",    0xe00000, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )
ROM_END

ROM_START( thunderh )
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
        ROM_LOAD32_BYTE( "680uaa01.21u", 0x000003, 0x080000, CRC(f2bb2ba1) SHA1(311e88d63179486014376c4af4ff0ef28673ee5a) )
        ROM_LOAD32_BYTE( "680uaa02.19u", 0x000002, 0x080000, CRC(52f617b5) SHA1(fda3133d3a7e04eb4432c69becdcf1872b3660d9) )
        ROM_LOAD32_BYTE( "680uaa03.21r", 0x000001, 0x080000, CRC(086a0574) SHA1(32fb93dbb93d2fe6af743ea4310b50a6cd03647d) )
        ROM_LOAD32_BYTE( "680uaa04.19r", 0x000000, 0x080000, CRC(85e1f8e3) SHA1(9172c54b6663f1bf390795068271198083a6860d) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
        ROM_LOAD32_WORD_SWAP( "680a05.14u",   0x000000, 0x200000, CRC(0c9f334d) SHA1(99ac622a04a7140244d81031df69a796b6fd2657) )
        ROM_LOAD32_WORD_SWAP( "680a06.12u",   0x000002, 0x200000, CRC(83074217) SHA1(bbf782ac125cd98d9147ef4e0373bf61f74726f7) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
        ROM_LOAD16_WORD_SWAP( "680a07.13k",   0x000000, 0x080000, CRC(12247a3e) SHA1(846cd9423efd3c9b17fce08393c6c83307d72f92) )

	ROM_REGION(0x20000, REGION_CPU3, 0)	/* 68k program for outboard sound? network? board */
        ROM_LOAD16_WORD_SWAP( "680c22.20k",   0x000000, 0x020000, CRC(d93c0ee2) SHA1(4b58418cbb01b51e12d6e7c86b2c81cd35d86248) )

	ROM_REGION(0x1000000, REGION_USER3, 0)	/* other roms */
        ROM_LOAD( "680a09.9s",    0x000000, 0x200000, CRC(71c2b049) SHA1(ce360172c8774b31edf16a80104c35b1caf26cd9) )
        ROM_LOAD( "680a10.7s",    0x200000, 0x200000, CRC(19882bf3) SHA1(7287da58853c84cbadbfb42bed37f2b0032c4b4d) )
        ROM_LOAD( "680a11.5s",    0x400000, 0x200000, CRC(0c74fe3f) SHA1(2e69f8d37552a74bbda65b134f747b4380ed33b0) )
        ROM_LOAD( "680a12.2s",    0x600000, 0x200000, CRC(b052919d) SHA1(a61c8eaf378ab7d780478db61217302d1b9f8f73) )
        ROM_LOAD( "680a13.18d",   0x800000, 0x200000, CRC(233f9074) SHA1(78ce42c35407d61df37cc0d16cdee84f8de968fa) )
        ROM_LOAD( "680a14.13d",   0xa00000, 0x200000, CRC(9ae15033) SHA1(12e291114629632b81f53811a6c8666aff4e92f3) )
        ROM_LOAD( "680a15.9d",    0xc00000, 0x200000, CRC(dc47c86f) SHA1(71af9b21f1ecc063135f501b1561869ee910c236) )
        ROM_LOAD( "680a16.4d",    0xe00000, 0x200000, CRC(ea388143) SHA1(3de5314a009d702186d5e285c8edefdd48139eab) )
ROM_END

static DRIVER_INIT(gticlub)
{
	init_konami_cgboard(0);
	sharc_dataram = auto_malloc(0x100000);
}

/*************************************************************************/

GAMEX( 1996, gticlub,	0,		 gticlub, gticlub, gticlub,	ROT0,	"Konami",	"GTI Club (ver AAA)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1996, gticlubj,	gticlub, gticlub, gticlub, gticlub,	ROT0,	"Konami",	"GTI Club (ver JAA)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1996, thunderh,	0,		 gticlub, gticlub, gticlub,	ROT0,	"Konami",	"Thunder Hurricane (ver UAA)", GAME_NOT_WORKING|GAME_NO_SOUND )

