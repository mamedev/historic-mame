/***************************************************************************
	pong.c
	Video handler

	J. Buchmueller, November '99
****************************************************************************/

#include "vidhrdw/generic.h"
#include "cpu/gensync/gensync.h"
#include "vidhrdw/pong.h"

extern int pong_hit_sound;
extern int pong_vblank_sound;
extern int pong_score_sound;

static int V;
static int VRESET;

static int ATRACT;
static int STOP_G;

static int vpad1_timer; 	/* NE555/B9 */
static int vpad1_count; 	/* 7493/B8 */
#define VPAD1 (vpad1_count < 15)
static int score1;			/* 7490/C7 + 74107/C8.1 */

static int vpad2_timer; 	/* NE555/A9 */
static int vpad2_count; 	/* 7493/A8 */
#define VPAD2 (vpad2_count < 15)
static int score2;			/* 7490/D7 + 74107/C8.2 */

static int speed;			/* 7493/F1 */
static int speed_q1;		/* 74107/H2.2*/
static int speed_q2;		/* 74107/H2.1 */


static int hpos_a;			/* 9316/G7 */
static int hpos_b;			/* 9316/H7 */
static int hpos_c;			/* 74107/G6.2 */
static int HVID;			/* 7420/H6.2 */

static int vpos_a;			/* 9316/B3 */
static int vpos_b;			/* 9316/A3 */
static int VVID;			/* 7402/D2.4 */

static int vvel_b;			/* 7474/A5.2 */
static int vvel_c;			/* 7474/A5.1 */
static int vvel_d;			/* 7474/B5.1 */
static int hit_vbl_q;       /* 74107/A2.1 */
static int hit_vbl_0;		/* direction during last frame (previous state of 74107/A2.1) */

static int v_velocity;		/* 7483/B4 */

static int score_q; 		/* 74107/H3.2 */
#define L	(score_q == 1)
#define R	(score_q == 0)

static int hit_a;			/* 7400/H4.3 */
static int hit_b;			/* 7400/H4.2 */

static int hit_vblank;		/* 74107/F3.1 */

static int hit_sound;       /* 7474/C2.1 */

static void *score_sound_timer;  /* NE555/G4 */
static void *serve_timer;  /* NE555/F4 */

#ifdef MAME_DEBUG
static int hpos;           /* debug */
static int vpos;		   /* debug */
static int hpos_h;		   /* debug */
static int vpos_h;		   /* debug */
#endif

INLINE void pong_hit_detector(void);
INLINE void pong_vertical_velocity(void);
INLINE void pong_7seg(int H0, int n);

int pong_vh_start(void)
{
	ATRACT = 0;
	STOP_G = 1;

	vpad1_timer = 0;
	vpad1_count = 0;
	score1 = 0;

	vpad2_timer = 0;
	vpad2_count = 0;
	score2 = 0;

	speed = 0;
	speed_q1 = 0;
	speed_q2 = 0;

	score_q = 0;

    hpos_a = 0;
	hpos_b = 0;
	hpos_c = 0;

	vpos_a = 0;
	vpos_b = 0;

	vvel_b = 0;
	vvel_c = 0;
	vvel_d = 0;
	hit_vbl_q = 0;

	v_velocity = 0;

	hit_a = 0;
	hit_b = 1;

	hit_sound = 0;

	score_sound_timer = NULL;
    serve_timer = NULL;

	pong_hit_detector();

    if( generic_bitmapped_vh_start() )
        return 1;

    return 0;
}

void pong_vh_stop(void)
{
	generic_vh_stop();
}

static void score_timer_cb(int param)
{
	score_sound_timer = NULL;
	/* disable score sound */
    pong_score_sound = 0;
}

static void serve_timer_cb(int param)
{
	serve_timer = NULL;
    /* falling edge of serve timer (output of NE555/F4) */
	hpos_a = 0;
	hpos_b = 0;
	hpos_c = 0;
}

void pong_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
#ifdef MAME_DEBUG
	char buf[32+1];

	sprintf(buf, "vvel: %2d", v_velocity);
    ui_text(buf, 0, 1);
	sprintf(buf, "hpos: %3d", hpos);
    ui_text(buf, 128, 1);
    sprintf(buf, "vpos: %3d", vpos);
    ui_text(buf, 256, 1);
    sprintf(buf, "Aa/Ba: %d/%d", hit_a, hit_b);
	ui_text(buf, 0, 9);
	sprintf(buf, "hpos_h: %3d", hpos_h);
    ui_text(buf, 128, 9);
    sprintf(buf, "vpos_h: %3d", vpos_h);
    ui_text(buf, 256, 9);
	sprintf(buf, "speed: %2d %d/%d->%d", speed, speed_q1, speed_q2, (speed_q1 && speed_q2) ? 0 : 1);
	ui_text(buf, 0, 17);
	sprintf(buf, "L/R: %d/%d", score_q, score_q ^ 1);
	ui_text(buf, 0, 25);
