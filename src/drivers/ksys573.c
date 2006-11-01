/***************************************************************************

  Konami System 573
  ===========================================================
  Driver by R. Belmont & smf

  NOTE: The first time you run each game, it will go through a special initialization
  procedure.  This can be quite lengthy (in the case of Dark Horse Legend).  Let it
  complete all the way before exiting MAME and you will not have to do it again!

  NOTE 2: The first time you run Konami 80's Gallery, it will dump you on a clock
  setting screen.  Press DOWN to select "SAVE AND EXIT" then press player 1 START
  to continue.

  TODO:
  * fix root counters in machine/psx.c so the hack here (actually MAME 0.89's machine/psx.c code)
    can be removed
  * integrate ATAPI code with Aaron's ATA/IDE code

  -----------------------------------------------------------------------------------------

  System 573 Hardware Overview
  Konami, 1998-2001

  This system uses Konami PSX-based hardware with an ATAPI CDROM drive.
  There is a slot for a security cart (cart is installed in CN14) and also a PCMCIA card slot,
  which is unused. The main board and CDROM drive are housed in a black metal box.
  The games can be swapped by exchanging the CDROM disc and the security cart, whereby the main-board
  FlashROMs are re-programmed after a small wait. On subsequent power-ups, there is a check to test if the
  contents of the FlashROMs matches the CDROM, then the game boots up immediately.

  The games that run on this system include...

  Game                                  Year    Hardware Code     CD Code
  --------------------------------------------------------------------------
  *Anime Champ                          2000
  Bass Angler                           1998    GE765 JA          765 JA A02
  *Bass Angler 2                        1999
  Dark Horse Legend                     1998    GX706 JA          706 JA A02
  *Dark Horse 2                         ?
  *Fighting Mania                       2000
  Fisherman's Bait                      1998    GE765 UA          765 UA B02
  Fisherman's Bait 2                    1998    GC865 UA          865 UA B02
  Fisherman's Bait Marlin Challenge     1999
  *Gun Mania                            2001
  *Gun Mania Zone Plus                  2001
  *Gachaga Champ                        1999
  *Hyper Bishi Bashi                    1999
  *Hyper Bishi Bashi Champ              2000
  Jikkyo Pawafuru Pro Yakyu             1998    GX802 JA          802 JA B02
  *Kick & Kick                          2001
  Konami 80's Arcade Gallery            1998    GC826 JA          826 JA A01
  Konami 80's AC Special                1998    GC826 UA          826 UA A01
  Salary Man Champ                      2001
  *Step Champ                           1999

  Note:
       Not all games listed above are confirmed to run on System 573.
       * - denotes not dumped yet. If you can help with the remaining undumped System 573 games,
       please contact http://www.mameworld.net/gurudumps/comments.html


  Main PCB Layout
  ---------------
                                                     External controls port
  GX700-PWB(A)B                                               ||
  (C)1997 KONAMI CO. LTD.                                     \/
  |-----------------------------------------------------==============-------|
  |   CN15            CNA                     CN10                           |
  |        CN16                                                              |
  |                                                 |------------------------|
  | PQ30RV21                                        |                        |
  |                         |-------|               |                        |
  |             KM416V256   |SONY   |               |     PCMCIA SLOT        |
  |                         |CXD2925|               |                        |
  |                         |-------|               |                        |
  |                                                 |                        |
  |                                                 |------------------------|
  | |-----|                                        CN21                      |
  | |32M  |  |---------|     |---------|                                     |
  | |-----|  |SONY     |     |SONY     |                                     |
  |          |CXD8561Q |     |CXD8530CQ|           29F016   29F016   |--|    |
  | |-----|  |         |     |         |                             |  |    |
  | |32M  |  |         |     |         |                             |  |    |
  | |-----|  |---------|     |---------|           29F016   29F016   |  |    |
  |      53.693175MHz    67.7376MHz                                  |  |    |
  |                                     |-----|                      |  |CN14|
  |      KM48V514      KM48V514         |9536 |    29F016   29F016   |  |    |
  |            KM48V514       KM48V514  |     |                      |  |    |
  |      KM48V514      KM48V514         |-----|                      |  |    |
  |            KM48V514      KM48V514              29F016   29F016   |--|    |
  | MC44200FT                          M48T58Y-70PC1                         |
  |                                                                      CN12|
  |                                    700A01.22                             |
  |                             14.7456MHz                                   |
  |                  |-------|                                               |
  |                  |KONAMI |    |----|                               LA4705|
  |   058232         |056879 |    |3644|                            SM5877   |
  |                  |       |    |----|         ADC0834                LM358|
  |                  |-------|            ADM485           CN4               |
  |                                                         CN3      CN17    |
  |                                TEST_SW  DIP4 USB   CN8     RCA-L/R   CN9 |
  |--|          JAMMA            |-------------------------------------------|
     |---------------------------|
  Notes:
  CNA       - 40-pin IDE cable connector
  CN3       - 10-pin connector labelled 'ANALOG', connected to a 9-pin DSUB connector mounted in the
              front face of the housing, labelled 'OPTION1'
  CN4       - 12-pin connector labelled 'EXT-OUT'
  CN5       - 10-pin connector labelled 'EXT-IN', connected to a 9-pin DSUB connector mounted in the
              front face of the housing, labelled 'OPTION2'
  CN8       - 15-pin DSUB plug labelled 'VGA-DSUB15' extending from the front face of the housing
              labelled 'RGB'. Use of this connector is optional because the video is output via the
              standard JAMMA connector
  CN9       - 4-pin connector for amplified stereo sound output to 2 speakers
  CN10      - Custom 80-pin connector (for mounting an additional plug-in board for extra controls,
              possibly with CN21 also)
  CN12      - 4-pin CD-DA input connector (for Red-Book audio from CDROM drive to main board)
  CN14      - 44-pin security cartridge connector. The cartridge only contains a small PCB labelled
              'GX700-PWB(D) (C)1997 KONAMI' and has locations for 2 ICs only
              IC1 - Small SOIC8 chip, identified as a XICOR X76F041 security supervisor containing 4X
              128 x8 secureFLASH arrays, stamped '0038323 E9750'
              IC2 - Solder pads for mounting of a PLCC68 or QFP68 packaged IC (not populated)
  CN15      - 4-pin CDROM power connector
  CN16      - 2-pin fan connector
  CN17      - 6-pin power connector, connected to an 8-pin power plug mounted in the front face
              of the housing. This can be left unused because the JAMMA connector supplies all power
              requirements to the PCB
  CN21      - Custom 30-pin connector (purpose unknown, but probably for mounting an additional
              plug-in board with CN10 also)
  TEST_SW   - Push-button test switch
  DIP4      - 4-position DIP switch
  USB       - USB connector extended from the front face of the housing labelled 'I/O'
  RCA-L/R   - RCA connectors for left/right audio output
  PQ30RV21  - Sharp PQ30RV21 low-power voltage regulator (5 Volt to 3 Volt)
  LA4705    - Sanyo LA4705 15W 2-channel power amplifier (SIP18)
  LM358     - National Semiconductor LM358 low power dual operational amplifier (SOIC8, @ 33C)
  CXD2925Q  - Sony CXD2925Q SPU (QFP100, @ 15Q)
  CXD8561Q  - Sony CXD8561Q GTE (QFP208, @ 10M)
  CXD8530CQ - Sony CXD8530CQ R3000-based CPU (QFP208, @ 17M)
  9536      - Xilinx XC9536 in-system-programmable CPLD (PLCC44, @ 22J)
  3644      - Hitachi H8/3644 HD6473644H microcontroller with 32k ROM & 1k RAM (QFP64, @ 18E,
              labelled '700 02 38920')
  056879    - Konami 056879 custom IC (QFP120, @ 13E)
  MC44200FT - Motorola MC44200FT Triple 8-bit Video DAC (QFP44)
  058232    - Konami 058232 custom ceramic IC (SIP14, @ 6C)
  SM5877    - Nippon Precision Circuits SM5877 2-channel D/A convertor (SSOP24, @32D)
  ADM485    - Analog Devices ADM485 low power EIA RS-485 transceiver (SOIC8, @ 20C)
  ADC0834   - National Semiconductor ADC0834 8-Bit Serial I/O A/D Converter with Multiplexer
              Option (SOIC14, @ 24D)
  M48T58Y-70- STMicroelectronics M48T58Y-70PC1 8k x8 Timekeeper RAM (DIP32, @ 22H)
              Note that this is not used for protection. If you put in a new blank Timekeeper RAM
              it will be programmed with some data on power-up. If you swap games, the Timekeeper
              is updated with the new game data
  29F016      Fujitsu 29F016A-90PFTN 2M x8 FlashROM (TSOP48, @ 27H/J/L/M & 31H/J/L/M)
              Also found Sharp LH28F016S (2M x8 TSOP40) in some units
  KM416V256 - Samsung Electronics KM416V256BT-7 256k x 16 DRAM (TSOP44/40, @ 11Q)
  KM48V514  - Samsung Electronics KM48V514BJ-6 512k x 8 EDO DRAM (SOJ28, @ 16G/H, 14G/H, 12G/H, 9G/H)
  32M       - NEC D481850GF-A12 128k x 32Bit x 2 Banks SGRAM (QFP100, @ 4P & 4L)

  Software  -
              - 700A01.22G 4M MaskROM (DIP32, @ 22G)
              - SONY ATAPI CDROM drive, with CDROM disc containing program + graphics + sound
                Some System 573 units contain a CR-583 drive dated October 1997, some contain a
                CR-587 drive dated March 1998. Note that the CR-587 will not read CDR discs ;-)


  Auxillary Controls PCB
  ----------------------

  GE765-PWB(B)A (C)1998 KONAMI CO. LTD.
  |-----------------------------|
  |          CN33     C2242     |
  |                   C2242     |
  |       NRPS11-G1A            |
  |                         CN35|
  |  D4701                      |
  |        74LS14     PC817     |-----------------|
  |                                               |
  |  PAL         PAL                              |
  | (E765B1)    (E765B2)         LCX245           |
  |                                               |
  |  74LS174     PAL                              |
  |             (E765B1)                          |
  |                                               |
  |              74LS174       CN31               |
  |-----------------------------------------------|
  Notes: (all IC's shown)
        This PCB is known to be used for the fishing reel controls on all of the fishing games (at least).

        CN31       - Connector joining this PCB to the MAIN PCB
        CN33       - Connector used to join the external controls connector mounted on the outside of the
                     metal case to this PCB.
        CN35       - Power connector
        NRPS11-G1A - Relay?
        D4701      - NEC uPD4701 Encoder (SOP24)
        C2242      - 2SC2242 Transistor
        PC817      - Sharp PC817 Photo-coupler IC (DIP4)
        PAL        - AMD PALCE16V8Q, stamped 'E765Bx' (DIP20)
*/

