/* Super Kaneko Nova System */
/* Original Driver by Sylvain Glaize */
/* taken to pieces and attempted reconstruction by David Haywood */

/* Credits (in no particular order):
   Olivier Galibert for all the assistance and information he's provided
   R.Belmont for working on the SH2 timers so sound worked
   Nicola Salmoria for hooking up the Roz and improving the dirty tile handling
   Paul Priest for a lot of things
   Stephh for spotting what was wrong with Puzz Loop's inputs
*/

/*
ToDo:
   Priorities
   Transparency effects (each pen has a transparent flag, there are seperate r,g,b alpha values to be used for these pens)
   Idle skipping on other games + bios ..
   Fix gaps between zoomed sprites (See Gals Panic 4 continue screen and timer)
   Fix broken games
   Remove rand() and replace with something that is deterministic within the scope of the emulation

Alpha pens:
   galpanis uses them to white in/out the bg's during attract? Solid white atm.

Priorities:
   Puzzloop appears to use the priorities, the other games might use alpha effects to disable a layer instead ..
     *they don't appear to use alpha, so how should it work, senknow intro, galpanis attract,  cyvern level 4?

Ram-Based Tiles:
   The front layer in galpanic games
   The kaneko logo in galpanis
   The Bios

Sprite Zoom:
   Used all over the place in the Gals Panic games, check the bit in galpani4 attract where if you hammer the button the ballon expands until it bursts.
   Puzz Loop main logo

Roz:
   Should be used on galpani4 bg's, but for some reason incxx,incxy,incyx,incyy are always 1,0,0,1 resp. Maybe a timer problem, input problem or core problem. Who knows?
      Update, rand() was returning a 15-bit number under mingw. Top bit seemed to determine whether it rotated or not.
   To zoom the skull in the senknow attract mode
   To zoom the girl at the end of a level in galpanis
   To zoom the kaneko logo in galpanis

Linescroll:
   Puzzloop intro, the screen covered in 'puzz loop' of varying sizes, the little ones go left and the big ones right.
   Puzzloop title screen uses it just to scroll the bg up.
   Also used behind the hi-score table on puzzloop...
   Galpani4/galpanis use it for 2P mode on both bg A and bg B.

R,G,B Brightness:
   The Electro Design games have this set darker than 0xff for sprites and bg's anyway.
   Galpanis uses it to dim the bg's in attract on the stats screens for the characters (bg's hidden by white pen atm).
Lots of games use them to fade, haven't seen r,g,b used independantly yet but you never know.

etc.etc.

-pjp/dh

gutsn Crash Reason
the routine at 401d8c8 calls 40249e0 with r14 as a parameter, which points where the return pr on the stack is
param is 60ffbc0, pr is at 60ffbcc, but 40249e0 writes more than 12 bytes

should sengekis be rot90 with the sprites flipped?  scrolling is in the wrong direction at the moment

*/

#include "driver.h"
#include <time.h>

#define BIOS_SKIP 1 // Skip Bios as it takes too long and doesn't complete atm.

// Defined in vidhrdw
extern void skns_sprite_kludge(int x, int y);

data32_t *skns_spc_ram, *skns_tilemapA_ram, *skns_tilemapB_ram, *skns_v3slc_ram;
data32_t *skns_palette_ram, *skns_v3t_ram, *skns_main_ram, *skns_cache_ram;
data32_t *skns_pal_regs, *skns_v3_regs, *skns_spc_regs;

data32_t skns_v3t_dirty[0x4000]; // allocate this elsewhere?
data32_t skns_v3t_4bppdirty[0x8000]; // allocate this elsewhere?
int skns_v3t_somedirty,skns_v3t_4bpp_somedirty;

static UINT8 bright_spc_b=0x00, bright_spc_g=0x00, bright_spc_r=0x00;
static UINT8 bright_v3_b=0x00,  bright_v3_g=0x00,  bright_v3_r=0x00;
static int use_spc_bright, use_v3_bright;

WRITE32_HANDLER ( skns_tilemapA_w );
WRITE32_HANDLER ( skns_tilemapB_w );
WRITE32_HANDLER ( skns_v3_regs_w );
VIDEO_START(skns);
VIDEO_UPDATE(skns);

/* hit.c */

static struct {
	UINT16 x1p, y1p, z1p, x1s, y1s, z1s;
	UINT16 x2p, y2p, z2p, x2s, y2s, z2s;
	UINT16 org;

	UINT16 x1_p1, x1_p2, y1_p1, y1_p2, z1_p1, z1_p2;
	UINT16 x2_p1, x2_p2, y2_p1, y2_p2, z2_p1, z2_p2;
	UINT16 x1tox2, y1toy2, z1toz2;
	INT16 x_in, y_in, z_in;
	UINT16 flag;

	UINT8 disconnect;
} hit;

static void hit_calc_orig(UINT16 p, UINT16 s, UINT16 org, UINT16 *l, UINT16 *r)
{
	switch(org & 3) {
	case 0:
		*l = p;
		*r = p+s;
	break;
	case 1:
		*l = p-s/2;
		*r = *l+s;
	break;
	case 2:
		*l = p-s;
		*r = p;
	break;
	case 3:
		*l = p-s;
		*r = p+s;
	break;
	}
}

static void hit_calc_axis(UINT16 x1p, UINT16 x1s, UINT16 x2p, UINT16 x2s, UINT16 org,
			  UINT16 *x1_p1, UINT16 *x1_p2, UINT16 *x2_p1, UINT16 *x2_p2,
			  INT16 *x_in, UINT16 *x1tox2)
{
	UINT16 x1l, x1r, x2l, x2r;
	hit_calc_orig(x1p, x1s, org,      &x1l, &x1r);
	hit_calc_orig(x2p, x2s, org >> 8, &x2l, &x2r);

	*x1tox2 = x2p-x1p;
	*x1_p1 = x1p;
	*x2_p1 = x2p;
	*x1_p2 = x1r;
	*x2_p2 = x2l;
	*x_in = x1r-x2l;
}