#endif

	if( readinputport(0) & 1 )
	{
		if( STOP_G )
		{
			STOP_G = 0;
			ATRACT = 1;
			score1 = 0;
			score2 = 0;
			if( serve_timer == NULL )
			{
				/* monoflop (NE555/G4) with a 330 kOhms resistor and 4.7 æF capacitor */
				serve_timer = timer_set(TIME_IN_USEC(1.2*330000.0*4.7), 0, serve_timer_cb);
			}
        }
	}

    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	fillbitmap(tmpbitmap, Machine->pens[0], &Machine->drv->visible_area);

	/* reset the VPAD timers */
	vpad1_timer = 0;
	vpad2_timer = 0;

	/* save state of the vblank flip-flop 74107/A2 */
    hit_vbl_0 = hit_vbl_q;

    if( errorlog )
		fprintf(errorlog, "pong_vh_screenrefresh at H:%d, V:%d\n", cpu_get_reg(GS_H), cpu_get_reg(GS_V));

	cpu_set_reg(GS_V, 0);
    V = 0;
	VRESET = 1;
}

/******* HORIZONTAL SYNC *****************************************************
 * Pong's horizontal counter is made up of two 7493 chips (4 bit
 * binary counters F8 and F9) and a JK flip-flop (74107 F6) for
 * the MSB, whic is bit 8 of the pixel column number.
 * The HRESET signal is generated when counters reached 256+128+64+4+2,
 * that is the pixel clock #454 of a scanline. The HRESET signal
 * is decoded using the eight input NAND gate 7430/F7, three inputs are
 * tied to Vcc and the remaining five are connected to the 256H, 128H,
 * 64H, 4H and 2H outputs. This signal goes to a D-type flip-flop.
 * The raising edge of the clock pulse latches the output of F7 in
 * 7474/E7.2, it's output Q\ is called HRESET.
 * The HRESET goes away at the next clock cycle's raising edge.
 * At the time HRESET is active, there's another flip-flop made from two
 * NAND gates (7400/H5.2 and 7400/H5.3) set to generate the HBLANK signal.
 * This signal remains active until the counter reaches 64+16 (decoded by
 * 7410/G5.2), so the first 80 pixel clocks are invisible/blanked.
 * During the horizontal blanking time, there's the HSYNC signal
 * drived from counter bit 6 (32H), so pixel clocks 0..31 are
 * HBLANK alone, 32..63 HBLANK+HSYNC and 64..79 HBLANK alone again.
 *****************************************************************************/
#define H4		(H&4)
#define H8		(H&8)
#define H16 	(H&16)
#define H32 	(H&32)
#define H64 	(H&64)
#define H128	(H&128)
#define H256	(H&256)

#define HBLANK	(H < PONG_HBLANK)

/******* VERTICAL SYNC *******************************************************
 * The vertical counter is also made up of two 7493 chips (4-bit
 * binary counters E8 and E9) and a JK flip-flop (74107/D9.2) for
 * the MSB, which is bit 8 of the scanline number.
 * The VRESET signal is generated when counters reached 256+4+1,
 * that is when scanline #261 is being displayed.
 * (This shows that PONG was a horizontal game ;)
 * Similiar to the HORIZONTAL COUNTER there are really two more
 * VERTICAL SYNC counter values, because VRESET is delayed by the
 * pixel clock: the VRESET signal is decoded using a three input
 * NAND gate (7410/D8.3) and it's inputs are connected to the 256V,
 * 4V and 1V outputs. The clock pulse (HRESET) in scanline 261
 * latches the output of D8.3 in the D-type flip-flop (7474/E7.1).
 * It's negative output Q\ is called VRESET, very similiar to the
 * horizontal stuff. The VRESET goes away on the next raising edge
 * of the cycle (HRESET going high).
 * At the time VRESET goes active (high), there's another flip-flop
 * made from two NOR gates (7400/F5.3 and 7400/F5.4) set to generate
 * the VBLANK\ signal. This signal remains active until the counter
 * reaches 16, so the first 16 scanlines are invisible/blanked.
 * During the vertical blanking time, there's the VSYNC signal drived
 * from the VERTICAL SYNC counter bits 2+3 (VBLANK and 4V and 8V\),
 * so scanlines 0..3 are VBLANK alone, scanlines 4..7 VBLANK+VSYNC
 * and scanlines 8..15 VBLANK alone again.
 *
 * If you multiply 261 by the 454 pixels per scanline, the pixel
 * clock must have been 7,109,640 Hz for a 60 Hz frame rate.
 * This would mean a crystal with 14,219,280 Hz, but probably it
 * was a standard US TV crystal with 14,318,100 Hz
 *****************************************************************************/
