/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"

extern unsigned char *williams_videoram;

unsigned char *williams_port_select;
unsigned char *williams_video_counter;
unsigned char *williams_bank_base;

unsigned char *robotron_catch;

unsigned char *stargate_catch;

unsigned char *defender_irq_enable;
unsigned char *defender_catch;
unsigned char *defender_bank_base;
unsigned char *defender_bank_ram;
int defender_bank_select;

unsigned char *splat_catch;

unsigned char *blaster_catch;
unsigned char *blaster_bank2_base;


void williams_sh_w(int offset,int data);
void williams_palette_w(int offset,int data);

/*
 *  int williams_inttable[] = {0,0x40,0xC0,0xff};
 *  int williams_inttable[] = {0,0x24,0x48,0x6c,0x90,0xB4,0xD8,0xff};
 *  int williams_inttable[] = {0,0x55,0xAA,0xff};
 *
 *  MUST be in this order to have the Back Zone not in the playfield
 *  in Joust.
 *
 *  int williams_inttable[] = {0x00,0x00,0x00,0x00};
 *  int williams_inttable[] = {0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xff};
 *
 *  Here is some test that I have done, This is for the value of 0xCB00
 *  It affect the color cycling and/or the speed of the game
 *  0xCB00 is the value of the video counter (6 hi bits)
 *  IRQ is supposed to be at 4 ms interval.
 *  I put IRQ 4 times by frame so we are supposed to have near 4 ms
 *
 *  So, the value of CB00 normally pass all the values from 0 to 0xFC each
 *  frame.
 *  After trying many combination, I found that Stargate have to have
 *  more time than the others games between IRQs, but will work if he see
 *  only 00 and FF at $CB00. So I call IRQ just 2 times by frame.
 *  If I do like the others, IRQ 4 time by frames, I need to boost the
 *  CPU speed, but even pass 2 mhz, the game will freeze if he dont have
 *  time to execute all he is supposed to before the next IRQ.
 *  Same thing with Defender. This thing was observed even on the arcade
 *  board. In defender, on the first wave try to catch more than 7 man,
 *  stop your ship, press DOWN to drop the mens. You will see the
 *  game slowdown. If you had the 10 men, it will freeze for a long
 *  period of time. On the emulator, it crash if there is not enough
 *  instructions executed before the video counter reached a certain
 *  value.
 *  But Robotron will not work if only 0 and FF pass at CB00, he want
 *  more value.With only 0 and FF the color cycling do not work.
 *  Joust is like Robotron, but, the values has to match the real
 *  thing. What I mean is that normally the first IRQ is supposed
 *  to be after the first Quarter of a screen refresh, the second
 *  is supposed to appen at the middle of the screen...
 *  So, the value at CB00 has to follow that rule.
 *  In Joust, when a shape is erased, it is displayed at the new position
 *  just at the next IRQ. So we have a blackout area in the screen.
 *  If the value of CB00 are not synchronized with our computer, this
 *  blackout zone can be in the middle of the screen.
 *  Thats why the values in the table are 0xAA,0xff,0,0x55 and not
 *  0,0x55,0xAA,0xff. It seem to work fine.
 */

static int williams_inttable[] = { 0xaa, 0xff, 0x00, 0x55 };
static int stargate_inttable[] = { 0x00, 0xff, 0x00, 0xff };

static int video_counter_index;
static unsigned char defender_video_counter;

static int blaster_bank;
static int vram_bank;


int defender_io_r (int offset);
void defender_io_w (int offset,int data);
void defender_bank_select_w (int offset,int data);

/***************************************************************************

	Common Williams routines

***************************************************************************/

/*
 *  Initialize the machine
 */

void williams_init_machine (void)
{
	/* set the optimization flags */
	m6809_Flags = M6809_FAST_NONE;
}


void defender_init_machine (void)
{
	/* set the optimization flags */
	m6809_Flags = M6809_FAST_NONE;
	/* initialize the banks */
	defender_bank_select_w (0,0);
}