static void hit_recalc(void)
{
	hit_calc_axis(hit.x1p, hit.x1s, hit.x2p, hit.x2s, hit.org,
		&hit.x1_p1, &hit.x1_p2, &hit.x2_p1, &hit.x2_p2,
		&hit.x_in, &hit.x1tox2);
	hit_calc_axis(hit.y1p, hit.y1s, hit.y2p, hit.y2s, hit.org,
		&hit.y1_p1, &hit.y1_p2, &hit.y2_p1, &hit.y2_p2,
		&hit.y_in, &hit.y1toy2);
	hit_calc_axis(hit.z1p, hit.z1s, hit.z2p, hit.z2s, hit.org,
		&hit.z1_p1, &hit.z1_p2, &hit.z2_p1, &hit.z2_p2,
		&hit.z_in, &hit.z1toz2);

	hit.flag = 0;
	hit.flag |= hit.y2p > hit.y1p ? 0x8000 : hit.y2p == hit.y1p ? 0x4000 : 0x2000;
	hit.flag |= hit.y_in >= 0 ? 0 : 0x1000;
	hit.flag |= hit.x2p > hit.x1p ? 0x0800 : hit.x2p == hit.x1p ? 0x0400 : 0x0200;
	hit.flag |= hit.x_in >= 0 ? 0 : 0x0100;
	hit.flag |= hit.z2p > hit.z1p ? 0x0080 : hit.z2p == hit.z1p ? 0x0040 : 0x0020;
	hit.flag |= hit.z_in >= 0 ? 0 : 0x0010;
	hit.flag |= hit.x_in >= 0 && hit.y_in >= 0 && hit.z_in >= 0 ? 8 : 0;
	hit.flag |= hit.z_in >= 0 && hit.x_in >= 0                  ? 4 : 0;
	hit.flag |= hit.y_in >= 0 && hit.z_in >= 0                  ? 2 : 0;
	hit.flag |= hit.x_in >= 0 && hit.y_in >= 0                  ? 1 : 0;
/*  if(0)
    log_event("HIT", "Recalc, (%d,%d)-(%d,%d)-(%d,%d):(%d,%d)-(%d,%d)-(%d,%d):%04x, (%d,%d,%d), %04x",
	      hit.x1p, hit.x1s, hit.y1p, hit.y1s, hit.z1p, hit.z1s,
	      hit.x2p, hit.x2s, hit.y2p, hit.y2s, hit.z2p, hit.z2s,
	      hit.org,
	      hit.x_in, hit.y_in, hit.z_in, hit.flag);
*/
}

WRITE32_HANDLER ( skns_hit_w )
//void hit_w(UINT32 adr, UINT32 data, int type)
{
	int adr = offset * 4;

	switch(adr) {
	case 0x00:
	case 0x28:
		hit.x1p = data;
	break;
	case 0x08:
	case 0x30:
		hit.y1p = data;
	break;
	case 0x38:
	case 0x50:
		hit.z1p = data;
	break;
	case 0x04:
	case 0x2c:
		hit.x1s = data;
	break;
	case 0x0c:
	case 0x34:
		hit.y1s = data;
	break;
	case 0x3c:
	case 0x54:
		hit.z1s = data;
	break;
	case 0x10:
	case 0x58:
		hit.x2p = data;
	break;
	case 0x18:
	case 0x60:
		hit.y2p = data;
	break;
	case 0x20:
	case 0x68:
		hit.z2p = data;
	break;
	case 0x14:
	case 0x5c:
		hit.x2s = data;
	break;
	case 0x1c:
	case 0x64:
		hit.y2s = data;
	break;
	case 0x24:
	case 0x6c:
		hit.z2s = data;
	break;
	case 0x70:
		hit.org = data;
	break;
	default:
//		log_write("HIT", adr, data, type);
	break;
	}
	hit_recalc();
}

WRITE32_HANDLER ( skns_hit2_w )
{
//	log_event("HIT", "Set disconnect to %02x", data);
	hit.disconnect = data;
}


READ32_HANDLER( skns_hit_r )
//UINT32 hit_r(UINT32 adr, int type)
{
	int adr = offset *4;

//	log_read("HIT", adr, type);

	if(hit.disconnect)
		return 0x0000;
	switch(adr) {
	case 0x28:
	case 0x2a:
		return (UINT16)((rand()&0xff)|((rand()&0xff)<<8));
	case 0x00:
	case 0x10:
		return (UINT16)hit.x_in;
	case 0x04:
	case 0x14:
		return (UINT16)hit.y_in;
	case 0x18:
		return (UINT16)hit.z_in;
	case 0x08:
	case 0x1c:
		return hit.flag;
	case 0x40:
		return hit.x1p;
	case 0x48:
		return hit.y1p;
	case 0x50:
		return hit.z1p;
	case 0x44:
		return hit.x1s;
	case 0x4c:
		return hit.y1s;
	case 0x54:
		return hit.z1s;
	case 0x58:
		return hit.x2p;
	case 0x60:
		return hit.y2p;
	case 0x68:
		return hit.z2p;
	case 0x5c:
		return hit.x2s;
	case 0x64:
		return hit.y2s;
	case 0x6c:
		return hit.z2s;
	case 0x70:
		return hit.org;
	case 0x80:
		return hit.x1tox2;
	case 0x84:
		return hit.y1toy2;
	case 0x88:
		return hit.z1toz2;
	case 0x90:
		return hit.x1_p1;
	case 0xa0:
		return hit.y1_p1;
	case 0xb0:
		return hit.z1_p1;
	case 0x98:
		return hit.x1_p2;
	case 0xa8:
		return hit.y1_p2;
	case 0xb8:
		return hit.z1_p2;
	case 0x94:
		return hit.x2_p1;
	case 0xa4:
		return hit.y2_p1;
	case 0xb4:
		return hit.z2_p1;
	case 0x9c:
		return hit.x2_p2;
	case 0xac:
		return hit.y2_p2;
	case 0xbc:
		return hit.z2_p2;
	default:
//		log_read("HIT", adr, type);
	return 0;
	}
}

/* end hit.c */


