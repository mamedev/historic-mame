/* ST-V SpeedUp Hacks */

/*
to be honest i think some of these cause more problems than they're worth ...
*/

#include "driver.h"
#include "machine/eeprom.h"

extern data32_t *stv_workram_h;
extern data32_t *stv_backupram;

DRIVER_INIT ( stv );

/* Hack the boot vectors .. not right but allows several IC13 games (which fail the checksums before hacking) to boot */
DRIVER_INIT( ic13 )
{
	/* this is WRONG but works for some games */
	data32_t *rom = (data32_t *)memory_region(REGION_USER1);
	rom[0xf10/4] = (rom[0xf10/4] & 0xff000000)|((rom[0xf10/4]/2)&0x00ffffff);
	rom[0xf20/4] = (rom[0xf20/4] & 0xff000000)|((rom[0xf20/4]/2)&0x00ffffff);
	rom[0xf30/4] = (rom[0xf30/4] & 0xff000000)|((rom[0xf30/4]/2)&0x00ffffff);

	init_stv();
}
/*
EEPROM write 0000 to address 2d
EEPROM write 0000 to address 2e
EEPROM write 0000 to address 2f
EEPROM write 0000 to address 30
EEPROM write ffff to address 31
EEPROM write ffff to address 32
EEPROM write ffff to address 33
EEPROM write ffff to address 34
EEPROM write ffff to address 35
EEPROM write ffff to address 36
EEPROM write ffff to address 37
EEPROM write ffff to address 38
EEPROM write ffff to address 39
EEPROM write ffff to address 3a
EEPROM write ffff to address 3b
EEPROM write ffff to address 3c
EEPROM write ffff to address 3d
EEPROM write ffff to address 3e
EEPROM write ffff to address 3f
*/
/*static data8_t stv_default_eeprom[128] = {
    0x53,0x45,0xff,0xff,0xff,0xff,0x3b,0xe2,
    0x00,0x00,0x00,0x00,0x00,0x02,0x01,0x00,
    0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x08,
    0x08,0xfd,0x10,0x04,0x23,0x2a,0x00,0x00,
    0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0x3b,0xe2,0xff,0xff,0x00,0x00,
    0x00,0x01,0x01,0x00,0x01,0x01,0x00,0x00,
    0x00,0x00,0x00,0x08,0x08,0xfd,0x10,0x04,
    0x23,0x2a,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff
};*/

static data8_t shienryu_default_eeprom[128] = {
	0x53,0x45,0x47,0x41,0x3b,0xe2,0x5e,0x09,
	0x5e,0x09,0x00,0x00,0x00,0x00,0x00,0x02,
	0x01,0x00,0x01,0x01,0x00,0x00,0x00,0x00,
	0x00,0x08,0x18,0xfd,0x18,0x01,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x5e,0x09,0x00,0x00,
	0x00,0x00,0x00,0x02,0x01,0x00,0x01,0x01,
	0x00,0x00,0x00,0x00,0x00,0x08,0x18,0xfd,
	0x18,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

static data8_t *stv_default_eeprom;
static int stv_default_eeprom_length;

NVRAM_HANDLER( stv )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file) EEPROM_load(file);
		else
		{
			if (stv_default_eeprom)	/* Set the EEPROM to Factory Defaults */
				EEPROM_set_data(stv_default_eeprom,stv_default_eeprom_length);
		}
	}
}

/*

06013AE8: MOV.L   @($D4,PC),R5
06013AEA: MOV.L   @($D8,PC),R0
06013AEC: MOV.W   @R5,R5
06013AEE: MOV.L   @R0,R0
06013AF0: AND     R10,R5
06013AF2: TST     R0,R0
06013AF4: BTS     $06013B00
06013AF6: EXTS.W  R5,R5
06013B00: EXTS.W  R5,R5
06013B02: TST     R5,R5
06013B04: BF      $06013B0A
06013B06: TST     R4,R4
06013B08: BT      $06013AE8

   (loops for 375868 instructions)

*/