#include "driver.h"
#include "state.h"
#include "cdrom.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "machine/eeprom.h"
#include "machine/intelfsh.h"
#include "machine/scsidev.h"
#include "machine/timekpr.h"
#include "machine/x76f041.h"
#include "sound/psx.h"
#include "sound/cdda.h"
#include <time.h>

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		if( cpu_getactivecpu() != -1 )
		{
			logerror( "%08x: %s", activecpu_get_pc(), buf );
		}
		else
		{
			logerror( "(timer) : %s", buf );
		}
	}
}

static INT32 flash_bank;
static int flash_chips;
static int onboard_flash_start;
static int pccard1_flash_start;
static int pccard2_flash_start;
static int pccard3_flash_start;
static int pccard4_flash_start;


/* EEPROM handlers */

static NVRAM_HANDLER( konami573 )
{
	int i;

	nvram_handler_timekeeper_0( machine, file, read_or_write );
	nvram_handler_x76f041_0( machine, file, read_or_write );

	for( i = 0; i < flash_chips; i++ )
	{
		nvram_handler_intelflash( machine, i, file, read_or_write );
	}
}

static WRITE32_HANDLER( mb89371_w )
{
	verboselog( 2, "mb89371_w %08x %08x %08x\n", offset, mem_mask, data );
}