/* start old driver code */


static void interrupt_callback(int param)
{
	cpu_set_irq_line(0,param,HOLD_LINE);
}

static MACHINE_INIT(skns)
{
	timer_pulse(TIME_IN_MSEC(2), 15, interrupt_callback);
	timer_pulse(TIME_IN_MSEC(8), 11, interrupt_callback);
	timer_pulse(TIME_IN_CYCLES(0,1824), 9, interrupt_callback);

	cpu_setbank(1,memory_region(REGION_USER1));
}


static INTERRUPT_GEN(skns_interrupt)
{
	UINT8 interrupt = 5;
	switch(cpu_getiloops())
	{
		case 0:
			interrupt = 5; // VBLANK
			break;
		case 1:
			interrupt = 1; // SPC
			break;
	}
	cpu_set_irq_line(0,interrupt,HOLD_LINE);
}

INPUT_PORTS_START( skns )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* Paddle A */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER1, 100, 15, 0, 0 )

	PORT_START  /* Paddle B */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 100, 15, 0, 0 )

	PORT_START  /* Paddle C */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER3, 100, 15, 0, 0 )

	PORT_START  /* DSW */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR(Flip_Screen) ) // This port affects 0x040191c8 function
	PORT_DIPSETTING(    0x02, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x04, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x08, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x10, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x20, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x40, 0x40, "Use Backup Ram" )
	PORT_DIPSETTING(    0x00, DEF_STR(No) )
	PORT_DIPSETTING(    0x40, DEF_STR(Yes) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x00, "Freezes the game")
	PORT_DIPSETTING(    0x80, "Right value")

	PORT_START  /* Paddle D */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER4, 100, 15, 0, 0 )

INPUT_PORTS_END


static READ32_HANDLER( nova_input_port_0_r )
{
	return readinputport(0)<<24 | readinputport(1)<<16 | readinputport(2)<<8 | readinputport(3);
}

static READ32_HANDLER( nova_input_port_3_r )
{
	return readinputport(8) | 0xffffff00;
}

static READ32_HANDLER( nova_input_port_dip_r )
{
	return readinputport(4)<<24 | readinputport(5)<<16 | readinputport(6)<<8 | readinputport(7);
}

static UINT32 timer_0_temp[4];

static WRITE32_HANDLER( msm6242_w )
{
	COMBINE_DATA(&timer_0_temp[offset]);

	if(offset>=4)
	{
		printf("Timer 0 outbound\n");
		return;
	}
}

static WRITE32_HANDLER( skns_io_w )
{
	switch(offset) {
	case 2:
		if(((mem_mask & 0xff000000) == 0))
		{ /* Coin Lock/Count */
//			coin_counter_w(0, data & 0x01000000);
//			coin_counter_w(1, data & 0x02000000);
//			coin_lockout_w(0, ~data & 0x04000000);
//			coin_lockout_w(1, ~data & 0x08000000); // Works in puzzloop, others behave strange.
		}
		if(((mem_mask & 0x00ff0000) == 0))
		{ /* Analogue Input Select */
		}
		if(((mem_mask & 0x0000ff00) == 0))
		{ /* Extended Output - Port A, Mahjong inputs, Comms etc. */
		}
		if(((mem_mask & 0x000000ff) == 0))
		{ /* Extended Output - Port B */
		}
	break;
	case 3:
		if(((mem_mask & 0x0000ff00) == 0))
		{ /* Interrupt Clear, do we need these? */
/*			if(data&0x01)
				cpu_set_irq_line(0,1,CLEAR_LINE);
			if(data&0x02)
				cpu_set_irq_line(0,3,CLEAR_LINE);
			if(data&0x04)
				cpu_set_irq_line(0,5,CLEAR_LINE);
			if(data&0x08)
				cpu_set_irq_line(0,7,CLEAR_LINE);
			if(data&0x10)
				cpu_set_irq_line(0,9,CLEAR_LINE);
			if(data&0x20)
				cpu_set_irq_line(0,0xb,CLEAR_LINE);
			if(data&0x40)
				cpu_set_irq_line(0,0xd,CLEAR_LINE);
			if(data&0x80)
				cpu_set_irq_line(0,0xf,CLEAR_LINE);*/
		}
		else
		{
			logerror("Unk IO Write memmask:%08x offset:%08x data:%08x\n", mem_mask, offset, data);
		}
	break;
	default:
		logerror("Unk IO Write memmask:%08x offset:%08x data:%08x\n", mem_mask, offset, data);
	break;
	}
}

static READ32_HANDLER( msm6242_r )
{
	struct tm *tm;
	time_t tms;
	long value;

	time(&tms);
	tm = localtime(&tms);

	// The clock is not y2k-compatible, wrap back 10 years, screw the leap years
	//  tm->tm_year -= 10;

	switch(offset) {
	case 0:
		value  = (tm->tm_sec % 10)<<24;
		value |= (tm->tm_sec / 10)<<16;
		value |= (tm->tm_min % 10)<<8;
		value |= (tm->tm_min / 10);
		break;
	case 1:
		value  = (tm->tm_hour % 10)<<24;
		value |= ((tm->tm_hour / 10) /*| (tm->tm_hour >= 12 ? 4 : 0)*/)<<16;
		value |= (tm->tm_mday % 10)<<8;
		value |= (tm->tm_mday / 10);
		break;
	case 2:
		value  = ((tm->tm_mon + 1) % 10)<<24;
		value |= ((tm->tm_mon + 1) / 10)<<16;
		value |= (tm->tm_year % 10)<<8;
		value |= ((tm->tm_year / 10) % 10);
		break;
	case 3:
	default:
		value  = (tm->tm_wday)<<24;
		value |= (1)<<16;
		value |= (6)<<8;
		value |= (4);
		break;
	}
	return value;
}