READ32_HANDLER( stv_speedup_r )
{
	if (activecpu_get_pc()==0x60154b4) cpu_spinuntil_int(); // bios menus..

	return stv_workram_h[0x0335d0/4];
}

READ32_HANDLER( stv_speedup2_r )
{
	if (activecpu_get_pc()==0x6013af0) cpu_spinuntil_int(); // for use in japan

	return stv_workram_h[0x0335bc/4];
}

void install_stvbios_speedups(void)
{
/* idle skip bios? .. not 100% sure this is safe .. we'll see */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60335d0, 0x60335d3, 0, 0, stv_speedup_r );
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60335bc, 0x60335bf, 0, 0, stv_speedup2_r );
}

static READ32_HANDLER( shienryu_slave_speedup_r )
{
 if (activecpu_get_pc()==0x06004410)
  cpu_spinuntil_time(TIME_IN_USEC(20)); // is this safe... we can't skip till vbl because its not a vbl wait loop

 return stv_workram_h[0x0ae8e4/4];
}


static READ32_HANDLER( shienryu_speedup_r )
{
	if (activecpu_get_pc()==0x060041C8) cpu_spinuntil_int(); // after you enable the sound cpu ...
	return stv_workram_h[0x0ae8e0/4];
}


DRIVER_INIT(shienryu)
{
	stv_default_eeprom = shienryu_default_eeprom;
	stv_default_eeprom_length = sizeof(shienryu_default_eeprom);

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ae8e0, 0x60ae8e3, 0, 0, shienryu_speedup_r ); // after you enable sound cpu
	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60ae8e4, 0x60ae8e7, 0, 0, shienryu_slave_speedup_r ); // after you enable sound cpu

	init_stv();
}

static READ32_HANDLER( prikura_speedup_r )
{
	if (activecpu_get_pc()==0x6018642) cpu_spinuntil_int(); // after you enable the sound cpu ...
	return stv_workram_h[0x0b9228/4];
}


DRIVER_INIT(prikura)
{
/*
 06018640: MOV.B   @R14,R0  // 60b9228
 06018642: TST     R0,R0
 06018644: BF      $06018640

    (loops for 263473 instructions)
*/
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60b9228, 0x60b922b, 0, 0, prikura_speedup_r );

	init_stv();
}


static READ32_HANDLER( hanagumi_speedup_r )
{
	if (activecpu_get_pc()==0x06010162) cpu_spinuntil_int(); // title logos

	return stv_workram_h[0x94188/4];
}

static READ32_HANDLER( hanagumi_slave_off )
{
	/* just turn the slave off, i don't think the game needs it */
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);

	return stv_workram_h[0x015438/4];
}

DRIVER_INIT(hanagumi)
{
/*
    06013E1E: NOP
    0601015E: MOV.L   @($6C,PC),R3
    06010160: MOV.L   @R3,R0  (6094188)
    06010162: TST     R0,R0
    06010164: BT      $0601015A
    0601015A: JSR     R14
    0601015C: NOP
    06013E20: MOV.L   @($34,PC),R3
    06013E22: MOV.B   @($00,R3),R0
    06013E24: TST     R0,R0
    06013E26: BT      $06013E1C
    06013E1C: RTS
    06013E1E: NOP

   (loops for 288688 instructions)
*/
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x6094188, 0x609418b, 0, 0, hanagumi_speedup_r );
   	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x6015438, 0x601543b, 0, 0, hanagumi_slave_off );

  	init_stv();
}




/* these idle loops might change if the interrupts change / are fixed because i don't really think these are vblank waits... */

/* puyosun

CPU0: Aids Screen

06021CF0: MOV.B   @($13,GBR),R0 (60ffc13)
06021CF2: CMP/PZ  R0
06021CF4: BT      $06021CF0
   (loops for xxx instructions)

   this is still very slow .. but i don't think it can be sped up further


*/