static READ32_HANDLER( mb89371_r )
{
	verboselog( 2, "mb89371_r %08x %08x\n", offset, mem_mask );
	return 0xffffffff;
}

static READ32_HANDLER( jamma_r )
{
	UINT32 data = 0;

//  logerror("JAMMA: read @ %x (mask %x PC %x)\n", offset, mem_mask, activecpu_get_pc());

	switch (offset)
	{
	case 0:
		data = readinputport(0);
		break;
	case 1:
		data = readinputport(1);
		data |= 0x000000c0;
		data |= x76f041_sda_read( 0 ) << 18;
		if( pccard1_flash_start < 0 )
		{
			data |= ( 1 << 26 );
		}
		if( pccard2_flash_start < 0 )
		{
			data |= ( 1 << 27 );
		}
		break;
	case 2:
		data = readinputport(2);
		break;
	case 3:
		data = readinputport(3);
		break;
	}

	return data;
}

static WRITE32_HANDLER( jamma_w )
{
	logerror("JAMMA: write %x to %x (mask %x PC %x)\n", data, offset, mem_mask, activecpu_get_pc());
}

static UINT32 control;

static READ32_HANDLER( control_r )
{
	verboselog( 1, "control_r( %08x, %08x ) %08x\n", offset, mem_mask, control );

	return control;
}

static WRITE32_HANDLER( control_w )
{
//  int old_bank = flash_bank;
	COMBINE_DATA(&control);

	verboselog( 1, "control_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	flash_bank = -1;

	if( onboard_flash_start >= 0 && ( control & ~0x43 ) == 0x00 )
	{
		flash_bank = onboard_flash_start + ( ( control & 3 ) * 2 );
//      if( flash_bank != old_bank ) printf( "onboard %d\r", control & 3 );
	}
	else if( pccard1_flash_start >= 0 && ( control & ~0x47 ) == 0x10 )
	{
		flash_bank = pccard1_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) printf( "pccard1 %d\r", control & 7 );
	}
	else if( pccard2_flash_start >= 0 && ( control & ~0x47 ) == 0x20 )
	{
		flash_bank = pccard2_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) printf( "pccard2 %d\r", control & 7 );
	}
	else if( pccard3_flash_start >= 0 && ( control & ~0x47 ) == 0x20 )
	{
		flash_bank = pccard3_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) printf( "pccard3 %d\r", control & 7 );
	}
	else if( pccard4_flash_start >= 0 && ( control & ~0x47 ) == 0x28 )
	{
		flash_bank = pccard4_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) printf( "pccard4 %d\r", control & 7 );
	}
}

#define ATAPI_CYCLES_PER_SECTOR (5000)	// plenty of time to allow DMA setup etc.  BIOS requires this be at least 2000, individual games may vary.

static UINT8 atapi_regs[16];
static void *atapi_timer;
static pSCSIDispatch atapi_device;
static void *atapi_device_data;

#define ATAPI_STAT_BSY	   0x80
#define ATAPI_STAT_DRDY    0x40
#define ATAPI_STAT_DMARDDF 0x20
#define ATAPI_STAT_SERVDSC 0x10
#define ATAPI_STAT_DRQ     0x08
#define ATAPI_STAT_CORR    0x04
#define ATAPI_STAT_CHECK   0x01

#define ATAPI_INTREASON_COMMAND 0x01
#define ATAPI_INTREASON_IO      0x02
#define ATAPI_INTREASON_RELEASE 0x04

#define ATAPI_REG_DATA		0
#define ATAPI_REG_ERRFEAT	1
#define ATAPI_REG_INTREASON	2
#define ATAPI_REG_SAMTAG	3
#define ATAPI_REG_COUNTLOW	4
#define ATAPI_REG_COUNTHIGH	5
#define ATAPI_REG_DRIVESEL	6
#define ATAPI_REG_CMDSTATUS	7