/* end old driver code */
static void palette_set_rgb_brightness (int offset, UINT8 brightness_r, UINT8 brightness_g, UINT8 brightness_b)
{
	int use_bright, r, g, b;

	b = ((skns_palette_ram[offset] >> 0  ) & 0x1f);
	g = ((skns_palette_ram[offset] >> 5  ) & 0x1f);
	r = ((skns_palette_ram[offset] >> 10  ) & 0x1f);

	if(offset<(0x40*256)) { // 1st half is for Sprites
		use_bright = use_spc_bright;
	} else { // V3 bg's
		use_bright = use_v3_bright;
	}

	if(use_bright) {
		if(brightness_b) b = ((b<<3) * (brightness_b+1))>>8;
		else b = 0;
		if(brightness_g) g = ((g<<3) * (brightness_g+1))>>8;
		else g = 0;
		if(brightness_r) r = ((r<<3) * (brightness_r+1))>>8;
		else r = 0;
	} else {
		b <<= 3;
		g <<= 3;
		r <<= 3;
	}

	palette_set_color(offset,r,g,b);
}

// This ignores the alpha values atm.
static WRITE32_HANDLER ( skns_pal_regs_w )
{
	int spc_changed=0, v3_changed=0, i;

	COMBINE_DATA(&skns_pal_regs[offset]);

	switch ( offset )
	{
	case (0x00/4): // RWRA0
		if( use_spc_bright != (data&1) ) {
			use_spc_bright = data&1;
			spc_changed = 1;
		}
		break;
	case (0x04/4): // RWRA1
		if( bright_spc_g != (data&0xff) ) {
			bright_spc_g = data&0xff;
			spc_changed = 1;
		}
		break;
	case (0x08/4): // RWRA2
		if( bright_spc_r != (data&0xff) ) {
			bright_spc_r = data&0xff;
			spc_changed = 1;
		}
		break;
	case (0x0C/4): // RWRA3
		if( bright_spc_b != (data&0xff) ) {
			bright_spc_b = data&0xff;
			spc_changed = 1;
		}
		break;

	case (0x10/4): // RWRB0
		if( use_v3_bright != (data&1) ) {
			use_v3_bright = data&1;
			v3_changed = 1;
		}
		break;
	case (0x14/4): // RWRB1
		if( bright_v3_g != (data&0xff) ) {
			bright_v3_g = data&0xff;
			v3_changed = 1;
		}
		break;
	case (0x18/4): // RWRB2
		if( bright_v3_r != (data&0xff) ) {
			bright_v3_r = data&0xff;
			v3_changed = 1;
		}
		break;
	case (0x1C/4): // RWRB3
		if( bright_v3_b != (data&0xff) ) {
			bright_v3_b = data&0xff;
			v3_changed = 1;
		}
		break;
	}

	if(spc_changed)
		for(i=0; i<=((0x40*256)-1); i++)
			palette_set_rgb_brightness (i, bright_spc_r, bright_spc_g, bright_spc_b);

	if(v3_changed)
		for(i=(0x40*256); i<=((0x80*256)-1); i++)
			palette_set_rgb_brightness (i, bright_v3_r, bright_v3_g, bright_v3_b);
}


static WRITE32_HANDLER ( skns_palette_ram_w )
{
	int r,g,b;
	int brightness_r, brightness_g, brightness_b;
	int use_bright;

	COMBINE_DATA(&skns_palette_ram[offset]);

	b = ((skns_palette_ram[offset] >> 0  ) & 0x1f);
	g = ((skns_palette_ram[offset] >> 5  ) & 0x1f);
	r = ((skns_palette_ram[offset] >> 10  ) & 0x1f);

	if(offset<(0x40*256)) { // 1st half is for Sprites
		use_bright = use_spc_bright;
		brightness_b = bright_spc_b;
		brightness_g = bright_spc_g;
		brightness_r = bright_spc_r;
	} else { // V3 bg's
		use_bright = use_v3_bright;
		brightness_b = bright_v3_b;
		brightness_g = bright_v3_g;
		brightness_r = bright_v3_r;
	}

	if(use_bright) {
		if(brightness_b) b = ((b<<3) * (brightness_b+1))>>8;
		else b = 0;
		if(brightness_g) g = ((g<<3) * (brightness_g+1))>>8;
		else g = 0;
		if(brightness_r) r = ((r<<3) * (brightness_r+1))>>8;
		else r = 0;
	} else {
		b <<= 3;
		g <<= 3;
		r <<= 3;
	}

	palette_set_color(offset,r,g,b);
}

static WRITE32_HANDLER( skns_v3t_w )
{
	data8_t *btiles = memory_region(REGION_GFX3);

	COMBINE_DATA(&skns_v3t_ram[offset]);

	skns_v3t_dirty[offset/0x40] = 1;
	skns_v3t_somedirty = 1;
	skns_v3t_4bppdirty[offset/0x20] = 1;
	skns_v3t_4bpp_somedirty = 1;

	data = skns_v3t_ram[offset];
// i think we need to swap around to decode .. endian issues?
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;
}

static WRITE32_HANDLER( skns_ymz280_w )
{
	if ((mem_mask & 0xff000000) == 0)
		YMZ280B_register_0_w(offset,(data >> 24) & 0xff);
	if ((mem_mask & 0x00ff0000) == 0)
		YMZ280B_data_0_w(offset,(data >> 16) & 0xff);
}

static MEMORY_READ32_START( skns_readmem )
	{ 0x00000000, 0x0007ffff, MRA32_ROM       }, /* BIOS ROM */
	{ 0x00400000, 0x00400003, nova_input_port_0_r } ,
	{ 0x00400004, 0x00400007, nova_input_port_dip_r } ,
	{ 0x00400008, 0x0040000b, MRA32_RAM } ,
	/* In between is write only */
	{ 0x0040000c, 0x0040000f, nova_input_port_3_r } ,
	{ 0x00800000, 0x00801fff, MRA32_RAM       }, /* 'backup' RAM */
