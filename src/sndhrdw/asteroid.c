/*****************************************************************************
 *
 * Asteroids Analog Sound system interface into discrete sound emulation
 * input mapping system.
 *
 *****************************************************************************/

#include <math.h>
#include "driver.h"

/************************************************************************/
/* Asteroids Sound System Analog emulation by K.Wilkins Nov 2000        */
/* Questions/Suggestions to mame@dysfunction.demon.co.uk                */
/************************************************************************/

#define	ASTEROID_THUMP_ENAB		NODE_01
#define	ASTEROID_THUMP_FREQ		NODE_02
#define	ASTEROID_SAUCER_ENAB	NODE_03
#define	ASTEROID_SAUCER_FIRE	NODE_04
#define	ASTEROID_SAUCER_SEL		NODE_05
#define	ASTEROID_THRUST_ENAB	NODE_06
#define	ASTEROID_FIRE_ENAB		NODE_07
#define	ASTEROID_LIFE_ENAB		NODE_08
#define ASTEROID_EXPLODE_NODE	NODE_09
#define ASTEROID_EXPLODE_PITCH	NODE_17
#define ASTEROID_THUMP_DUTY		NODE_18

DISCRETE_SOUND_START(asteroid_sound_interface)
	/************************************************/
	/* Input register mapping for asteroids ,the    */
	/* registers are lumped in three groups for no  */
	/* other reason than they are controlled by 3   */
	/* registers on the schematics                  */
	/* Address values are also arbitary in here.    */
	/************************************************/
        /*                   NODE              ADDR   MASK INIT                 GAIN    OFFSET */
        DISCRETE_INPUT (ASTEROID_SAUCER_ENAB  ,0x00,0x003f,  0)
        DISCRETE_INPUT (ASTEROID_SAUCER_FIRE  ,0x01,0x003f,  0)
        DISCRETE_INPUT (ASTEROID_SAUCER_SEL   ,0x02,0x003f,  0)
        DISCRETE_INPUT (ASTEROID_THRUST_ENAB  ,0x03,0x003f,  0)
        DISCRETE_INPUT (ASTEROID_FIRE_ENAB    ,0x04,0x003f,  0)
        DISCRETE_INPUT (ASTEROID_LIFE_ENAB    ,0x05,0x003f,  0)

        DISCRETE_INPUT (ASTEROID_THUMP_ENAB   ,0x10,0x003f,  0)
        DISCRETE_INPUTX(ASTEROID_THUMP_FREQ   ,0x11,0x003f,  (70.0/15.0)       ,20.0    ,0)
        DISCRETE_INPUTX(ASTEROID_THUMP_DUTY   ,0x12,0x003f,  (55.0/15.0)       ,33.0    ,0)

        DISCRETE_INPUTX(ASTEROID_EXPLODE_NODE ,0x20,0x003f,  ((1.0/15.0)*150.0),0.0     ,0)
        DISCRETE_INPUTX(ASTEROID_EXPLODE_PITCH,0x21,0x003f,  1000.0            ,0.0     ,0)

	/************************************************/
	/* Thump circuit is based on a VCO with the     */
	/* VCO control fed from the 4 low order bits    */
	/* from /THUMP bit 4 controls the osc enable.   */
	/* A resistor ladder network is used to convert */
	/* the 4 bit value to an analog value.          */
	/*                                              */
	/* The VCO is implemented with a 555 timer and  */
	/* an RC filter to perform smoothing on the     */
	/* output                                       */
	/*                                              */
	/* The sound can be tweaked with the gain and   */
	/* adder constants in the 2 lines below         */
	/************************************************/
        DISCRETE_SQUAREWAVE(NODE_11,ASTEROID_THUMP_ENAB,ASTEROID_THUMP_FREQ,40.0,ASTEROID_THUMP_DUTY,0)
	DISCRETE_RCFILTER(NODE_10,1,NODE_11,3300,0.1e-6)

	/************************************************/
	/* The SAUCER sound is based on two VCOs, a     */
	/* slow VCO feed the input to a higher freq VCO */
	/* with the SAUCERSEL switch being used to move */
	/* the frequency ranges of both VCOs            */
	/*                                              */
	/* The slow VCO is implemented with a 555 timer */
	/* and a 566 is used for the higher VCO.        */
	/*                                              */
	/* The sound can be tweaked with the gain and   */
	/* adder constants in the 2 lines below         */
	/************************************************/

        // SAUCER_SEL = 1 - larger saucer
        DISCRETE_GAIN(NODE_28,ASTEROID_SAUCER_SEL,-2.0)             // Freq Warble jump (0 or -2)
        DISCRETE_ADDER2(NODE_27,1,NODE_28,8.0)                      // 6 or 8 (warbles per s)
        DISCRETE_SINEWAVE(NODE_26,1,NODE_27,300.0,0)                // +300 .. -300 (amount of warble)

        DISCRETE_GAIN(NODE_25,ASTEROID_SAUCER_SEL,-300)             // Large saucer is 300hz lower
        DISCRETE_ADDER3(NODE_24,1,NODE_26,1300,NODE_25)             // Create fequency we want

        DISCRETE_GAIN(NODE_23,ASTEROID_SAUCER_SEL,-40)              // Large saucer has less volume
        DISCRETE_ADDER2(NODE_22,1,100,NODE_23)                      // Create gain we want

        DISCRETE_TRIANGLEWAVE(NODE_21,ASTEROID_SAUCER_ENAB,NODE_24,NODE_22)
        DISCRETE_RCFILTER(NODE_20,1,NODE_21,1,1.0e-5)


	/************************************************/
	/* The Fire sound is produced by a 555 based    */
	/* VCO where the frequency rapidly decays with  */
	/* time.                                        */
	/*                                              */
        /* Use a discharging RC for the decay           */
	/************************************************/
        DISCRETE_RCDISC(NODE_39,ASTEROID_FIRE_ENAB, 1000, 1000.0,1e-4)  // output: 1000 .. down
        DISCRETE_GAIN(NODE_38,NODE_39,0.04)                             // reduce volume over time
        DISCRETE_SQUAREWAVE(NODE_37,ASTEROID_FIRE_ENAB,NODE_39,NODE_38, 45,0)   // use sq wave to get the "squak"
        DISCRETE_RCFILTER(NODE_30,1,NODE_37,1,1.0e-5)


	/************************************************/
	/* The Fire sound is produced by a 555 based    */
	/* VCO where the frequency rapidly decays with  */
	/* time.                                        */
	/*                                              */
        /* Use a discharging RC for the decay           */
	/************************************************/
        DISCRETE_RCDISC(NODE_49,ASTEROID_SAUCER_FIRE,750, 1000.0,0.008)
        DISCRETE_GAIN(NODE_48,NODE_49,0.035)                     // reduce volume over time
        DISCRETE_SQUAREWAVE(NODE_40,ASTEROID_SAUCER_FIRE,NODE_49,NODE_48, 30,0)

	/************************************************/
	/* Thrust noise is just a gated noise source    */
	/* fed into a low pass RC filter                */
	/************************************************/
        DISCRETE_NOISE(NODE_51,ASTEROID_THRUST_ENAB,210,70.0)
        DISCRETE_RCFILTER(NODE_50,1,NODE_51,1000,1e-6)

	/************************************************/
	/* Explosion generation circuit, pitch and vol  */
	/* are variable                                 */
	/************************************************/

        // make high pitch, or allows higher pitch through circuit
        //DISCRETE_NOISE(NODE_61,1,ASTEROID_EXPLODE_PITCH,ASTEROID_EXPLODE_NODE)
        //DISCRETE_RCFILTER(NODE_60,1,NODE_61,40,1e-6)

        DISCRETE_NOISE(NODE_61,1,ASTEROID_EXPLODE_PITCH,ASTEROID_EXPLODE_NODE)
        DISCRETE_RCFILTER(NODE_60,1,NODE_61,400,1e-6)

	/************************************************/
	/* Life enable is just 3KHz tone from the clock */
	/* generation cct according to schematics       */
	/************************************************/
	DISCRETE_SINEWAVE(NODE_70,ASTEROID_LIFE_ENAB,3000,100.0,0)

	/************************************************/
	/* Combine all 7 sound sources with a double    */
	/* adder circuit                                */
	/************************************************/
        DISCRETE_ADDER4(NODE_91,1,NODE_10,NODE_20,NODE_30,NODE_40)
        DISCRETE_ADDER4(NODE_92,1,NODE_50,NODE_60,NODE_70,NODE_91)
        DISCRETE_GAIN(NODE_90,NODE_92,60.0)

        DISCRETE_OUTPUT(NODE_90)                                                                                                                // Take the output from the mixer
