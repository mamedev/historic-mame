/* 8080bw.c *********************************
 updated: 1997-04-09 08:46 TT
 updated  20-3-1998 LT Added color changes on base explosion
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'invaders' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 3:
 * bit 0=UFO  (repeats)       emulated
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 * bit 4=Bonus base           9.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "machine/74123.h"

void invaders_flipscreen_w(int data);
void invaders_screen_red_w(int data);

static WRITE_HANDLER( invad2ct_sh_port1_w );
static WRITE_HANDLER( invaders_sh_port3_w );
static WRITE_HANDLER( invaders_sh_port5_w );
static WRITE_HANDLER( invad2ct_sh_port7_w );

static WRITE_HANDLER( bandido_sh_port4_w );
static WRITE_HANDLER( bandido_sh_port5_w );

static WRITE_HANDLER( ballbomb_sh_port3_w );

static WRITE_HANDLER( boothill_sh_port3_w );
static WRITE_HANDLER( boothill_sh_port5_w );
READ_HANDLER( boothill_port_0_r );
READ_HANDLER( boothill_port_1_r );

READ_HANDLER( gunfight_port_0_r );
READ_HANDLER( gunfight_port_1_r );

WRITE_HANDLER( desertgu_controller_select_w );
READ_HANDLER( desertgu_port_1_r );

READ_HANDLER( seawolf_port_0_r );


void init_machine_invaders(void)
{
	install_port_write_handler(0, 0x03, 0x03, invaders_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, invaders_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

void init_machine_invad2ct(void)
{
	init_machine_invaders();

	install_port_write_handler(0, 0x01, 0x01, invad2ct_sh_port1_w);
	install_port_write_handler(0, 0x07, 0x07, invad2ct_sh_port7_w);

	SN76477_envelope_1_w(1, 1);
	SN76477_envelope_2_w(1, 0);
	SN76477_mixer_a_w(1, 0);
	SN76477_mixer_b_w(1, 0);
	SN76477_mixer_c_w(1, 0);
	SN76477_vco_w(1, 1);
}


/*
   Note: For invad2ct, the Player 1 sounds are the same as for the
         original and deluxe versions.  Player 2 sounds are all
         different, and are triggered by writes to port 1 and port 7.

*/

static void invaders_sh_1_w(int board, int data, unsigned char *last)
{
	int base_channel, base_sample;

	base_channel = 4 * board;
	base_sample  = 9 * board;

	SN76477_enable_w(board, !(data & 0x01));				/* Saucer Sound */

	if (data & 0x02 && ~*last & 0x02)
		sample_start (base_channel+0, base_sample+0, 0);	/* Shot Sound */

	if (data & 0x04 && ~*last & 0x04)
		sample_start (base_channel+1, base_sample+1, 0);	/* Base Hit */

	if (~data & 0x04 && *last & 0x04)
		sample_stop (base_channel+1);

	if (data & 0x08 && ~*last & 0x08)
		sample_start (base_channel+0, base_sample+2, 0);	/* Invader Hit */

	if (data & 0x10 && ~*last & 0x10)
		sample_start (base_channel+2, 8, 0);				/* Bonus Missle Base */

	invaders_screen_red_w(data & 0x04);

	*last = data;
}

static void invaders_sh_2_w(int board, int data, unsigned char *last)
{
	int base_channel, base_sample;

	base_channel = 4 * board;
	base_sample  = 9 * board;

	if (data & 0x01 && ~*last & 0x01)
		sample_start (base_channel+1, base_sample+3, 0);	/* Fleet 1 */

	if (data & 0x02 && ~*last & 0x02)
		sample_start (base_channel+1, base_sample+4, 0);	/* Fleet 2 */

	if (data & 0x04 && ~*last & 0x04)
		sample_start (base_channel+1, base_sample+5, 0);	/* Fleet 3 */

	if (data & 0x08 && ~*last & 0x08)
		sample_start (base_channel+1, base_sample+6, 0);	/* Fleet 4 */

	if (data & 0x10 && ~*last & 0x10)
		sample_start (base_channel+3, base_sample+7, 0);	/* Saucer Hit */

	invaders_flipscreen_w(data & 0x20);

	*last = data;
}


static WRITE_HANDLER( invad2ct_sh_port1_w )
{
	static unsigned char last = 0;

	invaders_sh_1_w(1, data, &last);
}

static WRITE_HANDLER( invaders_sh_port3_w )
{
	static unsigned char last = 0;

	invaders_sh_1_w(0, data, &last);
}

static WRITE_HANDLER( invaders_sh_port5_w )
{
	static unsigned char last = 0;

	invaders_sh_2_w(0, data, &last);
}

static WRITE_HANDLER( invad2ct_sh_port7_w )
{
	static unsigned char last = 0;

	invaders_sh_2_w(1, data, &last);
}


/*******************************************************/
/*                                                     */
/* Midway "Gun Fight"                                  */
/*                                                     */
/*******************************************************/

void init_machine_gunfight(void)
{
	install_port_read_handler(0, 0x00, 0x00, gunfight_port_0_r);
	install_port_read_handler(0, 0x01, 0x01, gunfight_port_1_r);
}


/*******************************************************/
/*                                                     */
/* Midway "Boot Hill"                                  */
/*                                                     */
/*******************************************************/

/* HC 4/14/98 NOTE: *I* THINK there are sounds missing...
i dont know for sure... but that is my guess....... */