//	{ 0x00c00000, 0x00c00003, skns_ymz280_r   }, /* ymz280 (sound) */
	{ 0x01000000, 0x0100000f, msm6242_r       } ,
	{ 0x02000000, 0x02003fff, MRA32_RAM       }, /* 'spc' RAM */
	{ 0x02100000, 0x0210003f, MRA32_RAM       }, /* 'spc' */
	{ 0x02400000, 0x0240007f, MRA32_RAM       }, /* 'v3' */
    { 0x02500000, 0x02507fff, MRA32_RAM       }, /* 'v3tbl' RAM */
    { 0x02600000, 0x02607fff, MRA32_RAM       }, /* 'v3slc' RAM */
	{ 0x02a00000, 0x02a0001f, MRA32_RAM       }, /* skns_pal_regs */
    { 0x02a40000, 0x02a5ffff, MRA32_RAM       }, /* 'palette' RAM */
	{ 0x02f00000, 0x02f000ff, skns_hit_r      }, /* hit */
	{ 0x04000000, 0x041fffff, MRA32_BANK1     }, /* GAME ROM */
	{ 0x04800000, 0x0483ffff, MRA32_RAM       }, /* 'v3t' RAM */
	{ 0x06000000, 0x060fffff, MRA32_RAM       }, /* 'main' RAM */
	{ 0xc0000000, 0xc0000fff, MRA32_RAM       }, /* 'cache' RAM */
MEMORY_END

static MEMORY_WRITE32_START( skns_writemem )
	{ 0x00000000, 0x0007ffff, MWA32_ROM }, /* BIOS ROM */
	{ 0x00400000, 0x0040000f, skns_io_w }, /* I/O Write */
	{ 0x00800000, 0x00801fff, MWA32_RAM, (data32_t **)&generic_nvram, &generic_nvram_size }, /* 'backup' RAM */
	{ 0x00c00000, 0x00c00003, skns_ymz280_w }, /* ymz280_w (sound) */
	{ 0x01000000, 0x0100000f, msm6242_w } ,
	{ 0x01800000, 0x01800003, skns_hit2_w },
	{ 0x02000000, 0x02003fff, MWA32_RAM, &skns_spc_ram }, /* sprite ram */
	{ 0x02100000, 0x0210003f, MWA32_RAM, &skns_spc_regs  }, /* sprite registers */
	{ 0x02400000, 0x0240007f, skns_v3_regs_w, &skns_v3_regs }, /* tilemap registers */
	{ 0x02500000, 0x02503fff, skns_tilemapA_w, &skns_tilemapA_ram }, /* tilemap A */
	{ 0x02504000, 0x02507fff, skns_tilemapB_w, &skns_tilemapB_ram }, /* tilemap B */
	{ 0x02600000, 0x02607fff, MWA32_RAM, &skns_v3slc_ram }, /* tilemap linescroll */
	{ 0x02a00000, 0x02a0001f, skns_pal_regs_w, &skns_pal_regs },
	{ 0x02a40000, 0x02a5ffff, skns_palette_ram_w, &skns_palette_ram },
	{ 0x02f00000, 0x02f000ff, skns_hit_w },
	{ 0x04000000, 0x041fffff, MWA32_ROM }, /* GAME ROM */
	{ 0x04800000, 0x0483ffff, skns_v3t_w, &skns_v3t_ram }, /* tilemap b ram based tiles */
	{ 0x06000000, 0x060fffff, MWA32_RAM, &skns_main_ram },
	{ 0xc0000000, 0xc0000fff, MWA32_RAM, &skns_cache_ram }, /* 'cache' RAM */
MEMORY_END

/***** GFX DECODE *****/

static struct GfxLayout skns_tilemap_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128,
	  8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*16*8
};

static struct GfxLayout skns_4bpptilemap_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3  },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
	 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*16*4
};

static struct GfxDecodeInfo skns_bg_decode[] =
{
   /* REGION_GFX1 is sprites, RLE encoded */
	{ REGION_GFX2, 0, &skns_tilemap_layout, 0x000, 256 },
	{ REGION_GFX3, 0, &skns_tilemap_layout, 0x000, 256 },
	{ REGION_GFX2, 0, &skns_4bpptilemap_layout, 0x000, 256 },
	{ REGION_GFX3, 0, &skns_4bpptilemap_layout, 0x000, 256 },
	{ -1 }
};

/***** MACHINE DRIVER *****/

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 33333333/2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }	// irq ?
};