static READ32_HANDLER( puyosun_speedup_r )
{
	if (activecpu_get_pc()==0x6021CF2) cpu_spinuntil_time(TIME_IN_USEC(400)); // spinuntilint breaks controls again .. urgh


	return stv_workram_h[0x0ffc10/4];
}

static READ32_HANDLER( puyosun_speedup2_r )
{
	/* this is read when the opcode is read .. we can't do much else because the only
     thing the loop checks is an internal sh2 register */

	if (activecpu_get_pc()==0x6023702) cpu_spinuntil_time(TIME_IN_USEC(2000));

//  logerror ("Ugly %08x\n", activecpu_get_pc());

	return stv_workram_h[0x023700/4];
}

DRIVER_INIT(puyosun)
{
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffc10, 0x60ffc13, 0, 0, puyosun_speedup_r ); // idle loop of main cpu
   	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x6023700, 0x6023703, 0, 0, puyosun_speedup2_r ); // UGLY hack for second cpu

	init_ic13();
}

/* mausuke

CPU0 Data East Logo:
060461A0: MOV.B   @($13,GBR),R0  (60ffc13)
060461A2: CMP/PZ  R0
060461A4: BT      $060461A0
   (loops for 232602 instructions)

*/

static READ32_HANDLER( mausuke_speedup_r )
{
	if (activecpu_get_pc()==0x060461A2) cpu_spinuntil_time(TIME_IN_USEC(20)); // spinuntilint breaks controls again .. urgh

	return stv_workram_h[0x0ffc10/4];
}

DRIVER_INIT(mausuke)
{
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffc10, 0x60ffc13, 0, 0, mausuke_speedup_r ); // idle loop of main cpu

	init_ic13();
}

static READ32_HANDLER( cottonbm_speedup_r )
{
	if (activecpu_get_pc()==0x06030EE4) cpu_spinuntil_time(TIME_IN_USEC(20)); // spinuntilint breaks lots of things

	return stv_workram_h[0x0ffc10/4];
}

static READ32_HANDLER( cottonbm_speedup2_r )
{
	/* this is read when the opcode is read .. we can't do much else because the only
     thing the loop checks is an internal sh2 register

     it will fill the log with cpu #1 (PC=06032B52): warning - op-code execute on mapped I/O :/
     */

	if (activecpu_get_pc()==0x6032b52)
	{
		if (
		   (stv_workram_h[0x0ffc44/4] != 0x260fbe34) &&
		   (stv_workram_h[0x0ffc48/4] != 0x260fbe34) &&
		   (stv_workram_h[0x0ffc44/4] != 0x260fbe2c) &&
		   (stv_workram_h[0x0ffc48/4] != 0x260fbe2c)
		   )
		{
			logerror("cpu1 skip %08x %08x\n",stv_workram_h[0x0ffc44/4],stv_workram_h[0x0ffc48/4]);

			cpu_spinuntil_time(TIME_IN_USEC(200));
		}

	}

//  logerror ("Ugly %08x\n", activecpu_get_pc());

	return stv_workram_h[0x032b50/4];
}

DRIVER_INIT(cottonbm)
{
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffc10, 0x60ffc13, 0, 0, cottonbm_speedup_r ); // idle loop of main cpu
   	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x6032b50, 0x6032b53, 0, 0, cottonbm_speedup2_r ); // UGLY hack for second cpu

	init_stv();
}

static READ32_HANDLER( cotton2_speedup_r )
{
	if (activecpu_get_pc()==0x06031c7c) cpu_spinuntil_time(TIME_IN_USEC(20)); // spinuntilint breaks lots of things

	return stv_workram_h[0x0ffc10/4];
}