/*
 *  Update the video counter and call an interrupt
 */

int williams_interrupt (void)
{
	*williams_video_counter = williams_inttable[video_counter_index++ & 0x03];
	return interrupt ();
}


/*
 *  Read either port 0 or 1, depending on williams_port_select
 */

int williams_input_port_0_1 (int offset)
{
	if ((*williams_port_select & 0x1c) == 0x1c)
		return input_port_1_r (0);
	else
		return input_port_0_r (0);
}


/*
 *  Read either port 2 or 3, depending on williams_port_select
 */

int williams_input_port_2_3 (int offset)
{
	if((*williams_port_select & 0x1c) == 0x1c)
	   return input_port_3_r (0);
	else
	   return input_port_2_r (0);
}


/*
 *  Switch between VRAM and ROM
 */

void williams_vram_select_w(int offset,int data)
{
	vram_bank = data;
	if (vram_bank)
	{
		cpu_setbank (1, williams_bank_base);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
	}
}



/***************************************************************************

	Robotron-specific routines

***************************************************************************/

/* JB 970823 - speed up very busy loop in Robotron */
/*    D19B: LDA   $10    ; dp=98
      D19D: CMPA  #$02
      D19F: BCS   $D19B  ; (BLO)   */
int robotron_catch_loop_r (int offset)
{
	unsigned char t = *robotron_catch;
	if (t < 2 && cpu_getpc () == 0xd19d)
		cpu_seticount (0);
	return t;
}



/***************************************************************************

	Stargate-specific routines

***************************************************************************/