static MACHINE_DRIVER_START(skns)
	MDRV_CPU_ADD(SH2,28638000)
	MDRV_CPU_MEMORY(skns_readmem,skns_writemem)
	MDRV_CPU_VBLANK_INT(skns_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(skns)
	MDRV_NVRAM_HANDLER(generic_1fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16,64*16)
	MDRV_VISIBLE_AREA(0,319,0,239)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_GFXDECODE(skns_bg_decode)

	MDRV_VIDEO_START(skns)
	MDRV_VIDEO_UPDATE(skns)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END

/***** BIOS SKIPPING *****/

static READ32_HANDLER( bios_skip_r )
{
#if BIOS_SKIP
	if (activecpu_get_pc()==0x6fa) cpu_writemem32bedw(0x06000029,1);
#endif
	return skns_main_ram[0x00028/4];
}

/***** IDLE SKIPPING *****/

static READ32_HANDLER( cyvern_speedup_r )
{
	if (activecpu_get_pc()==0x402EBD4) cpu_spinuntil_int();
	return skns_main_ram[0x4d3c8/4];
}

static READ32_HANDLER( puzloopj_speedup_r )
{
	if (activecpu_get_pc()==0x401dca2) cpu_spinuntil_int();
	return skns_main_ram[0x86714/4];
}

static READ32_HANDLER( puzzloop_speedup_r )
{
/*
	0401DA12: MOV.L   @($80,PC),R1
	0401DA14: MOV.L   @R1,R0 (R1=0x6081d38)
	0401DA16: TST     R0,R0
	0401DA18: BF      $0401DA26
	0401DA26: BRA     $0401DA12
*/
	if (activecpu_get_pc()==0x401da16) cpu_spinuntil_int();
	return skns_main_ram[0x81d38/4];
}

static READ32_HANDLER( senknow_speedup_r )
{
	if (activecpu_get_pc()==0x4017dd0) cpu_spinuntil_int();
	return skns_main_ram[0x0000dc/4];
}

static READ32_HANDLER( teljan_speedup_r )
{
	if (activecpu_get_pc()==0x401ba34) cpu_spinuntil_int();
	return skns_main_ram[0x002fb4/4];
}

static READ32_HANDLER( jjparads_speedup_r )
{
	if (activecpu_get_pc()==0x4015e86) cpu_spinuntil_int();
	return skns_main_ram[0x000994/4];
}

static READ32_HANDLER( jjparad2_speedup_r )
{
	if (activecpu_get_pc()==0x401620c) cpu_spinuntil_int();
	return skns_main_ram[0x000984/4];
}

static READ32_HANDLER( ryouran_speedup_r )
{
	if (activecpu_get_pc()==0x40182d0) cpu_spinuntil_int();
	return skns_main_ram[0x000a14/4];
}

static READ32_HANDLER( galpans2_speedup_r )
{
	if (activecpu_get_pc()==0x4049ae4) cpu_spinuntil_int();
	return skns_main_ram[0x0fb6bc/4];
}

static READ32_HANDLER( panicstr_speedup_r )
{
	if (activecpu_get_pc()==0x404e68c) cpu_spinuntil_int();
	return skns_main_ram[0x0f19e4/4];
}

static READ32_HANDLER( sengekis_speedup_r ) // 60006ee  600308e
{
	if (activecpu_get_pc()==0x60006ee) cpu_spinuntil_int();
	return skns_main_ram[0xb7380/4];
}

static DRIVER_INIT( skns )     { install_mem_read32_handler(0, 0x6000028, 0x600002b, bios_skip_r );  }
static DRIVER_INIT( galpani4 ) { skns_sprite_kludge(-5,-1); init_skns();  } // Idle Loop caught by sh-2 core
static DRIVER_INIT( galpanis ) { skns_sprite_kludge(-5,-1); init_skns();  } // Idle Loop caught by sh-2 core
static DRIVER_INIT( cyvern )   { skns_sprite_kludge(+0,+2); init_skns(); install_mem_read32_handler(0, 0x604d3c8, 0x604d3cb, cyvern_speedup_r );   }
static DRIVER_INIT( galpans2 ) { skns_sprite_kludge(-1,-1); init_skns(); install_mem_read32_handler(0, 0x60fb6bc, 0x60fb6bf, galpans2_speedup_r );  }
static DRIVER_INIT( panicstr ) { skns_sprite_kludge(-1,-1); init_skns(); install_mem_read32_handler(0, 0x60f19e4, 0x60f19e7, panicstr_speedup_r );  }
static DRIVER_INIT( senknow )  { skns_sprite_kludge(+1,+1); init_skns(); install_mem_read32_handler(0, 0x60000dc, 0x60000df, senknow_speedup_r );  }
static DRIVER_INIT( puzzloop ) { skns_sprite_kludge(-9,-1); init_skns(); install_mem_read32_handler(0, 0x6081d38, 0x6081d3b, puzzloop_speedup_r ); }
static DRIVER_INIT( puzloopj ) { skns_sprite_kludge(-9,-1); init_skns(); install_mem_read32_handler(0, 0x6086714, 0x6086717, puzloopj_speedup_r ); }
static DRIVER_INIT( jjparads ) { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6000994, 0x6000997, jjparads_speedup_r );  }
static DRIVER_INIT( jjparad2 ) { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6000984, 0x6000987, jjparad2_speedup_r );  }
static DRIVER_INIT( ryouran )  { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6000a14, 0x6000a17, ryouran_speedup_r );  }
static DRIVER_INIT( teljan )   { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6002fb4, 0x6002fb7, teljan_speedup_r );  }

static DRIVER_INIT( sarukani ) { skns_sprite_kludge(-1,-1); init_skns();  }
static DRIVER_INIT( gutsn )    { skns_sprite_kludge(+0,+0); init_skns();  } // Not Working yet
static DRIVER_INIT( sengekis ) { skns_sprite_kludge(-192,-272); init_skns(); install_mem_read32_handler(0, 0x60b7380, 0x60b7383, sengekis_speedup_r ); } // Not Working yet


/***** ROM LOADING *****/

ROM_START( skns )
	ROM_REGION( 0x0100000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* Japan BIOS */
	ROM_LOAD       ( "ksns-bio.eur", 0x080000, 0x080000, 0xe2b9d7d1 ) /* Europ BIOS */
ROM_END

ROM_START( cyvern )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "cvj-even.u10", 0x000000, 0x100000, 0x802fadb4 )
	ROM_LOAD16_BYTE( "cvj-odd.u8",   0x000001, 0x100000, 0xf8a0fbdd )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "cv100-00.u24", 0x000000, 0x400000, 0xcd4ae88a )
	ROM_LOAD( "cv101-00.u20", 0x400000, 0x400000, 0xa6cb3f0b )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles Plane A */
	ROM_LOAD( "cv200-00.u16", 0x000000, 0x400000, 0xddc8c67e )
	ROM_LOAD( "cv201-00.u13", 0x400000, 0x400000, 0x65863321 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "cv210-00.u18", 0x400000, 0x400000, 0x7486bf3a )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "cv300-00.u4", 0x000000, 0x400000, 0xfbeda465 )
ROM_END