static UINT16 atapi_data[32*1024];
static UINT8  atapi_scsi_packet[32*1024];
static int atapi_data_ptr, atapi_xferlen, atapi_xferbase, atapi_cdata_wait;

static UINT8 sector_buffer[ 4096 ];

static void atapi_xfer_end( int xfermod )
{
	int i, n_this;

	timer_adjust(atapi_timer, TIME_NEVER, 0, 0);

//  mame_printf_debug("ATAPI: xfer_end.  xferlen = %d, xfermod = %d\n", atapi_xferlen, xfermod);

	while (atapi_xferlen)
	{
		// get a sector from the SCSI device
		atapi_device(SCSIOP_READ_DATA, atapi_device_data, 2048, sector_buffer);

		atapi_xferlen -= 2048;

		i = 0;
		n_this = 2048 / 4;
		while( n_this > 0 )
		{
			g_p_n_psxram[ atapi_xferbase / 4 ] =
				( sector_buffer[ i + 0 ] << 0 ) |
				( sector_buffer[ i + 1 ] << 8 ) |
				( sector_buffer[ i + 2 ] << 16 ) |
				( sector_buffer[ i + 3 ] << 24 );
			atapi_xferbase += 4;
			i += 4;
			n_this--;
		}

	}

	if (xfermod > 0)
	{
		int nxmod = 0;

		if (xfermod > 63488)
		{
			atapi_xferlen = 63488;
			nxmod = xfermod - 63488;
		}
		else
		{
			atapi_xferlen = xfermod;
		}

		//mame_printf_debug("ATAPI: starting next piece of multi-part transfer\n");

		atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
		atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;

		timer_adjust(atapi_timer, TIME_IN_CYCLES((ATAPI_CYCLES_PER_SECTOR * (atapi_xferlen/2048)), 0), nxmod, 0);
	}
	else
	{
		//mame_printf_debug("ATAPI: Transfer completed, dropping DRQ\n");
		atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRDY;
		atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO | ATAPI_INTREASON_COMMAND;
	}

	psx_irq_set(0x400);
}

static READ32_HANDLER( atapi_r )
{
	int reg, data, shift;
	int i;
	UINT8 temp_data[64*1024];

	if (mem_mask == 0xffff0000)	// word-wide command read
	{
//      mame_printf_debug("ATAPI: packet read = %04x\n", atapi_data[atapi_data_ptr]);

		// assert IRQ and drop DRQ
		if (atapi_data_ptr == 0)
		{
			//mame_printf_debug("ATAPI: dropping DRQ\n");
			psx_irq_set(0x400);
			atapi_regs[ATAPI_REG_CMDSTATUS] = 0;

			// get the data from the device
			atapi_device(SCSIOP_READ_DATA, atapi_device_data, 0, temp_data);

			// fix it up in an endian-safe way
			for (i = 0; i < atapi_xferlen; i += 2)
			{
				atapi_data[i/2] = temp_data[i] | temp_data[i+1]<<8;
			}
		}

		data = atapi_data[atapi_data_ptr];
		atapi_data_ptr++;
		shift = 0;
	}
	else
	{
		reg = offset<<1;
		shift = 0;
		if (mem_mask == 0xff00ffff)
		{
			reg += 1;
			shift = 16;
		}

		data = atapi_regs[reg];

//      mame_printf_debug("ATAPI: read reg %d = %x (PC=%x)\n", reg, data, activecpu_get_pc());
	}

	return data<<shift;
}