#define V4		(V&4)
#define V8		(V&8)
#define V16 	(V&16)
#define V32 	(V&32)
#define V64 	(V&64)
#define V128	(V&128)

#define VBLANK	(V < PONG_VBLANK)


/******* VERTICAL SYNC *******************************************************
 * The VERTICAL SYNC counters are incremented once per scanline
 * We emulate this by calling pong_vh_scanline() 262 times per frame
 *****************************************************************************/
int pong_vh_scanline(void)
{
	int HRESET, H;
	int pen1 = Machine->pens[1];

	if( V >= PONG_MAX_V )
		return ignore_interrupt();

/******* THE NET *************************************************************
 * I'll try to start with the simpler things:
 * The net is a vertical line of one pixel width,
 * which occurs every four scanlines - it is 'dashed'.
 * This is decoded using a JK flip-flop clocked
 * with the pixel clock, J tied to 256H and K to -256H.
 * So everytime when there's a change in 256H/-256H the
 * JK flip-flop will be set for one pixel clock.
 * The -Q output is sent into a three input NOR gate together
 * with VBLANK and 4V, so the NET output of gate G2 is true
 * for (H == 256) and ((V & 4) == 0) for every non-blanked
 * scanline (scanlines 16...260).
 *****************************************************************************/
	if( !V4 && !VBLANK )
		plot_pixel(tmpbitmap, 256, V, pen1);

/******* THE SCORE ***********************************************************
 * The score for both players is counted using a decade
 * counter plus a JK flip-flop for the 10. There's C7/C8 1st
 * for player 1 and D7/C8 2nd for player two.
 * The two scores are de-multiplexed using two 74153 chips C6/D6.
 * This are four one-of-four de-multiplexers and they select
 * the signals by decoding 32H and 64H signals.
 * So there are > 14 different stages during each scanline.
 * They alternate according to the following table:
 *  state   A       B       C       D
 *  -------------------------------------------
 *  0       +5v     P1.E    P1.E    P1.E
 *  1       P1.A    P1.B    P1.C    P1.D
 *  2       +5v     P2.E    P2.E    P2.E
 *  3       P2.A    P2.B    p2.C    P2.D
 *  4       - same ase 0 -
 *  ...
 * So we have four score 'digits' here, spread to 4 * 32 pixels.
 * They are contained in the signals A,B,C and D.
 * A BCD-to-7segment decoder 7448 is used to convert those
 * four bits into the segment of a 7-segment LED, which has
 * the following layout:
 *         a
 *       -----
 *    f |  g  | b
 *       -----
 *    e |     | c
 *       -----
 *         d
 * One tricky detail is hidden in the states 0, 2, 4 etc.:
 * The 7448 decodes 15 into 'no segments active'. Since the
 * A signal is tied to +5V and all other inputs are tied to -Q
 * of the overflow JK flip-flop for the even states, this has
 * the effect of suppressing the leading zero until the overflow
 * from the JK flip-flop is set (and thus -Q is cleared to lo).
 *
 * Well, and Pong uses no LED displays, but decodes the
 * horizontal and vertical positions into segments using
 * a whole lotta gates. I don't want to describe it here ;)
 * Look at the gates E2, E3, F2 for the horizontal decoding.
 *****************************************************************************/

    /* Decode the players scores into 7 segment displays
     * Gate F2.1 decodes 32V && !64V && !128V && 'hpos valid',
     * so we first check if the VERTICAL SYNC counter is in range:
     */
    if( V32 && !V64 && !V128 )
    {
        /* Instead of going through the whole line and
         * checking the expression for the E2/E3 gates,
         * we call a decoding function with appropriate X offsets.
         */
		pong_7seg(128, score1 > 9 ? 1 : 15);
		pong_7seg(128 + 32, score1 % 10);
		pong_7seg(256 + 32, score2 > 9 ? 1 : 15);
		pong_7seg(256 + 32 + 32, score2 % 10);
    }


/******* THE ATRACT MODE *****************************************************
 * If the ATRACT signal is lo (while no game is running), the pads are not
 * drawn. The ball HORIZONTAL POSITION counters are kept reset, so there's
 * no ball drawn either. We bail out here if ATRACT is not set
 *****************************************************************************/

	if( ATRACT )
	{

/******* THE VPADS ***********************************************************
 * The players pads are two vertical blocks which are four
 * pixels wide and fifteen pixels high.
 * The starting scanline of each pad is derived from a timer,
 * the well known NE555 chip. The paddle inputs are used to
 * fire the players timer after a variable amount of time.
 * These timers are triggered by the -256V signal, which is
 * active hi for scanlines in range from 0 to 255.
 * When one of the timer fires, it allows the -HSYNC signal
 * to pass a NAND gate (7400 B7) and the output of this gate
 * increments a four bit binary (7493/A8 7493/B8?) counter.
 * This clock is inhibted once the counter reaches 8+4+2+1 = 15,
 * which is decoded using a four input NAND gate (7420 A7).
 * So the counter does count from 0..15 and stop there.
 * The counters are held reset while the timer has not yet
 * fired for scanlines above the pad. The -VPADx signals are
 * also inhibited by the NAND gates of B7 (7400) after the
 * terminal count of 15 has been reached.
 *
 * The horizontal position of the VPADS is decoded using a
 * D-type latch with negative clock input, which is clocked
 * with the 4H signal (pixel clock / 4).
 * The 128H signal is the data which is latched, so the
 * output of the latch is high whenever the horizontal
 * counter has bit 7 set while bit 2 goes from hi to lo.
 * The 256H signal is used to decide which of the two
 * VPADx signals to use: VPAD1 if 256H enables one NOR gate
 * G2 (7427) while VPAD2 is sent through anohter gate
 * together with then negative -256H signal.
 * So the left VPAD is at pixels 128..131 and the right
 * is at pixels 256..259 whenever the respective VPADx
 * counters are counting.
 *****************************************************************************/

		/* check for the left VPAD */
		if( VPAD1 )
		{
			plot_pixel(tmpbitmap, 128+0, V, pen1);
			plot_pixel(tmpbitmap, 128+1, V, pen1);
			plot_pixel(tmpbitmap, 128+2, V, pen1);
			plot_pixel(tmpbitmap, 128+3, V, pen1);
			plot_pixel(tmpbitmap, 128+4, V, pen1);
			plot_pixel(tmpbitmap, 128+5, V, pen1);
			plot_pixel(tmpbitmap, 128+6, V, pen1);
			plot_pixel(tmpbitmap, 128+7, V, pen1);
			vpad1_count += 1;
		}
		else
		/* it isn't, was the timer fired already? */
		if( vpad1_timer == 0 )
		{
			/* no, check if it should fire now */
			vpad1_timer = ( V >= readinputport(1) );
			/* if it did, start counting from the next scanline */
			if( vpad1_timer )
				vpad1_count = 0;
		}

		/* check for the right VPAD */
		if( VPAD2 )
		{
			plot_pixel(tmpbitmap, 256+128+0, V, pen1);
			plot_pixel(tmpbitmap, 256+128+1, V, pen1);
			plot_pixel(tmpbitmap, 256+128+2, V, pen1);
			plot_pixel(tmpbitmap, 256+128+3, V, pen1);
			plot_pixel(tmpbitmap, 256+128+4, V, pen1);
			plot_pixel(tmpbitmap, 256+128+5, V, pen1);
			plot_pixel(tmpbitmap, 256+128+6, V, pen1);
			plot_pixel(tmpbitmap, 256+128+7, V, pen1);
            vpad2_count += 1;
		}
		else
		/* it isn't, was the timer fired already? */
		if( vpad2_timer == 0 )
		{
			/* no, check if it should fire now */
			vpad2_timer = ( V >= readinputport(2) );
			/* if it did, start counting from the next scanline */
			if( vpad2_timer )
				vpad2_count = 0;
		}

		if( VRESET )
		{
			int a, b, c;
			/*
			 * NOR gate 7402/G1.4 is tied to the speed counter outputs
			 * (7493/F1) bits 1 and 3, so if one of them is high, the NOR
			 * output is low; if both are low, the output is high.
			 * But the NOR is used as an OR by inverting it's output
			 * with 7400/H1.1 again, so we toggle the output.
			 */
			a = ( (speed & 2) || (speed & 8) ) ? 1 : 0;
			/*
			 * NAND gate 7400/E1.3 is also tied to bits 1 and 3.
			 * If the counter reads 10, the output is low, otherwise high.
			 */
			b = ( (speed & 2) && (speed & 8) ) ? 0 : 1;
			/*
			 * The signals (a) and (b) go into NAND gate 7400/H1.4,
			 * so the output is low if both inputs are high.
			 */
			c = ( a && b ) ? 0 : 1;
			/*
			 * Now since VRESET is high when we come here,
			 * the two JK flip-flops of H2 are cleared depending
			 * on signal (a) -> clears H2.2 (speed_q1)
			 * signal (b) -> clears H2.1 (speed_q2)
			 */
			if( c == 1 ) speed_q1 = 0;
			if( a == 1 ) speed_q2 = 0;
		}
		else
		{
			/*
			 * clock 74107/H2.1 and 74107/H2.2
			 * J of H2.2 is connected to Q of H2.1, K is tied to GND,
			 * so Q2 is set if Q1 was set, else it is unchanged.
			 * input J of H2.1 tied to Vcc, K comes from the NAND gate
			 * (7400/H4.1), so if K was low the Q output of H2.2
			 * is set, otherwise it is toggled.
			 */
			if( speed_q1 )
			{
				if( !speed_q2 )
					speed_q1 ^= 1;
				speed_q2 = 1;
			}
			else
			{
				speed_q1 ^= 1;
			}
		}

/******* VERTICAL POSITION ***************************************************
 * This section is made up of two presettable 4-bit binary counters
 * (9316/B3 and 9316/A3). The first is loaded with a value from
 * the VERTICAL VELOCITY section (outputs of 4-bit full adder 7483/B4)
 * and the second is always with zero.
 * The clock input for both counters is the -HSYNC signal, so the
 * count takes place during the HBLANK time when -HSYNC goes hi
 * again (to be exact: at pixel clock #64 of a scanline).
 * However, both counters have 'enable counter' inputs:
 * Counter a (9316/B3) only counts if the -VBLANK signal is hi,
 * so it does not count during VBLANK (scanlines 0 to 15).
 * Counter b (9316/A3) only counts if the RC output of counter A
 * is high - this is the case whenever counter A counted to 15.
 * *IMPORTANT*: RC is only goes high when the T enable input is high too!
 * The VVID signal is true whenever counter A counted to a value
 * >= 12 (bits 2+3 are hi) and the RC output counter B is set
 * (counter B = 15). When both counters reach 15 they are reloaded.
 *****************************************************************************/
		/*
		 * Output of 7400 B2.2
		 * Are the RC outputs of both VERTICAL POSITION counters active?
		 */
		if( vpos_a == 15 && !VBLANK && vpos_b == 15 )
		{
			pong_vertical_velocity();
			/*
			 * reset VERTICAL POSITION counter A to the value output
			 * from the VERTICAL VELOCITY 4-bit full adder
			 */
			vpos_a = v_velocity;
			/*
			 * reset VERTICAL POSITION counter B to zero
			 * all inputs of 9316/A3 are tied to ground)
			 */
			vpos_b = 0;
		}
		else
		{
			/* If the RC output of VERTICAL POSITION counter A (9316/B3) is hi */
			if( vpos_a == 15 && !VBLANK )
			{
				/* Increment the VERTICAL POSITION counter B (9316/A3) */
				vpos_b = ++vpos_b % 16;
				/*
				 * raising edge of RC also resets the HIT SOUND D FF 7474/C2
				 */
				if( vpos_b == 15 )
				{
					/* eventually enable hit sound */
					pong_hit_sound = hit_sound;
					hit_sound = 0;
				}
			}
        }
		/* If VBLANK is not active */
		if( !VBLANK )
		{
			/* Increment the VERTICAL POSITION counter A (9316/B3) */
			vpos_a = ++vpos_a % 16;
		}

		if( V == PONG_VBLANK )
		{
            /* eventually enable hit VBLANK sound */
			if( hit_vblank > 0 )
				hit_vblank--;
			pong_vblank_sound = hit_vblank;
		}

        /*
		 * output of 7410 E2.2
		 * check if bits 2 and 3 of counter A and the RC output of counter B are set
		 */
		VVID = ( vpos_a >= 12 && vpos_b == 15 ) ? 1 : 0;

        if( serve_timer == NULL )
        {
			if( VVID && V == VBLANK )	/* VBLANK just went low ? */
			{
				/* check for VVID on the falling edge of VBLANK
				 * this is done with 74107/A2.1 from the VERTICAL VELOCITY
				 * section which is clocked with VBLANK and toggles it's
				 * Q/-Q outputs if VVID is active (high).
				 * A2.1 is cleared by any of the HIT1 or HIT2 signals.
				 */
				hit_vbl_q = hit_vbl_0 ^ 1;
				hit_vblank = 2;
			}

/******* HORIZONTAL SYNC *****************************************************
 * The HORIZONTAL SYNC counters are swept through now to emulate the
 * ball position (HORIZONTAL POSITION) and HIT DETECTION stuff.
 *****************************************************************************/

			for( HRESET = 1, H = 0; H < PONG_MAX_H; HRESET ? HRESET = 0 : H++ )
			{
/******* HORIZONTAL POSITION *************************************************
 * This section is made up of two presettable 4-bit binary counters
 * (9316/G7 and 9316/H7) and a JK flip-flop (74107/G5.2).
 * The first counter is loaded with eight plus a value from the
 * HIT DETECTION section (outputs Aa, Ba called hit_a, hit_b here)
 * and the second is always loaded with a constant value of eight.
 * The clock input for both counters is the CLOCK signal.
 * However, both counters have 'enable counter' inputs:
 * Counter A (9316/G7) only counts if the -HBLANK signal is hi,
 * so it does not count during HBLANK (pixels 0 to 79 of a scanline).
 * Counter B (9316/H7) only counts if the RC output of counter A
 * is hi - this is the case whenever counter A counted to 15.
 * *IMPORTANT*: RC is only goes high when the T enable input is high too!
 * The JK flip-flop toggles it's Q output whenever the RC output
 * of counter B goes from hi to lo.
 * The HVID signal is true whenever counter A counted to a value
 * >= 12 (bits 2+3 are hi), the RC output counter B is set.
 * (counter B = 15) and the JK flip-flop output Q is hi.
 * When both counters reach 15 and the JK flip-flop Q output is hi
 * the counters are reloaded (and the JK flip-flop is reset).
 *****************************************************************************/
				/* if HBLANK is not active */
				if( !HBLANK )
				{
					/* output of 7410 G5.3
					 * check if the RC outputs of both HORIZONTAL POSITION counters
					 * and the Q output of the JK flip-flop are set
					 */
					if( hpos_a == 15 && hpos_b == 15 && hpos_c )
					{
						pong_hit_detector();
						/*
						 * load counter A with D=8, C=0, B=hit_b and A=hit_a
						 * (hit_a and hit_b signals from HIT DETECTION section)
						 */
						hpos_a = 8 + hit_b * 2 + hit_a;
						/*
						 * Load counter B with D=8 (Vcc) and C,B,A=0 (GND)
						 * constant load 8
						 */
						hpos_b = 8;
						/*
						 * on a falling edge of RC output of H7: toggle the JK flip-flop
						 */
						hpos_c ^= 1;
					}
					else
					{
						/* if the RC output of HORIZONTAL POSITION counter A (9316/G7) is high */
						if( hpos_a == 15 )
						{
							/* increment the HORIZONTAL POSITION counter B (9316/H7) */
							hpos_b = ++hpos_b % 16;

							/* on a falling edge of RC output of H7: toggle the JK flip-flop */
							if( hpos_b == 0 )
								hpos_c ^= 1;
                        }
						/* increment the HORIZONTAL POSITION counter A (9316/G7) */
						hpos_a = ++hpos_a % 16;
                    }
                }
				/*
				 * Output of 7410/E2.2
				 * Are bits 2+3 of counter A, RC of counter B and the JK flip-flop active?
				 */
				HVID = ( hpos_a >= 12 && hpos_b == 15 && hpos_c ) ? 1 : 0;

                if( HVID && VVID )
				{
					if( !VBLANK )
						plot_pixel(tmpbitmap, H, V, pen1);
#ifdef	MAME_DEBUG
					if( vpos_a == 12 ) vpos = V;	/* first scanline of VVID */
					if( hpos_a == 12 ) hpos = H;	/* first pixel of HVID */
#endif
					if( VPAD1 && H >= 128 && H < 128+8 )
					{
#ifdef MAME_DEBUG
                        hpos_h = hpos;
                        vpos_h = vpos;
#endif
                        /* 7474/H3.2 is cleared by the HIT1 signal */
						score_q = 0;
						if( hit_sound == 0 )
						{
							pong_hit_sound = hit_sound = 1;
							if( speed < 10 )
								speed += 1;
						}
                        vvel_b = (vpad1_count & 2) ? 0 : 1;
                        vvel_c = (vpad1_count & 4) ? 0 : 1;
                        vvel_d = (vpad1_count & 8) ? 0 : 1;
						/* 74107/A2.1 is cleared by any HIT signal */
						hit_vbl_q = 0;
                    }
					else
					if( VPAD2 && H >= 256+128 && H < 256+128+8 )
					{
#ifdef MAME_DEBUG
                        hpos_h = hpos;
                        vpos_h = vpos;
#endif
                        /* 7474/H3.2 is set by the HIT2 signal */
						score_q = 1;
						if( hit_sound == 0 )
						{
							pong_hit_sound = hit_sound = 1;
							if( speed < 10 )
								speed += 1;
						}
						vvel_b = (vpad2_count & 2) ? 0 : 1;
						vvel_c = (vpad2_count & 4) ? 0 : 1;
						vvel_d = (vpad2_count & 8) ? 0 : 1;
						/* 74107/A2.1 is cleared by any HIT signal */
						hit_vbl_q = 0;
                    }
					else
					if( HRESET )  /* no hit and HBLANK starts ? */
					{
						/* RESET SPEED */
						speed = 0;
						/*
						 * start the SCORE SOUND TIMER NE555/G4
						 */
						if( ATRACT && score_sound_timer == NULL )
						{
                            /* monoflop (NE555/G4) with a 220 kOhms resistor and 1.0 æF capacitor */
							score_sound_timer = timer_set(TIME_IN_USEC(1.2*220000.0*1.0), 0, score_timer_cb);
                            pong_score_sound = 1;
							/* raising edge of SC (output of NE555/G4) toggles D-type flip-flop 7474/H3.2 */
                            score_q ^= 1;
                            if( L )
							{
								if( ++score1 == ((readinputport(0) & 2) ? 15 : 11) )
								{
									STOP_G = 1;
									ATRACT = 0;
								}
							}
							if( R )
							{
								if( ++score2 == ((readinputport(0) & 2) ? 15 : 11) )
								{
									STOP_G = 1;
									ATRACT = 0;
								}
							}
                        }
						if( serve_timer == NULL )
						{
							/* monoflop (NE555/G4) with a 330 kOhms resistor and 4.7 æF capacitor */
							serve_timer = timer_set(TIME_IN_USEC(1.2*330000.0*4.7), 0, serve_timer_cb);
						}
					}
				}
			}
		}	/* !serve_timer */
	}	/* ATRACT */

	VRESET = 0;
	V++;

    return ignore_interrupt();
}

