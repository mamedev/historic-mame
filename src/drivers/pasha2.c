/********************************************************************

Pasha Pasha 2
Dong Sung, 1998

3PLAY
|--------------------------------------------------|
|      DA1311    UM53   AD-65    DREAM  9.6MHz     |
|KA22065  TL062  UM51   AD-65          RESET_SW    |
|   VOL1   VOL2                  1MHz   TL7705     |
|                                                  |
|    DSW2(8)            20MHz    GM71C18163        |
|     93C46                                   UM2  |
|           PAL                                    |
|J                                                 |
|A                                                 |
|M                      E1-16XT             AT89C52|
|M                                                 |
|A                                  U102           |
|           6116                                   |
|           6116       U3           U101           |
|                                                  |
|                                                  |
|                                             12MHz|
|                    A42MX16                       |
|    DSW1(8)                                       |
| UCN5801                                          |
| UCN5801                                          |
|         16MHz                                    |
|--------------------------------------------------|
Notes:
      U3         - 27C040 EPROM (DIP32)
      UM2/UM51/53- 29F040 EPROM (PLCC32)
      U101/102   - Each location contains a small adapter board plugged into a DIP42 socket. Each
                   adapter board holds 2x Intel E28F016S5 TSOP40 FlashROMs. On the PCB under the ROMs
                   it's marked '32MASK'
      A42MX16    - Actel A42MX16 FPGA (QFP160)
      AT89C52    - Atmel AT89C52 Microcontroller w/8k internal FlashROM, clock 12MHz (DIP40)
      E1-16XT    - Hyperstone E1-16XT CPU, clock 20MHz
      DREAM      - ATMEL DREAM SAM9773 Single Chip Synthesizer/MIDI with Effects and Serial Interface, clock 9.6MHz (TQFP80)
      AD-65      - Oki compatible M6295 sound chip, clock 1MHz
      5493R45    - ISSI 5493R45-001 128k x8 SRAM (SOJ32)
      GM71C18163 - Hynix 1M x16 DRAM (SOJ42)
      VSync      - 60Hz
      HSync      - 15.15kHz


 preliminary driver by Pierpaolo Prazzoli

*********************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "sound/okim6295.h"

static UINT16 *tiles, *wram;

static WRITE16_HANDLER( pasha2_misc_w )
{
	if(data & 0x0800)
	{
		int bank = data & 0xf000;
		switch(bank)
		{
		//right?
		case 0x8000: memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 0); break;
		case 0x9000: memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 1); break;

		//???? empty banks ????
		case 0xa000: memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 2); break;
		case 0xb000: memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 3); break;
		case 0xc000: memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 4); break;
		case 0xd000: memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 5); break;
		default: printf("default = %X @ %X\n",bank, activecpu_get_pc());
		}
	}
	else
		printf("%X\n",activecpu_get_pc());
}

static READ16_HANDLER( fake_r )
{
	return rand();
	return ~0;
}

static ADDRESS_MAP_START( pasha2_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	AM_BASE(&wram)
	AM_RANGE(0x40000000, 0x4003ffff) AM_RAM AM_BASE(&tiles) //tiles?
	AM_RANGE(0x40060000, 0x40060001) AM_WRITENOP
	AM_RANGE(0x40064000, 0x40064001) AM_WRITENOP
	AM_RANGE(0x40068000, 0x40068001) AM_WRITENOP
	AM_RANGE(0x4006c000, 0x4006c001) AM_WRITENOP
	AM_RANGE(0x40070000, 0x40070001) AM_WRITENOP
	AM_RANGE(0x40074000, 0x40074001) AM_WRITENOP
	AM_RANGE(0x40078000, 0x40078001) AM_WRITENOP
	AM_RANGE(0x80000000, 0x803fffff) AM_ROMBANK(1)
	AM_RANGE(0xe0000000, 0xe00003ff) AM_RAM //tilemap? palette?
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1,0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( pasha2_io, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x00, 0x03) AM_WRITENOP
	AM_RANGE(0x08, 0x0b) AM_READ(fake_r)
	AM_RANGE(0x18, 0x1b) AM_READ(fake_r)
	AM_RANGE(0x20, 0x23) AM_WRITENOP
	AM_RANGE(0x40, 0x43) AM_READ(fake_r) // tests $100, $200, $300 (active low)
	AM_RANGE(0x60, 0x63) AM_READ(fake_r)
	AM_RANGE(0x80, 0x83) AM_READ(fake_r) // tests $8, $80, $8000 (active low, nibble swapped)
	AM_RANGE(0xa0, 0xa3) AM_WRITENOP
	AM_RANGE(0xc0, 0xc1) AM_WRITENOP
	AM_RANGE(0xc2, 0xc3) AM_WRITE(pasha2_misc_w)
	AM_RANGE(0xe0, 0xe3) AM_READ(fake_r) // mask $F -> then discarded
	AM_RANGE(0xe4, 0xe7) AM_READ(fake_r) // mask $F -> then discarded
	AM_RANGE(0xe0, 0xef) AM_WRITENOP
ADDRESS_MAP_END

VIDEO_UPDATE( pasha2 )
{/*
    int x, y, count = 0;

    fillbitmap(bitmap,Machine->pens[0],cliprect);

    for(y = 0; y < 256/2; x++)
    {
        for(x = 0; x < 256/2; y++)
        {
            plot_pixel(bitmap, x*2+0,y,Machine->pens[tiles[count] & 0xff]);
            plot_pixel(bitmap, x*2+1,y,Machine->pens[(tiles[count] & 0xff00)>>8]);
            count++;
        }
    }

*/
}

