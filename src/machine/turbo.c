/*************************************************************************

	 Turbo - Sega - 1981

	 Machine Hardware

*************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"

/* globals */
UINT8 turbo_opa, turbo_opb, turbo_opc;
UINT8 turbo_ipa, turbo_ipb, turbo_ipc;
UINT8 turbo_fbpla, turbo_fbcol;
UINT8 turbo_segment_data[32];
UINT8 turbo_speed;

/* local data */
static UINT8 segment_address, segment_increment;
static UINT8 osel, bsel, accel;

/* prototypes */
extern UINT8 turbo_collision;


/*******************************************

	Sample handling

*******************************************/

static void update_samples(void)
{
	/* accelerator sounds */
	/* BSEL == 3 --> off */
	/* BSEL == 2 --> standard */
	/* BSEL == 1 --> tunnel */
	/* BSEL == 0 --> ??? */
	if (bsel == 3 && sample_playing(6))
		sample_stop(6);
	else if (bsel != 3 && !sample_playing(6))
		sample_start(6, 7, 1);
	if (sample_playing(6))
//		sample_set_freq(6, 44100 * (accel & 0x3f) / 7 + 44100);
		sample_set_freq(6, 44100 * (accel & 0x3f) / 5.25 + 44100);
}


/*******************************************

	8255 PPI handling

*******************************************/
/*
	chip index:
	0 = IC75 - CPU Board, Sheet 6, D7
	1 = IC32 - CPU Board, Sheet 6, D6
	2 = IC123 - CPU Board, Sheet 6, D4
	3 = IC6 - CPU Board, Sheet 5, D7
*/

static WRITE_HANDLER(chip0_portA_w)
{
	turbo_opa = data;	/* signals 0PA0 to 0PA7 */
}

static WRITE_HANDLER(chip0_portB_w)
{
	turbo_opb = data;	/* signals 0PB0 to 0PB7 */
}

static WRITE_HANDLER(chip0_portC_w)
{
	turbo_opc = data;	/* signals 0PC0 to 0PC7 */
}


static WRITE_HANDLER(chip1_portA_w)
{
	turbo_ipa = data;	/* signals 1PA0 to 1PA7 */
}

static WRITE_HANDLER(chip1_portB_w)
{
	turbo_ipb = data;	/* signals 1PB0 to 1PB7 */
}

static WRITE_HANDLER(chip1_portC_w)
{
	turbo_ipc = data;	/* signals 1PC0 to 1PC7 */
}


static WRITE_HANDLER(chip2_portA_w)
{
	/*
		2PA0 = /CRASH
		2PA1 = /TRIG1
		2PA2 = /TRIG2
		2PA3 = /TRIG3
		2PA4 = /TRIG4
		2PA5 = OSEL0
		2PA6 = /SLIP
		2PA7 = /CRASHL
	*/
	/* missing short crash sample, but I've never seen it triggered */
	if (!(data & 0x02)) sample_start(0, 0, 0);
	if (!(data & 0x04)) sample_start(0, 1, 0);
	if (!(data & 0x08)) sample_start(0, 2, 0);
	if (!(data & 0x10)) sample_start(0, 3, 0);
	if (!(data & 0x40)) sample_start(1, 4, 0);
	if (!(data & 0x80)) sample_start(2, 5, 0);
	osel = (osel & 6) | ((data >> 5) & 1);
	update_samples();
}

static WRITE_HANDLER(chip2_portB_w)
{
	/*
		2PB0 = ACC0
		2PB1 = ACC1
		2PB2 = ACC2
		2PB3 = ACC3
		2PB4 = ACC4
		2PB5 = ACC5
		2PB6 = /AMBU
		2PB7 = /SPIN
	*/
	accel = data & 0x3f;
	update_samples();
	if (!(data & 0x40))
	{
		if (!sample_playing(7))
			sample_start(7, 8, 0);
		else
			logerror("ambu didnt start\n");
	}
	else
		sample_stop(7);
	if (!(data & 0x80)) sample_start(3, 6, 0);
}

static WRITE_HANDLER(chip2_portC_w)
{
	/*
		2PC0 = OSEL1
		2PC1 = OSEL2
		2PC2 = BSEL0
		2PC3 = BSEL1
		2PC4 = SPEED0
		2PC5 = SPEED1
		2PC6 = SPEED2
		2PC7 = SPEED3
	*/
	turbo_speed = (data >> 4) & 0x0f;
	bsel = (data >> 2) & 3;
	osel = (osel & 1) | ((data & 3) << 1);
	update_samples();
}


static WRITE_HANDLER(chip3_portC_w)
{
	/* bit 0-3 = signals PLA0 to PLA3 */
	/* bit 4-6 = signals COL0 to COL2 */
	/* bit 7 = unused */
	turbo_fbpla = data & 0x0f;
	turbo_fbcol = (data & 0x70) >> 4;
}


static ppi8255_interface intf =
{
	4, /* 4 chips */
	{0, 0, 0, input_port_4_r}, /* Port A read */
	{0, 0, 0, input_port_2_r}, /* Port B read */
	{0, 0, 0, 0}, /* Port C read */
	{chip0_portA_w, chip1_portA_w, chip2_portA_w, 0}, /* Port A write */
	{chip0_portB_w, chip1_portB_w, chip2_portB_w, 0}, /* Port B write */
	{chip0_portC_w, chip1_portC_w, chip2_portC_w, chip3_portC_w} /* Port C write */
};



/*******************************************

	Machine Init

*******************************************/

void turbo_init_machine(void)
{
	ppi8255_init(&intf);
	segment_address = segment_increment = 0;
}


/*******************************************

	8279 handling
	IC84 - CPU Board, Sheet 5, C7

*******************************************/

READ_HANDLER( turbo_8279_r )
{
	if ((offset & 1) == 0)
		return readinputport(1);  /* DSW 1 */
	else
	{
		logerror("read 0xfc%02x\n", offset);
		return 0x10;
	}
}

WRITE_HANDLER( turbo_8279_w )
{
	switch (offset & 1)
	{
		case 0x00:
			turbo_segment_data[segment_address * 2] = data & 15;
			turbo_segment_data[segment_address * 2 + 1] = (data >> 4) & 15;
			segment_address = (segment_address + segment_increment) & 15;
			break;

		case 0x01:
			switch (data & 0xe0)
			{
				case 0x80:
					segment_address = data & 15;
					segment_increment = 0;
					break;
				case 0x90:
					segment_address = data & 15;
					segment_increment = 1;
					break;
				case 0xc0:
					memset(turbo_segment_data, 0, 32);
					break;
			}
			break;
	}
}


/*******************************************

	Misc handling

*******************************************/

READ_HANDLER( turbo_collision_r )
{
	return readinputport(3) | (turbo_collision & 15);
}

WRITE_HANDLER( turbo_collision_clear_w )
{
	turbo_collision = 0;
}

WRITE_HANDLER( turbo_coin_and_lamp_w )
{
	data &= 1;
	switch (offset & 7)
	{
		case 0:		/* Coin Meter 1 */
		case 1:		/* Coin Meter 2 */
		case 2:		/* n/c */
			break;

		case 3:		/* Start Lamp */
			set_led_status(0, data & 1);
			break;

		case 4:		/* n/c */
		default:
			break;
	}
}