void init_machine_boothill(void)
{
	install_port_read_handler (0, 0x00, 0x00, boothill_port_0_r);
	install_port_read_handler (0, 0x01, 0x01, boothill_port_1_r);

	install_port_write_handler(0, 0x03, 0x03, boothill_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, boothill_sh_port5_w);
}

static WRITE_HANDLER( boothill_sh_port3_w )
{
	switch (data)
	{
		case 0x0c:
			sample_start (0, 0, 0);
			break;

		case 0x18:
		case 0x28:
			sample_start (1, 2, 0);
			break;

		case 0x48:
		case 0x88:
			sample_start (2, 3, 0);
			break;
	}
}

/* HC 4/14/98 */
static WRITE_HANDLER( boothill_sh_port5_w )
{
	switch (data)
	{
		case 0x3b:
			sample_start (2, 1, 0);
			break;
	}
}


/*******************************************************/
/*                                                     */
/* Taito "Balloon Bomber"                              */
/*                                                     */
/*******************************************************/

/* This only does the color swap for the explosion */
/* We do not have correct samples so sound not done */

void init_machine_ballbomb(void)
{
	install_port_write_handler(0, 0x03, 0x03, ballbomb_sh_port3_w);
}

static WRITE_HANDLER( ballbomb_sh_port3_w )
{
	invaders_screen_red_w(data & 0x04);
}


/*******************************************************/
/*                                                     */
/* Exidy "Bandido"                              	   */
/*                                                     */
/*******************************************************/

static void bandido_74123_0_output_changed_cb(void)
{
	SN76477_vco_w    (0,  TTL74123_output_r(0));
	SN76477_mixer_a_w(0, !TTL74123_output_r(0));

	SN76477_enable_w(0, TTL74123_output_comp_r(0) && TTL74123_output_comp_r(1));
}

static void bandido_74123_1_output_changed_cb(void)
{
	SN76477_set_vco_voltage(0, !TTL74123_output_comp_r(1) ? 5.0 : 0.0);

	SN76477_enable_w(0, TTL74123_output_comp_r(0) && TTL74123_output_comp_r(1));
}

static struct TTL74123_interface bandido_74123_0_intf =
{
	RES_K(33),
	CAP_U(33),
	bandido_74123_0_output_changed_cb
};

static struct TTL74123_interface bandido_74123_1_intf =
{
	RES_K(33),
	CAP_U(33),
	bandido_74123_1_output_changed_cb
};


void init_machine_bandido(void)
{
	install_port_write_handler(0, 0x04, 0x04, bandido_sh_port4_w);
	install_port_write_handler(0, 0x05, 0x05, bandido_sh_port5_w);

	TTL74123_config(0, &bandido_74123_0_intf);
	TTL74123_config(1, &bandido_74123_1_intf);

	/* set up the fixed connections */
	TTL74123_reset_comp_w  (0, 1);
	TTL74123_trigger_comp_w(0, 0);

	TTL74123_trigger_comp_w(1, 0);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_noise_clock_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
}


static int bandido_t0,bandido_t1,bandido_p1,bandido_p2;


static WRITE_HANDLER( bandido_sh_port4_w )
{
	bandido_t0 = data & 1;

	bandido_p1 = (bandido_p1 & 0x4f) |
				 ((data & 0x02) << 3) |		/* P1.4 */
				 ((data & 0x08) << 2) |		/* P1.5 */
				 ((data & 0x20) << 2);		/* P1.7 */

	cpu_set_irq_line(1, I8035_EXT_INT, ((bandido_p1 & 0x70) == 0x70) ? ASSERT_LINE : CLEAR_LINE);


	TTL74123_trigger_w   (0, data & 0x04);

	TTL74123_trigger_w   (1, data & 0x10);
	TTL74123_reset_comp_w(1, data & 0x04);
}

static WRITE_HANDLER( bandido_sh_port5_w )
{
	bandido_t1 = (data >> 5) & 1;

	bandido_p1 = (bandido_p1 & 0xb0) |
				 ((data & 0x01) << 3) |		/* P1.3 */
				 ((data & 0x02) << 1) |		/* P1.2 */
				 ((data & 0x04) >> 1) |		/* P1.1 */
				 ((data & 0x08) >> 3) |		/* P1.0 */
				 ((data & 0x10) << 2);		/* P1.6 */

	cpu_set_irq_line(1, I8035_EXT_INT, ((bandido_p1 & 0x70) == 0x70) ? ASSERT_LINE : CLEAR_LINE);
}

READ_HANDLER( bandido_sh_t0_r )
{
	return bandido_t0;
}

READ_HANDLER( bandido_sh_t1_r )
{
	return bandido_t1;
}

READ_HANDLER( bandido_sh_p1_r )
{
	return bandido_p1;
}

READ_HANDLER( bandido_sh_p2_r )
{
	return bandido_p2;
}

WRITE_HANDLER( bandido_sh_p2_w )
{
	bandido_p2 = data;

	DAC_data_w(0, bandido_p2 & 0x80 ? 0xff : 0x00);
}


/*******************************************************/
/*                                                     */
/* Midway "Desert Gun"                                 */
/*                                                     */
/*******************************************************/

void init_machine_desertgu(void)
{
	install_port_read_handler (0, 0x01, 0x01, desertgu_port_1_r);

	install_port_write_handler(0, 0x07, 0x07, desertgu_controller_select_w);
}


/*******************************************************/
/*                                                     */
/* Midway "Sea Wolf"                                   */
/*                                                     */
/*******************************************************/

void init_machine_seawolf(void)
{
	install_port_read_handler (0, 0x01, 0x01, seawolf_port_0_r);
}