ROM_START( galpani4 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gp4j1.u10",    0x000000, 0x080000, 0x919a3893 )
    ROM_LOAD16_BYTE( "gp4j1.u08",    0x000001, 0x080000, 0x94cb1fb7 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
    ROM_LOAD( "gp410000.u24", 0x000000, 0x200000, 0x1df61f01 )
    ROM_LOAD( "gp410100.u20", 0x200000, 0x100000, 0x8e2c9349 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gp420000.u16", 0x000000, 0x200000, 0xf0781376 )
    ROM_LOAD( "gp420100.u18", 0x200000, 0x200000, 0x10c4b183 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "gp430000.u4", 0x000000, 0x200000, 0x8374663a )
ROM_END

ROM_START( galpanis )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gps-e.u10",    0x000000, 0x100000, 0xc6938c3f )
    ROM_LOAD16_BYTE( "gps-o.u8",     0x000001, 0x100000, 0xe764177a )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "gps10000.u24", 0x000000, 0x400000, 0xa1a7acf2 )
    ROM_LOAD( "gps10100.u20", 0x400000, 0x400000, 0x49f764b6 )
    ROM_LOAD( "gps10200.u17", 0x800000, 0x400000, 0x51980272 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gps20000.u16", 0x000000, 0x400000, 0xc146a09e )
    ROM_LOAD( "gps20100.u13", 0x400000, 0x400000, 0x9dfa2dc6 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "gps30000.u4", 0x000000, 0x400000, 0x9e4da8e3 )
ROM_END

ROM_START( galpans2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gps2j.u6",     0x000000, 0x100000, 0x6e74005b )
    ROM_LOAD16_BYTE( "gps2j.u4",     0x000001, 0x100000, 0x9b4b2304 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "gs210000.u21", 0x000000, 0x400000, 0x294b2f14 )
    ROM_LOAD( "gs210100.u20", 0x400000, 0x400000, 0xf75c5a9a )
    ROM_LOAD( "gs210200.u8",  0x800000, 0x400000, 0x25b4f56b )
    ROM_LOAD( "gs210300.u32", 0xc00000, 0x400000, 0xdb6d4424 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gs220000.u17", 0x000000, 0x400000, 0x5caae1c0 )
    ROM_LOAD( "gs220100.u9",  0x400000, 0x400000, 0x8d51f197 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "gs221000.u3",  0x400000, 0x400000, 0x58800a18 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
      ROM_LOAD( "gs230000.u1",  0x000000, 0x400000, 0x0348e8e1 )
ROM_END

ROM_START( gutsn )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gts000j0.u6"  ,0x000000, 0x080000, 0x8ee91310 )
    ROM_LOAD16_BYTE( "gts001j0.u4",  0x000001, 0x080000, 0x80b8ee66 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
    ROM_LOAD( "gts10000.u24", 0x000000, 0x400000, 0x1959979e )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gts20000.u16", 0x000000, 0x400000, 0xc443aac3 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "gts30000.u4", 0x000000, 0x400000, 0x8c169141 )
ROM_END

ROM_START( panicstr )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "ps1000j0.u10", 0x000000, 0x100000, 0x59645f89 )
    ROM_LOAD16_BYTE( "ps1001j0.u8",  0x000001, 0x100000, 0xc4722be9 )

  	ROM_REGION( 0x800000, REGION_GFX1, 0 )
    ROM_LOAD( "ps-10000.u24", 0x000000, 0x400000, 0x294b2f14 )
    ROM_LOAD( "ps110100.u20", 0x400000, 0x400000, 0xe292f393 )

  	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "ps120000.u16", 0x000000, 0x400000, 0xd772ac15 )

  	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

  	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "ps-30000.u4",  0x000000, 0x400000, 0x2262e263 )
ROM_END

ROM_START( puzzloop )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "ksns-bio.eur", 0x000000, 0x080000, 0xe2b9d7d1 ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "puzloope.u6",  0x000000, 0x080000, 0x273adc38 )
	ROM_LOAD16_BYTE( "puzloope.u4",  0x000001, 0x080000, 0x14ac2870 )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "pzl10000.u24", 0x000000, 0x400000, 0x35bf6897 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pzl20000.u16", 0x000000, 0x400000, 0xff558e68 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "pzl21000.u18", 0x400000, 0x400000, 0xc8b3be64 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "pzl30000.u4", 0x000000, 0x400000, 0x38604b8d )
ROM_END

ROM_START( puzloopj )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "pl0j2u6.u10",  0x000000, 0x080000, 0x23c3bf97 )
	ROM_LOAD16_BYTE( "pl0j2u4.u8",   0x000001, 0x080000, 0x55b2a3cb )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "pzl10000.u24", 0x000000, 0x400000, 0x35bf6897 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pzl20000.u16", 0x000000, 0x400000, 0xff558e68 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "pzl21000.u18", 0x400000, 0x400000, 0xc8b3be64 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "pzl30000.u4", 0x000000, 0x400000, 0x38604b8d )
ROM_END

/* haven't even tried to run the ones below yet */

ROM_START( jjparads )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "jp1j1.u10",    0x000000, 0x080000, 0xde2fb669 )
    ROM_LOAD16_BYTE( "jp1j1.u8",     0x000001, 0x080000, 0x7276efb1 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "jp100-00.u24", 0x000000, 0x400000, 0xf31b2e95 )
    ROM_LOAD( "jp101-00.u20", 0x400000, 0x400000, 0x70cc8c24 )
    ROM_LOAD( "jp102-00.u17", 0x800000, 0x400000, 0x35401c1e )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jp200-00.u16", 0x000000, 0x200000, 0x493d63db )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "jp300-00.u4", 0x000000, 0x200000, 0x7023fe46 )
ROM_END

ROM_START( jjparad2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "jp2000j1.u6",  0x000000, 0x080000, 0x5d75e765 )
    ROM_LOAD16_BYTE( "jp2001j1.u4",  0x000001, 0x080000, 0x1771910a )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "jp210000.u21", 0x000000, 0x400000, 0x79a7e3d7 )
    ROM_LOAD( "jp210100.u20", 0x400000, 0x400000, 0x42415e0c )
    ROM_LOAD( "jp210200.u8",  0x800000, 0x400000, 0x26731745 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jp220000.u17", 0x000000, 0x400000, 0xd0e71873 )
    ROM_LOAD( "jp220100.u9",  0x400000, 0x400000, 0x4c7d964d )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "jp230000.u1", 0x000000, 0x400000, 0x73e30d7f )
ROM_END