int stargate_input_port_0_r(int offset)
{
	int keys, altkeys;

	keys = input_port_0_r (0);
	altkeys = input_port_3_r (0);

	if (altkeys)
	{
		keys |= altkeys;
		if (Machine->memory_region[0][0x9c92] == 0xfd)
		{
			if (keys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (keys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}

int stargate_interrupt (void)
{
	*williams_video_counter = stargate_inttable[video_counter_index++ & 0x03];
	return interrupt ();
}


/* JB 970823 - speed up very busy loop in Stargate */
/*    0011: LDA   $39    ; dp=9c
      0013: BEQ   $0011   */
int stargate_catch_loop_r (int offset)
{
	unsigned char t = *stargate_catch;
	if (t == 0 && cpu_getpc () == 0x0013)
		cpu_seticount (0);
	return t;
}



/***************************************************************************

	Defender-specific routines

***************************************************************************/

int defender_interrupt (void)
{
	defender_video_counter = stargate_inttable[video_counter_index++ & 0x03];

  /*
   *  IRQ only if enabled (0x3C will it work all the time? or should I check for a bit?)
   *  I think I should check for 111 in bits 5-3 but it work anyway
   */

	if (*defender_irq_enable == 0x3c)
		return interrupt();

	return ignore_interrupt();
}


/*
 *  Defender Select a bank
 *  There is just data in bank 0
 */

void defender_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x0c000, 0x0c000, 0x0c000, 0x13000 };
	/* set bank address */
	cpu_setbank(1,&RAM[bank[data&7]]);
	if( bank[data] < 0x10000 ){
		/* i/o area map */
		cpu_setbankhandler_r(1,defender_io_r );
		cpu_setbankhandler_w(1,defender_io_w );
	}else{
		/* bank rom map */
		cpu_setbankhandler_r(1,MRA_BANK1);
		cpu_setbankhandler_w(1,MWA_ROM);
	}
}


/*
 *   Defender Read at C000-CFFF
 */

int defender_input_port_1_r(int offset)
{
	int keys, altkeys;

	keys = readinputport(1);
	altkeys = readinputport(3);

	if (altkeys)
	{
		keys |= altkeys;
		if (Machine->memory_region[0][0xa0bb] == 0xfd)
		{
			if (keys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (keys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}

int defender_io_r (int offset)
{
	if (offset == 0xc00)           /* Buttons IN 0  */
		return input_port_0_r (0);
	if (offset == 0xc04)           /* Buttons IN 1  */
		return defender_input_port_1_r (0);
	if (offset == 0xc06)           /* Buttons IN 2  */
		return input_port_2_r (0);
	if (offset == 0x800)           /* video counter */
		return defender_video_counter;

	/* If not bank 0 then return banked RAM */
	return defender_bank_base[offset];
}


/*
 *  Defender Write at C000-CFFF
 */

void defender_io_w (int offset,int data)
{
	defender_bank_base[offset] = data;

	/* WatchDog */
	if (offset == 0x03fc)
		return;

	/* Palette */
	if (offset < 0x10)
		williams_palette_w (offset, data);

	/* Sound */
	if (offset == 0x0c02)
		williams_sh_w (offset,data);
}


/* JB 970823 - speed up very busy loop in Defender */
/*    E7C3: LDA   $5D    ; dp=a0
      E7C5: BEQ   $E7C3   */
int defender_catch_loop_r(int offset)
{
	unsigned char t = *defender_catch;
	if (t == 0 && cpu_getpc () == 0xe7c5)
		cpu_seticount (0);
	return t;
}



/***************************************************************************

	Splat!-specific routines

***************************************************************************/

/* JB 970823 - speed up very busy loop in Splat */
/*    D04F: LDA   $4B    ; dp=98
      D051: BEQ   $D04F   */
int splat_catch_loop_r(int offset)
{
	unsigned char t = *splat_catch;
	if (t == 0 && cpu_getpc () == 0xd051)
		cpu_seticount (0);
	return t;
}


/***************************************************************************

	Sinistar-specific routines

***************************************************************************/

/*
 *  Sinistar Joystick
 */
int sinistar_input_port_0(int offset)
{
	int i;
	int keys;


/*~~~~~ make it incremental */
	keys = input_port_0_r (0);

	if (keys & 0x04)
		i = 0x40;
	else if (keys & 0x08)
		i = 0xC0;
	else
		i = 0x20;

	if (keys&0x02)
		i += 0x04;
	else if (keys&0x01)
		i += 0x0C;
	else
		i += 0x02;

	return i;
}

/***************************************************************************

	Blaster-specific routines

***************************************************************************/

#if 0 /* the fix for Blaster is more expensive than the loop */
/* JB 970823 - speed up very busy loop in Blaster */
/*    D184: LDA   $00    ; dp=97
      D186: CMPA  #$02
      D188: BCS   $D184  ; (BLO)   */
int blaster_catch_loop_r(int offset)
{
	unsigned char t = *blaster_catch;
	if (t < 2 && cpu_getpc () == 0xd186)
		cpu_seticount (0);
	return t;
}
#endif


/*
 *  Blaster Joystick
 */
int blaster_input_port_0(int offset)
{
	int i;
	int keys;

	keys = input_port_0_r (0);

	if (keys & 0x04)
		i = 0x00;
	else if (keys & 0x08)
		i = 0x80;
	else
		i = 0x40;

	if (keys&0x02)
		i += 0x00;
	else if (keys&0x01)
		i += 0x08;
	else
		i += 0x04;

	return i;
}


/*
 *  Blaster bank select
 */

static int bank[16] = { 0x00000, 0x10000, 0x14000, 0x18000, 0x1c000, 0x20000, 0x24000, 0x28000,
                        0x2c000, 0x30000, 0x34000, 0x38000, 0x2c000, 0x30000, 0x34000, 0x38000 };

void blaster_vram_select_w(int offset,int data)
{
	vram_bank = data;
	if (vram_bank)
	{
		cpu_setbank (1, &RAM[bank[blaster_bank]]);
		cpu_setbank (2, blaster_bank2_base);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
		cpu_setbank (2, williams_videoram + 0x4000);
	}
}

void blaster_bank_select_w(int offset,int data)
{
	blaster_bank = data & 15;
	if (vram_bank)
	{
		cpu_setbank (1, &RAM[bank[blaster_bank]]);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
	}
}