static WRITE32_HANDLER( atapi_w )
{
	int reg;
	int xfermod;
	int i;

	if (mem_mask == 0xffff0000)	// word-wide command write
	{
//      mame_printf_debug("ATAPI: packet write %04x\n", data);
		atapi_data[atapi_data_ptr] = data;
		atapi_data_ptr++;

		if (atapi_cdata_wait)
		{
//          mame_printf_debug("ATAPI: waiting, ptr %d wait %d\n", atapi_data_ptr, atapi_cdata_wait);
			if (atapi_data_ptr == atapi_cdata_wait)
			{
				// decompose SCSI packet into proper byte order
				for (i = 0; i < atapi_cdata_wait; i += 2)
				{
					atapi_scsi_packet[i] = atapi_data[i/2]&0xff;
					atapi_scsi_packet[i+1] = atapi_data[i/2]>>8;
				}

				// send it to the device
				atapi_device(SCSIOP_WRITE_DATA, atapi_device_data, atapi_cdata_wait, atapi_scsi_packet);

				// assert IRQ
				psx_irq_set(0x400);

				// not sure here, but clear DRQ at least?
				atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
			}
		}

		if ((!atapi_cdata_wait) && (atapi_data_ptr == 6))
		{
			// reset data pointer for reading SCSI results
			atapi_data_ptr = 0;

			// assert IRQ
			psx_irq_set(0x400);

			// decompose SCSI packet into proper byte order
			for (i = 0; i < 16; i += 2)
			{
				atapi_scsi_packet[i] = atapi_data[i/2]&0xff;
				atapi_scsi_packet[i+1] = atapi_data[i/2]>>8;
			}

			// send it to the SCSI device
			atapi_xferlen = atapi_device(SCSIOP_EXEC_COMMAND, atapi_device_data, 0, atapi_scsi_packet);

			if (atapi_xferlen != -1)
			{
//              mame_printf_debug("ATAPI: SCSI command %02x returned %d bytes from the device\n", atapi_data[0]&0xff, atapi_xferlen);

				// store the returned command length in the ATAPI regs, splitting into
				// multiple transfers if necessary
				xfermod = 0;
				if (atapi_xferlen > 63488)
				{
						xfermod = atapi_xferlen - 63488;
					atapi_xferlen = 63488;
				}


				atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
				atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;

				// perform special ATAPI processing of certain commands
				switch (atapi_data[0]&0xff)
				{
					case 0x42:	// READ SUB-CHANNEL
						atapi_regs[ATAPI_REG_CMDSTATUS] |= ATAPI_STAT_CHECK | ATAPI_STAT_SERVDSC;
						break;

					case 0x55:	// MODE SELECT
						atapi_cdata_wait = atapi_data[4]/2;
						atapi_data_ptr = 0;
						break;

					case 0xa8:	// READ (12)
						// indicate data ready: set DRQ and DMA ready, and IO in INTREASON
						atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
						atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;

						// set a transfer complete timer (Note: CYCLES_PER_SECTOR can't be lower than 2000 or the BIOS ends up "out of order")
						timer_adjust(atapi_timer, TIME_IN_CYCLES((ATAPI_CYCLES_PER_SECTOR * (atapi_xferlen/2048)), 0), xfermod, 0);
						break;

					case 0x00:	// BUS RESET / TEST UNIT READY
						atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
						break;
				}

				// if no data to return, set the registers properly
				if (atapi_xferlen == 0)
				{
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRDY;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO|ATAPI_INTREASON_COMMAND;
				}
			}
			else
			{
//              mame_printf_debug("ATAPI: SCSI device returned error!\n");

				atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_CHECK;
				atapi_regs[ATAPI_REG_ERRFEAT] = 0x50;	// sense key = ILLEGAL REQUEST
				atapi_regs[ATAPI_REG_COUNTLOW] = 0;
				atapi_regs[ATAPI_REG_COUNTHIGH] = 0;
			}
		}
	}
	else
	{
		reg = offset<<1;
		if (mem_mask == 0xff00ffff)
		{
			reg += 1;
			data >>= 16;
		}

		atapi_regs[reg] = data;
//      mame_printf_debug("ATAPI: reg %d = %x (offset %x mask %x PC=%x)\n", reg, data, offset, mem_mask, activecpu_get_pc());

		if (reg == ATAPI_REG_CMDSTATUS)
		{
//          mame_printf_debug("ATAPI command %x issued! (PC=%x)\n", data, activecpu_get_pc());

			switch (data)
			{
				case 0xa0:	// PACKET
		 			atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_COMMAND;

					atapi_data_ptr = 0;
					atapi_cdata_wait = 0;
					break;

				case 0xa1:	// IDENTIFY PACKET DEVICE
		 			atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;

					atapi_data_ptr = 0;
					atapi_data[0] = 0x8500;	// ATAPI device, cmd set 5 compliant, DRQ within 3 ms of PACKET command
					atapi_data[49] = 0x0400; // IORDY may be disabled

					atapi_regs[ATAPI_REG_COUNTLOW] = 0;
					atapi_regs[ATAPI_REG_COUNTHIGH] = 2;

					psx_irq_set(0x400);
						break;

				case 0xef:	// SET FEATURES
		 			atapi_regs[ATAPI_REG_CMDSTATUS] = 0;

					atapi_data_ptr = 0;

					psx_irq_set(0x400);
					break;

				default:
					mame_printf_debug("ATAPI: Unknown IDE command %x\n", data);
					break;
			}
		}
	 }
}

static void atapi_init(void)
{
	memset(atapi_regs, 0, sizeof(atapi_regs));

	atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
	atapi_regs[ATAPI_REG_ERRFEAT] = 1;
	atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
	atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;

	atapi_data_ptr = 0;
	atapi_cdata_wait = 0;

	atapi_timer = timer_alloc( atapi_xfer_end );
	timer_adjust(atapi_timer, TIME_NEVER, 0, 0);

	// allocate a SCSI CD-ROM device
	atapi_device = SCSI_DEVICE_CDROM;
	atapi_device(SCSIOP_ALLOC_INSTANCE, &atapi_device_data, 0, (UINT8 *)NULL);
}

static WRITE32_HANDLER( atapi_reset_w )
{
	if (data)
	{
//      mame_printf_debug("ATAPI reset\n");

		atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
		atapi_regs[ATAPI_REG_ERRFEAT] = 1;
		atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
		atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;

		atapi_data_ptr = 0;
		atapi_cdata_wait = 0;
	}
}

static void cdrom_dma_read( UINT32 n_address, INT32 n_size )
{
//  mame_printf_debug("DMA read: address %08x size %08x\n", n_address, n_size);
}

static void cdrom_dma_write( UINT32 n_address, INT32 n_size )
{
//  mame_printf_debug("DMA write: address %08x size %08x\n", n_address, n_size);

	atapi_xferbase = n_address;
}

static UINT32 m_n_security_control;