INPUT_PORTS_START( pasha2 )
INPUT_PORTS_END

static INTERRUPT_GEN( pasha2_interrupts )
{
	switch(cpu_getiloops())
	{
		case 0: cpunum_set_input_line(0, 2, PULSE_LINE); break; //?
		case 1: cpunum_set_input_line(0, 4, PULSE_LINE); break; //?
		case 2: cpunum_set_input_line(0, 5, PULSE_LINE); break; //vblank irq
	}

	//irq3 is enabled but it's empty
}

static MACHINE_DRIVER_START( pasha2 )
	MDRV_CPU_ADD(E116T /*E116XT*/, 20000000)	/* 20 MHz */
	MDRV_CPU_PROGRAM_MAP(pasha2_map,0)
	MDRV_CPU_IO_MAP(pasha2_io,0)
	MDRV_CPU_VBLANK_INT(pasha2_interrupts, 3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)

	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_UPDATE(pasha2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	//and ATMEL DREAM SAM9773
MACHINE_DRIVER_END


ROM_START( pasha2 )
	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD( "pp2.u3",       0x00000, 0x80000, CRC(1c701273) SHA1(f465323a1d3f2fd752c51c178fafe4cc866e28d6) )

	ROM_REGION16_BE( 0x400000*6, REGION_USER2, ROMREGION_ERASEFF ) /* data roms */
	ROM_LOAD16_BYTE( "pp2-b.u101",   0x000000, 0x200000, BAD_DUMP CRC(82b42d4d) SHA1(57b71b141350ed7a68fe152d85945ff0f2290eca) )
	ROM_LOAD16_BYTE( "pp2-a.u101",   0x000001, 0x200000, BAD_DUMP CRC(a571dee8) SHA1(e730e8239c688f4b55a8f7ee88f9c64a228e215a) )
	ROM_LOAD16_BYTE( "pp2-b.u102",   0x400000, 0x200000, BAD_DUMP CRC(9cf4363c) SHA1(1ab644ae0ef44df6690fe2ad794c4a79cb941ddb) )
	ROM_LOAD16_BYTE( "pp2-a.u102",   0x400001, 0x200000, BAD_DUMP CRC(45460cd3) SHA1(4df8bf9bbd90563816c86ef1e51447e828806071) )
	// empty space

	ROM_REGION( 0x0800, REGION_CPU2, 0 ) /* AT89C52 (protected) */
	ROM_LOAD( "pasha2_at89c52",  0x0000, 0x0800, NO_DUMP ) /* MCU internal 8K flash */

	ROM_REGION( 0x80000, REGION_USER3, 0 ) /* SAM9773 sound data ? */
	ROM_LOAD( "pp2.um2",      0x00000, 0x80000, CRC(86814b37) SHA1(70f8a94410e362669570c39e00492c0d69de6b17) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "pp2.um51",     0x00000, 0x80000, CRC(3b1b1a30) SHA1(1ea1266d280a2b96ac4ef9fe8ee7b1a5f7861672) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki Samples */
	ROM_LOAD( "pp2.um53",     0x00000, 0x80000, CRC(8a29ad03) SHA1(3e9b0c86d8e3bb0b7691f68ad45431f6f9e8edbd) )
ROM_END

static READ16_HANDLER( pasha2_speedup_r )
{
	if(activecpu_get_pc() == 0x8302)
	{
		cpu_spinuntil_int();
	}

	return wram[(0x95744/2)+offset];
}

DRIVER_INIT( pasha2 )
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x95744, 0x95747, 0, 0, pasha2_speedup_r );

	memory_set_bankptr(1, memory_region(REGION_USER2) + 0x400000 * 0);
}

GAME( 1998, pasha2, 0, pasha2, pasha2, pasha2, ROT0, "Dong Sung", "Pasha Pasha 2", GAME_NOT_WORKING | GAME_NO_SOUND )