static READ32_HANDLER( cotton2_speedup2_r )
{
	/* this is read when the opcode is read .. we can't do much else because the only
     thing the loop checks is an internal sh2 register

     it will fill the log with cpu #1 (PC=xxx): warning - op-code execute on mapped I/O :/
     */

	if (activecpu_get_pc()==0x60338ec)
	{
		if (
		   (stv_workram_h[0x0ffc44/4] != 0x260fd264) &&
		   (stv_workram_h[0x0ffc48/4] != 0x260fd264) &&
		   (stv_workram_h[0x0ffc44/4] != 0x260fd25c) &&
		   (stv_workram_h[0x0ffc48/4] != 0x260fd25c)
		   )
		{
			logerror("cpu1 skip %08x %08x\n",stv_workram_h[0x0ffc44/4],stv_workram_h[0x0ffc48/4]);

			cpu_spinuntil_time(TIME_IN_USEC(200));
		}

	}

//  logerror ("Ugly %08x\n", activecpu_get_pc());

	return stv_workram_h[0x0338ec/4];
}

DRIVER_INIT(cotton2)
{
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffc10, 0x60ffc13, 0, 0, cotton2_speedup_r ); // idle loop of main cpu
   	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60338ec, 0x60338ef, 0, 0, cotton2_speedup2_r ); // UGLY hack for second cpu

	init_stv();
}

static READ32_HANDLER( dnmtdeka_speedup_r )
{
	if (activecpu_get_pc()==0x6027c93) cpu_spinuntil_time(TIME_IN_USEC(20));

	return stv_workram_h[0x0985a0/4];
}


DRIVER_INIT(dnmtdeka)
{
//      memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60985a0, 0x60985a3, 0, 0, dnmtdeka_speedup_r ); // idle loop of main cpu

	init_ic13();
}


static READ32_HANDLER( fhboxers_speedup_r )
{
	if (activecpu_get_pc()==0x060041c4) cpu_spinuntil_time(TIME_IN_USEC(20));

	return stv_workram_h[0x00420c/4];
}

static READ32_HANDLER( fhboxers_speedup2_r )
{
	if (activecpu_get_pc()==0x0600bb0c) cpu_spinuntil_time(TIME_IN_USEC(20));


	return stv_workram_h[0x090740/4];
}

/* fhboxers ... doesn't seem to work properly anyway .. even without the speedups, timing issues / interrupt order .. who knows */

DRIVER_INIT(fhboxers)
{
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x600420c, 0x600420f, 0, 0, fhboxers_speedup_r ); // idle loop of main cpu
   	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x6090740, 0x6090743, 0, 0, fhboxers_speedup2_r ); // idle loop of second cpu

	init_ic13();
}


static READ32_HANDLER( bakubaku_speedup_r )
{
	if (activecpu_get_pc()==0x06036dc8) cpu_spinuntil_int(); // title logos

	return stv_workram_h[0x0833f0/4];
}

static READ32_HANDLER( bakubaku_speedup2_r )
{
	if (activecpu_get_pc()==0x06033762) 	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);

	return stv_workram_h[0x0033762/4];
}

static READ32_HANDLER( bakubaku_hangskip_r )
{
	if (activecpu_get_pc()==0x060335e4) stv_workram_h[0x0335e6/4] = 0x32300909;

	return stv_workram_h[0x033660/4];
}

DRIVER_INIT(bakubaku)
{
   	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60833f0, 0x60833f3, 0, 0, bakubaku_speedup_r ); // idle loop of main cpu
   	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60fdfe8, 0x60fdfeb, 0, 0, bakubaku_speedup2_r ); // turn off slave sh2, is it needed after boot ??
   	//memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x6033660, 0x6033663, 0, 0, bakubaku_hangskip_r ); // it waits for a ram address to change what should change it?

	init_ic13();
}

static READ32_HANDLER( groovef_hack1_r )
{
	if(activecpu_get_pc() == 0x6005e7e) stv_workram_h[0x0fffcc/4] = 0x00000000;
//  usrintf_showmessage("1 %08x",activecpu_get_pc());
	return stv_workram_h[0x0fffcc/4];
}