static WRITE32_HANDLER( security_w )
{
	COMBINE_DATA( &m_n_security_control );

	verboselog( 2, "security_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	if( ACCESSING_LSW32 )
	{
		x76f041_sda_write( 0, ( data >> 0 ) & 1 );
		x76f041_scl_write( 0, ( data >> 1 ) & 1 );
		x76f041_cs_write( 0, ( data >> 2 ) & 1 );
		x76f041_rst_write( 0, ( data >> 3 ) & 1 );
	}
}

static READ32_HANDLER( security_r )
{
	UINT32 data = m_n_security_control;
	verboselog( 2, "security_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
	return data;
}

static READ32_HANDLER( flash_r )
{
	UINT32 data = 0;

	if( flash_bank < 0 )
	{
		printf( "%08x: flash_r( %08x, %08x ) no bank selected %08x\n", activecpu_get_pc(), offset, mem_mask, control );
		data = 0xffffffff;
	}
	else
	{
		int adr = offset * 2;

		if( ACCESSING_LSB32 )
		{
			data |= ( intelflash_read( flash_bank + 0, adr + 0 ) & 0xff ) << 0; // 31m/31l/31j/31h
		}
		if( ( mem_mask & 0x0000ff00 ) == 0 )
		{
			data |= ( intelflash_read( flash_bank + 1, adr + 0 ) & 0xff ) << 8; // 27m/27l/27j/27h
		}
		if( ( mem_mask & 0x00ff0000 ) == 0 )
		{
			data |= ( intelflash_read( flash_bank + 0, adr + 1 ) & 0xff ) << 16; // 31m/31l/31j/31h
		}
		if( ACCESSING_MSB32 )
		{
			data |= ( intelflash_read( flash_bank + 1, adr + 1 ) & 0xff ) << 24; // 27m/27l/27j/27h
		}
	}

	verboselog( 2, "flash_r: %08x = %08x (mask %08x)\n", offset, data, mem_mask );

	return data;
}

static WRITE32_HANDLER( flash_w )
{
	verboselog( 2, "flash_w: %08x to %08x (mask %08x)\n", data, offset, mem_mask );

	if( flash_bank < 0 )
	{
		printf( "%08x: flash_w( %08x, %08x, %08x ) no bank selected %08x\n", activecpu_get_pc(), offset, mem_mask, data, control );
	}
	else
	{
		int adr = offset * 2;

		if( ( mem_mask & 0x000000ff ) == 0 )
		{
			intelflash_write( flash_bank + 0, adr + 0, ( data >> 0 ) & 0xff );
		}
		if( ( mem_mask & 0x0000ff00 ) == 0 )
		{
			intelflash_write( flash_bank + 1, adr + 0, ( data >> 8 ) & 0xff );
		}
		if( ( mem_mask & 0x00ff0000 ) == 0 )
		{
			intelflash_write( flash_bank + 0, adr + 1, ( data >> 16 ) & 0xff );
		}
		if( ( mem_mask & 0xff000000 ) == 0 )
		{
			intelflash_write( flash_bank + 1, adr + 1, ( data >> 24 ) & 0xff );
		}
	}
}

static READ32_HANDLER( unk_r )
{
	return 0xffffffff;
}

/* Root Counters */

static void *m_p_timer_root[ 3 ];
static UINT16 m_p_n_root_count[ 3 ];
static UINT16 m_p_n_root_mode[ 3 ];
static UINT16 m_p_n_root_target[ 3 ];
static UINT32 m_p_n_root_start[ 3 ];

#define RC_STOP ( 0x01 )
#define RC_RESET ( 0x04 ) /* guess */
#define RC_COUNTTARGET ( 0x08 )
#define RC_IRQTARGET ( 0x10 )
#define RC_IRQOVERFLOW ( 0x20 )
#define RC_REPEAT ( 0x40 )
#define RC_CLC ( 0x100 )
#define RC_DIV ( 0x200 )

static UINT32 psxcpu_gettotalcycles( void )
{
	/* TODO: should return the start of the current tick. */
	return cpunum_gettotalcycles(0) * 2;
}

static int root_divider( int n_counter )
{
	if( n_counter == 0 && ( m_p_n_root_mode[ n_counter ] & RC_CLC ) != 0 )
	{
		/* TODO: pixel clock, probably based on resolution */
		return 5;
	}
	else if( n_counter == 1 && ( m_p_n_root_mode[ n_counter ] & RC_CLC ) != 0 )
	{
		return 2150;
	}
	else if( n_counter == 2 && ( m_p_n_root_mode[ n_counter ] & RC_DIV ) != 0 )
	{
		return 8;
	}
	return 1;
}

static UINT16 root_current( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & RC_STOP ) != 0 )
	{
		return m_p_n_root_count[ n_counter ];
	}
	else
	{
		UINT32 n_current;
		n_current = psxcpu_gettotalcycles() - m_p_n_root_start[ n_counter ];
		n_current /= root_divider( n_counter );
		n_current += m_p_n_root_count[ n_counter ];
		if( n_current > 0xffff )
		{
			/* TODO: use timer for wrap on 0x10000. */
			m_p_n_root_count[ n_counter ] = n_current;
			m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
		}
		return n_current;
	}
}

static int root_target( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & RC_COUNTTARGET ) != 0 ||
		( m_p_n_root_mode[ n_counter ] & RC_IRQTARGET ) != 0 )
	{
		return m_p_n_root_target[ n_counter ];
	}
	return 0x10000;
}