/****** HIT DETECTOR *********************************************************
 * It all starts with a 4-bit binary counter (7493/F1) which counts the
 * speed of the ball motion. This counter is clocked with the HIT SOUND
 * signal. The clock is inhibted by a NAND gate (7400/E1.4) when the counter
 * reaches 10. The counter bits 1 and 3 (speed2 and speed8) are then sent
 * into a combination of NAND and NOR gates. Their outputs are used to
 * clear the two JK flip-flops H2.1 and H2.2 while VRESET is hi.
 * The Q outputs of those JK flip-flops is sent into NAND gate 7400/H4.1
 * and it's output again is used to generate Aa/Ba from the L/R signals
 * of D flip-flop 7474/H3.2.
 *****************************************************************************/
INLINE void pong_hit_detector(void)
{
	int q, a;

	q = (speed_q1 == 1 && speed_q2 == 1) ? 0 : 1;
	a = ( L && q ) ? 0 : 1;
    /*
	 * The HIT DETECTOR outputs Aa and Ba (hit_a and hit_b)
	 */
	hit_b = ( R && q ) ? 0 : 1;
	hit_a = ( a == 0 || hit_b == 0 ) ? 1 : 0;
}

/****** VERTICAL VELOCITY ****************************************************
 * When a hit occurs, the vpad counter bits 1,2 and 3 of the VPAD
 * that was hit are latched in 7474/A5.2, 7474/A5.1 and 7474/B5.1
 * To be exact, the negated value of the bits is latched, because
 * the 7450 chips (A6.1, A6.2 and B6.2) which are used to de-multiplex
 * pad1/pad2 counters are AND/NOR gates.
 * The signals are called vvel_b, vvel_c and vvel_d here.
 * The second signal which is used is the state VVID during VBLANK,
 * so it means the ball has hit the upper or lower screen border.
 * This signal is called hit_vbl_q here.
 *****************************************************************************/