DISCRETE_SOUND_END


DISCRETE_SOUND_START(astdelux_sound_interface)
	/************************************************/
	/* Asteroid delux sound hardware is mostly done */
	/* in the Pokey chip except for the thrust and  */
	/* explosion sounds that are a direct lift of   */
	/* the asteroids hardware hence is a clone of   */
	/* the circuit above apart from gain scaling.   */
	/*                                              */
	/* Note that the thrust enable signal is invert */
	/************************************************/
	DISCRETE_INPUTX(ASTEROID_THRUST_ENAB  ,0x03,0x003f,-1.0              ,1.0,1.0)
	DISCRETE_INPUTX(ASTEROID_EXPLODE_NODE ,0x20,0x003f,((1.0/15.0)*100.0),0.0,0.0)
	DISCRETE_INPUTX(ASTEROID_EXPLODE_PITCH,0x21,0x003f,1000.0            ,0.0,0.0)

	/************************************************/
	/* Thrust noise is just a gated noise source    */
	/* fed into a low pass RC filter                */
	/************************************************/
        DISCRETE_NOISE(NODE_51,ASTEROID_THRUST_ENAB,210,35.0)
        DISCRETE_RCFILTER(NODE_50,1,NODE_51,1000,1e-6)

	/************************************************/
	/* Explosion generation circuit, pitch and vol  */
	/* are variable                                 */
	/************************************************/
	DISCRETE_NOISE(NODE_61,1,ASTEROID_EXPLODE_PITCH,ASTEROID_EXPLODE_NODE)
	DISCRETE_RCFILTER(NODE_60,1,NODE_61,400,1e-6)

	/************************************************/
	/* Combine all 7 sound sources with a double    */
	/* adder circuit                                */
	/************************************************/
	DISCRETE_ADDER2(NODE_91,1,NODE_50,NODE_60)
	DISCRETE_GAIN(NODE_90,NODE_91,160.0)

	DISCRETE_OUTPUT(NODE_90)														// Take the output from the mixer
DISCRETE_SOUND_END


WRITE_HANDLER( asteroid_explode_w )
{
	discrete_sound_w(0x20,(data&0x3c)>>2);				// Volume
	discrete_sound_w(0x21,12/(((data&0xc0)>>6)+1));		// Noise Pitch divider 12KHz / (1+Value) in KHz
}

WRITE_HANDLER( asteroid_thump_w )
{
	discrete_sound_w(0x10,data&0x10);			//Thump enable
	discrete_sound_w(0x11,(data&0x0f)^0x0f);	//Thump frequency
	discrete_sound_w(0x12,data&0x0f);			//Thump duty
}

WRITE_HANDLER( asteroid_sounds_w )
{
	discrete_sound_w(0x00+offset,(data&0x80)?1:0);
}

WRITE_HANDLER( astdelux_sounds_w )
{
	/* Only ever activates the thrusters in Astdelux */
	discrete_sound_w(0x03,(data&0x80)?1:0);
}