static READ32_HANDLER( groovef_hack2_r )
{
	if(activecpu_get_pc() == 0x6005e88) stv_workram_h[0x0ca6cc/4] = 0x00000000;
//  usrintf_showmessage("2 %08x",activecpu_get_pc());
	return stv_workram_h[0x0ca6cc/4];
}

static READ32_HANDLER( groovef_speedup_r )
{
//  logerror ("groove speedup \n");
	if (activecpu_get_pc()==0x060a4972)
	{
		cpu_spinuntil_int(); // title logos
//      logerror ("groove speedup skipping\n");

	}

	return stv_workram_h[0x0c64ec/4];
}
/*
static READ32_HANDLER( groovef_second_cpu_off_r )
{
    if (activecpu_get_pc()==0x060060c2)     cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
    return 0;
}
*/

static READ32_HANDLER( groovef_second_skip_r )
{
	if (activecpu_get_pc()==0x060060ca) cpu_spinuntil_time(TIME_IN_USEC(200));


	return stv_workram_h[0x0060e0/4];
}

DRIVER_INIT( groovef )
{
	/* prevent game from hanging on startup -- todo: remove these hacks */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ca6cc, 0x60ca6cf, 0, 0, groovef_hack2_r );
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60fffcc, 0x60fffcf, 0, 0, groovef_hack1_r );

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60c64ec, 0x60c64ef, 0, 0, groovef_speedup_r );
//  memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60060dc, 0x60060df, 0, 0, groovef_second_cpu_off_r ); // not a good idea, needs it for ai.
	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60060e0, 0x60060e3, 0, 0, groovef_second_skip_r ); // careful .. its not an interrupt wait loop

	init_stv();
}

/* danchih hangs on the title screen without this hack .. */

/*  info from Saturnin Author

> seems to be fully playable, can you be more specific about the scu level 2
> dma stuff? i'd prefer a real solution than this hack, it could affect
other
> games too for all i know.

Unfortunalely I don't know much more about it : I got this info from a
person
who ran the SGL object files through objdump ...

0x060ffcbc _DMASt_SCU1
0x060ffcbd _DMASt_SCU2

But when I got games looping on thoses locations, the problem was related to
a
SCU interrupt (in indirect mode) which was registered but never triggered, a
bug in my SH2 core prevented the SR bits to be correctly filled in some
cases ...
When the interrupt is correctly triggered, I don't have these loops anymore,
and Hanafuda works without hack now (unless the sound ram one)


*/

static READ32_HANDLER( danchih_hack_r )
{
	if (activecpu_get_pc()==0x06028b2a) return 0x0e0c0000;

	return stv_workram_h[0x0ffcbc/4];
}

DRIVER_INIT( danchih )
{
	/* prevent game from hanging on title screen -- todo: remove these hacks */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffcbc, 0x60ffcbf, 0, 0, danchih_hack_r );

	init_stv();
}

/*
060011AE: AND     #$0F,R0
060011B0: MOV     #$5E,R1
060011B2: ADD     R5,R1
060011B4: MOV.B   R0,@R1
060011B6: MOV     R5,R0
060011B8: ADD     #$70,R0

060011BA: MOV.B   @(R0,R4),R0 <- reads 0x02020000,cause of the crash
060011BC: RTS
060011BE: NOP
060131AA: CMP/EQ  #$01,R0
060131AC: BT      $0601321C
060131AE: CMP/EQ  #$02,R0
060131B0: BT      $0601324A

TODO: understand where it gets 0x02020000,it must be 0x0000000

*/

static READ32_HANDLER( astrass_hack_r )
{
	if(activecpu_get_pc()==0x60011bc) return 0x00000000;

	return stv_workram_h[0x000770/4];
}

DRIVER_INIT( astrass )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000770, 0x6000773, 0, 0, astrass_hack_r );

	init_ic13();
}