static void root_timer_adjust( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & RC_STOP ) != 0 )
	{
		timer_adjust( m_p_timer_root[ n_counter ], TIME_NEVER, n_counter, 0 );
	}
	else
	{
		int n_duration;

		n_duration = root_target( n_counter ) - root_current( n_counter );
		if( n_duration < 1 )
		{
			n_duration += 0x10000;
		}

		n_duration *= root_divider( n_counter );

		timer_adjust( m_p_timer_root[ n_counter ], TIME_IN_SEC( (double)n_duration / 33868800 ), n_counter, 0 );
	}
}

static void root_finished( int n_counter )
{
//  if( ( m_p_n_root_mode[ n_counter ] & RC_COUNTTARGET ) != 0 )
	{
		/* TODO: wrap should be handled differently as RC_COUNTTARGET & RC_IRQTARGET don't have to be the same. */
		m_p_n_root_count[ n_counter ] = 0;
		m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
	}
	if( ( m_p_n_root_mode[ n_counter ] & RC_REPEAT ) != 0 )
	{
		root_timer_adjust( n_counter );
	}
	if( ( m_p_n_root_mode[ n_counter ] & RC_IRQOVERFLOW ) != 0 ||
		( m_p_n_root_mode[ n_counter ] & RC_IRQTARGET ) != 0 )
	{
		psx_irq_set( 0x10 << n_counter );
	}
}

WRITE32_HANDLER( k573_counter_w )
{
	int n_counter;

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		m_p_n_root_count[ n_counter ] = data;
		m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
		break;
	case 1:
		m_p_n_root_count[ n_counter ] = root_current( n_counter );
		m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
		m_p_n_root_mode[ n_counter ] = data;

		if( ( m_p_n_root_mode[ n_counter ] & RC_RESET ) != 0 )
		{
			/* todo: check this is correct */
			m_p_n_root_count[ n_counter ] = 0;
			m_p_n_root_mode[ n_counter ] &= ~( RC_STOP | RC_RESET );
		}
//      if( ( data & 0xfca6 ) != 0 ||
//          ( ( data & 0x0100 ) != 0 && n_counter != 0 && n_counter != 1 ) ||
//          ( ( data & 0x0200 ) != 0 && n_counter != 2 ) )
//      {
//          printf( "mode %d 0x%04x\n", n_counter, data & 0xfca6 );
//      }
		break;
	case 2:
		m_p_n_root_target[ n_counter ] = data;
		break;
	default:
		return;
	}

	root_timer_adjust( n_counter );
}

READ32_HANDLER( k573_counter_r )
{
	int n_counter;
	UINT32 data;

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		data = root_current( n_counter );
		break;
	case 1:
		data = m_p_n_root_mode[ n_counter ];
		break;
	case 2:
		data = m_p_n_root_target[ n_counter ];
		break;
	default:
		return 0;
	}
	return data;
}