INLINE void pong_vertical_velocity(void)
{
	int a = 0;	/* A4 is tied to GND */
	int b = 6;	/* B2 and B3 are tied to Vcc, B4 tied to GND */

	/* inputs of XOR gate 7486/A4.3 */
	if( vvel_b != hit_vbl_q )
		a += 1; 			/* A1 */

	/* inputs of XOR gate 7486/A4.2 */
	if( vvel_c != hit_vbl_q )
		a += 2; 			/* A2 */

	/* inputs of AND/NOR gate 7450/B6.1 */
	if( vvel_d == hit_vbl_q )
		b += 1; 			/* equal: A3 = lo, B1 = hi */
	else
		a += 4; 			/* else:  A3 = hi, B1 = lo */

    /* Output of the 4-bit binary adder 7483/B4 */
	v_velocity = (a + b) & 15;
}


/*
 * Decode a value into it's active 7-segment display bars and
 * draw them into PONG's screenbitmap using the logic from the
 * schematics.
 */
INLINE void pong_7seg(int h0, int n)
{
	/*
	 * This is what an 7448 chip produces at it's outputs
	 * a-g for input values betwee 0 and 15:
	 * 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
	 * +-+	 + +-+ +-+ + + +-+ +-+ +-+ +-+ +-+		   + + +-+ +
	 * | |	 |	 |	 | | | |   |	 | | | | |		   | | |   |
	 * | |	 | +-+ +-+ +-+ +-+ +-+	 | +-+ +-+ +-+ +-+ +-+ +-+ +-+
	 * | |	 | |	 |	 |	 | | |	 | | | | |	 | |		   |
	 * +-+	 + +-+ +-+	 + +-+ +-+	 + +-+ +-+ +-+ +-+	   +-+ +-+
	 */
    static UINT8 decode_7seg[16][7] = {
		{1,  1,  1,  1,  1,  1,  0},	/*	1 */
		{0,  1,  1,  0,  0,  0,  0},	/*	0 */
		{1,  1,  0,  1,  1,  0,  1},	/*	2 */
		{1,  1,  1,  1,  0,  0,  1},	/*	3 */
		{0,  1,  1,  0,  0,  1,  1},	/*	4 */
		{1,  0,  1,  1,  0,  1,  1},	/*	5 */
		{1,  0,  1,  1,  1,  1,  1},	/*	6 */
		{1,  1,  1,  0,  0,  0,  0},	/*	7 */
		{1,  1,  1,  1,  1,  1,  1},	/*	8 */
		{1,  1,  1,  1,  0,  1,  1},	/*	9 */
		{0,  0,  0,  1,  1,  0,  1},	/* 10 */
		{0,  0,  1,  1,  0,  1,  1},	/* 11 */
		{0,  1,  0,  0,  0,  1,  1},	/* 12 */
		{1,  0,  0,  1,  0,  1,  1},	/* 13 */
		{0,  0,  0,  1,  1,  1,  1},	/* 14 */
		{0,  0,  0,  0,  0,  0,  0},	/* 15 */
	};
	int H;

    n %= 16;

	/*
	 * We can step in paces of 4 pixels, because the mininum H
	 * signal which has an effect on the display is 4H
	 */
	for( H = h0; H < h0 + 32; H += 4 )
	{
		int f = (decode_7seg[n][5] && (!H4 && !H8 && !!H16) && !V16) ? 0 : 1;
		int e = (decode_7seg[n][4] && V16 && (!H4 && !H8 && !!H16)) ? 0 : 1;
		int b = (decode_7seg[n][1] && (!!(H4 && H8) && !!H16) && !V16) ? 0 : 1;
		int c = (decode_7seg[n][2] && (!!(H4 && H8) && !!H16) && V16) ? 0 : 1;
		int a = (decode_7seg[n][0] && !V16 && (H16 && !V8 && !V4)) ? 0 : 1;
		int g = (decode_7seg[n][6] && !V16 && (V8 && V4 && H16)) ? 0 : 1;
		int d = (decode_7seg[n][3] && V16 && (V8 && V4 && H16)) ? 0 : 1;

		int score = (!a || !b || !c || !d || !e || !f || !g);

		if( score )
		{
			int pen2 = Machine->pens[2];
			plot_pixel(tmpbitmap, H + 0, V, pen2);
			plot_pixel(tmpbitmap, H + 1, V, pen2);
			plot_pixel(tmpbitmap, H + 2, V, pen2);
			plot_pixel(tmpbitmap, H + 3, V, pen2);
		}
    }
}