ROM_START( sengekis )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "ss01j.u6",     0x000000, 0x080000, 0x9efdcd5a )
    ROM_LOAD16_BYTE( "ss01j.u4",     0x000001, 0x080000, 0x92c3f45e )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "ss100-00.u21", 0x000000, 0x400000, 0xbc7b3dfa )
    ROM_LOAD( "ss101-00.u20", 0x400000, 0x400000, 0xab2df280 )
    ROM_LOAD( "ss102-00.u8",  0x800000, 0x400000, 0x0845eafe )
    ROM_LOAD( "ss103-00.u32", 0xc00000, 0x400000, 0xee451ac9 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "ss200-00.u17", 0x000000, 0x400000, 0xcd773976 )
    ROM_LOAD( "ss201-00.u9",  0x400000, 0x400000, 0x301fad4c )

	ROM_REGION( 0x600000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "ss210-00.u3",  0x400000, 0x200000, 0xc3697805 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "ss300-00.u1", 0x000000, 0x400000, 0x35b04b18 )
ROM_END

ROM_START( senknow )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "snw000j1.u6",  0x000000, 0x080000, 0x0d6136f6 )
    ROM_LOAD16_BYTE( "snw001j1.u4",  0x000001, 0x080000, 0x4a10ec3d )

	ROM_REGION( 0x0800000, REGION_GFX1, 0 )
    ROM_LOAD( "snw10000.u21", 0x000000, 0x400000, 0x5133c69c )
    ROM_LOAD( "snw10100.u20", 0x400000, 0x400000, 0x9dafe03f )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "snw20000.u17", 0x000000, 0x400000, 0xd5fe5f8c )
    ROM_LOAD( "snw20100.u9",  0x400000, 0x400000, 0xc0037846 )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "snw21000.u3",  0x400000, 0x400000, 0xf5c23e79 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "snw30000.u1",  0x000000, 0x400000, 0xec9eef40 )
ROM_END

ROM_START( teljan )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "tel1j.u10",    0x000000, 0x080000, 0x09b552fe )
    ROM_LOAD16_BYTE( "tel1j.u8",     0x000001, 0x080000, 0x070b4345 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "tj100-00.u24", 0x000000, 0x400000, 0x810144f1 )
    ROM_LOAD( "tj101-00.u20", 0x400000, 0x400000, 0x82f570e1 )
    ROM_LOAD( "tj102-00.u17", 0x800000, 0x400000, 0xace875dc )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "tj200-00.u16", 0x000000, 0x400000, 0xbe0f90b2 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "tj300-00.u4", 0x000000, 0x400000, 0x685495c4 )
ROM_END

ROM_START( ryouran )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "or000j1.u10",  0x000000, 0x080000, 0xd93aa491 )
    ROM_LOAD16_BYTE( "or001j1.u8",   0x000001, 0x080000, 0xf466e5e9 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "or100-00.u24", 0x000000, 0x400000, 0xe9c7695b )
    ROM_LOAD( "or101-00.u20", 0x400000, 0x400000, 0xfe06bf12 )
    ROM_LOAD( "or102-00.u17", 0x800000, 0x400000, 0xf2a5237b )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "or200-00.u16", 0x000000, 0x400000, 0x4c4701a8 )
    ROM_LOAD( "or201-00.u13", 0x400000, 0x400000, 0xa94064aa )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "or300-00.u4", 0x000000, 0x400000, 0xa3f64b79 )
ROM_END


ROM_START( sarukani )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, 0x7e2b836c ) /* BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "sk1j1.u10",    0x000000, 0x080000, 0xfcc131b6 )
    ROM_LOAD16_BYTE( "sk1j1.u8",     0x000001, 0x080000, 0x3b6aa343 )

	ROM_REGION( 0x0400000, REGION_GFX1, 0 )
    ROM_LOAD( "sk100-00.u24", 0x000000, 0x200000, 0x151dd88a )
    ROM_LOAD( "sk-101.u20",   0x200000, 0x100000, 0x779cce23 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "sk200-00.u16", 0x000000, 0x200000, 0x2e297c61 )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "sk300-00.u4", 0x000000, 0x200000, 0xe6535c05 )
ROM_END


/***** GAME DRIVERS *****/

GAMEX( 1996, skns,     0,    skns, skns, 0,        ROT0,  "Kaneko", "Super Kaneko Nova System BIOS", NOT_A_DRIVER )

/* playable */
GAMEX( 1996, galpani4, skns,    skns, skns, galpani4, ROT0,  "Kaneko", "Gals Panic 4 (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, galpanis, skns,    skns, skns, galpanis, ROT0,  "Kaneko", "Gals Panic S - Extra Edition (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, cyvern,   skns,    skns, skns, cyvern,   ROT90, "Kaneko", "Cyvern (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, galpans2, skns,    skns, skns, galpans2, ROT0,  "Kaneko", "Gals Panic S2 (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, panicstr, skns,    skns, skns, panicstr, ROT0,  "Kaneko", "Panic Street (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, senknow , skns,    skns, skns, senknow,  ROT0,  "Kaneko", "Sen-Know (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, puzzloop, skns,    skns, skns, puzzloop, ROT0,  "Mitchell", "Puzz Loop (Europe)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, puzloopj, puzzloop,skns, skns, puzloopj, ROT0,  "Mitchell", "Puzz Loop (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, jjparads, skns,    skns, skns, jjparads, ROT0,  "Electro Design", "Jan Jan Paradise", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, jjparad2, skns,    skns, skns, jjparad2, ROT0,  "Electro Design", "Jan Jan Paradise 2", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, ryouran , skns,    skns, skns, ryouran,  ROT0,  "Electro Design", "Otome Ryouran", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, teljan  , skns,    skns, skns, teljan,   ROT0,  "Electro Design", "Tel Jan", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, sengekis, skns,    skns, skns, sengekis, ROT270,"Kaneko / Warashi", "Sengeki Striker", GAME_IMPERFECT_GRAPHICS ) // should be rot90 + flipped sprites?

/* not playable */
GAMEX( 2000, gutsn,    skns,    skns, skns, gutsn,    ROT0,  "Kaneko", "Guts'n (Japan)", GAME_NOT_WORKING ) // doesn't display anything (sh2?)
GAMEX( 1997, sarukani, skns,    skns, skns, sarukani, ROT0,  "Kaneko", "Saru-Kani-Hamu-Zou", GAME_NOT_WORKING ) // character doesn't move