static ADDRESS_MAP_START( konami573_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f000000, 0x1f3fffff) AM_READWRITE( flash_r, flash_w )
	AM_RANGE(0x1f400000, 0x1f40001f) AM_READWRITE( jamma_r, jamma_w )
	AM_RANGE(0x1f480000, 0x1f48000f) AM_READWRITE( atapi_r, atapi_w )	// IDE controller, used mostly in ATAPI mode (only 3 pure IDE commands seen so far)
	AM_RANGE(0x1f500000, 0x1f500003) AM_READWRITE( control_r, control_w )	// Konami can't make a game without a "control" register.
	AM_RANGE(0x1f560000, 0x1f560003) AM_WRITE( atapi_reset_w )
	AM_RANGE(0x1f5c0000, 0x1f5c0003) AM_WRITENOP 				// watchdog?
	AM_RANGE(0x1f620000, 0x1f623fff) AM_READWRITE( timekeeper_0_32le_lsb16_r, timekeeper_0_32le_lsb16_w )
	AM_RANGE(0x1f640098, 0x1f64009b) AM_READ( unk_r ) AM_WRITENOP
	AM_RANGE(0x1f680000, 0x1f68001f) AM_READWRITE(mb89371_r, mb89371_w)
	AM_RANGE(0x1f6a0000, 0x1f6a0003) AM_READWRITE( security_r, security_w )
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(k573_counter_r, k573_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END


static void flash_init( void )
{
	int i;
	int chip;
	int size;
	unsigned char *data;
	static struct
	{
		int *start;
		int region;
		int chips;
		int type;
		int size;
	}
	flash_init[] =
	{
		{ &onboard_flash_start, REGION_USER3,  8, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard1_flash_start, REGION_USER4, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard2_flash_start, REGION_USER5, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard3_flash_start, REGION_USER6, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard4_flash_start, REGION_USER7, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ NULL, 0, 0, 0, 0 },
	};

	flash_chips = 0;

	i = 0;
	while( flash_init[ i ].start != NULL )
	{
		data = memory_region( flash_init[ i ].region );
		if( data != NULL )
		{
			size = 0;
			*( flash_init[ i ].start ) = flash_chips;
			for( chip = 0; chip < flash_init[ i ].chips; chip++ )
			{
				intelflash_init( flash_chips, flash_init[ i ].type, data + size );
				size += flash_init[ i ].size;
				flash_chips++;
			}
			if( size != memory_region_length( flash_init[ i ].region ) )
			{
				fatalerror( "flash_init %d incorrect region length\n", i );
			}
		}
		else
		{
			*( flash_init[ i ].start ) = -1;
		}
		i++;
	}
}


static DRIVER_INIT( konami573 )
{
	int i;

	psx_driver_init();
	atapi_init();
	psx_dma_install_read_handler(5, cdrom_dma_read);
	psx_dma_install_write_handler(5, cdrom_dma_write);

	for (i = 0; i < 3; i++)
	{
		m_p_timer_root[i] = timer_alloc(root_finished);
	}

	timekeeper_init( 0, TIMEKEEPER_M48T58, NULL );
	x76f041_init( 0, memory_region( REGION_USER2 ) );
	flash_init();
}

static MACHINE_RESET( konami573 )
{
	psx_machine_init();

	/* security cart */
	psx_sio_input( 1, PSX_SIO_IN_DSR, PSX_SIO_IN_DSR );

	flash_bank = -1;
}

static struct PSXSPUinterface konami573_psxspu_interface =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

INTERRUPT_GEN( sys573_vblank )
{
	psx_vblank();
}

static MACHINE_DRIVER_START( konami573 )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( konami573_map, 0 )
	MDRV_CPU_VBLANK_INT( sys573_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( konami573 )
	MDRV_NVRAM_HANDLER( konami573 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( konami573_psxspu_interface )
	MDRV_SOUND_ROUTE( 0, "left", 1.0 )
	MDRV_SOUND_ROUTE( 1, "right", 1.0 )
MACHINE_DRIVER_END

INPUT_PORTS_START( konami573 )
	/* IN0*/
	PORT_START
	PORT_BIT( 0xffffffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	PORT_START
	PORT_DIPNAME( 0x00000001, 0x00000001, "Unused 2" )
	PORT_DIPSETTING(          0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, "Screen Flip" )
	PORT_DIPSETTING(          0x00000002, DEF_STR( Normal ) )
	PORT_DIPSETTING(          0x00000000, "V-Flip" )
	PORT_DIPNAME( 0x00000004, 0x00000004, "Unused 1")
	PORT_DIPSETTING(          0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, "Start Up Device" )
	PORT_DIPSETTING(          0x00000008, "CD-ROM Drive" )
	PORT_DIPSETTING(          0x00000000, "Flash ROM" )
	PORT_BIT( 0x000000f0, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* 0xc0 */
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_SPECIAL )
//  PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x00001000, 0x00001000, "Network?" )
	PORT_DIPSETTING(          0x00001000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
//  PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* x76f041 */
//  PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* skip hang at startup */
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* skip hang at startup */
//  PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PCCARD 1 */
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PCCARD 2 */
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_SERVICE1 )
//  PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	PORT_START
	PORT_BIT( 0xffff0000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1) /* serial? */
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)    /* serial? */
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* skip init? */
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_START1 ) /* skip init? */
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2) /* serial? */
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)    /* serial? */
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* skip init? */
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START2 ) /* skip init? */

	/* IN3 */
	PORT_START
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
//  PORT_BIT( 0xf4fffbff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0b000000, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* skip e-Amusement error */
INPUT_PORTS_END

#define SYS573_BIOS_A ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

// BIOS
ROM_START( sys573 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_FILL( 0x0000000, 0x0000224, 0x00 )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )
ROM_END

// Games
ROM_START( konam80s )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826uaa.u1",  0x000000, 0x000224, BAD_DUMP CRC(7d0a3374) SHA1(b23b1cac4fd5368cac866cbd9161717ce7c4b55a) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "gc826uaa", 0, MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( konam80j )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826jaa.u1",  0x000000, 0x000224, BAD_DUMP CRC(91956507) SHA1(ac8769c7d2f6b9dd46fc0b0094d383adea959ff2) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "gc826jaa", 0, MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( darkhleg )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx706jaa.u1",  0x000000, 0x000224, BAD_DUMP CRC(72b42574) SHA1(79dc959f0ce95ccb9ac0dbf0a72aec973e91bc56) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "gx706jaa", 0, MD5(4f096051df039b0d104d4c0fff5dadb8) SHA1(4c8d976096c2da6d01804a44957daf9b50103c90) )
ROM_END

ROM_START( pbballex )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx802jab.u1",  0x000000, 0x000224, BAD_DUMP CRC(ea8bdda3) SHA1(780034ab08871631ef0e3e9b779ca89e016c26a8) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "gx802jab", 0, MD5(52bb53327ba48f87dcb030d5e50fe94f) SHA1(67ddce1ad7e436c18e08d5a8c77f3259dbf30572) )
ROM_END

// System 573 BIOS (we're missing the later version that boots up with a pseudo-GUI)
GAME( 1998, sys573, 0, konami573, konami573, konami573, ROT0, "Konami", "System 573 BIOS", NOT_A_DRIVER )

// Standard System 573 games
GAME( 1998, konam80s, sys573, konami573, konami573, konami573, ROT90, "Konami", "Konami 80s AC Special (US ver UAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, konam80j, konam80s, konami573, konami573, konami573, ROT90, "Konami", "Konami 80s Gallery (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, darkhleg, sys573, konami573, konami573, konami573, ROT0, "Konami", "Dark Horse Legend (Japan ver JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, pbballex, sys573, konami573, konami573, konami573, ROT0, "Konami", "Powerful Pro Baseball EX (Japan ver JAB)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
