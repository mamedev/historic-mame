/*************************************************************************

	sndhrdw\sprint2.c

*************************************************************************/
#include "driver.h"
#include "sprint2.h"
#include "sound/discrete.h"


/************************************************************************/
/* sprint2 Sound System Analog emulation                                */
/************************************************************************/

const struct discrete_lfsr_desc sprint2_lfsr={
	DISC_CLK_IS_FREQ,
	16,                  /* Bit Length */
	0,                   /* Reset Value */
	0,                   /* Use Bit 0 as XOR input 0 */
	14,                  /* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,      /* Feedback stage1 is XNOR */
	DISC_LFSR_OR,        /* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,   /* Feedback stage3 replaces the shifted register contents */
	0x000001,            /* Everything is shifted into the first bit only */
	0,                   /* Output is not inverted */
	15                   /* Output bit */
};

/* Nodes - Sounds */
#define SPRINT2_MOTORSND1          NODE_10
#define SPRINT2_MOTORSND2          NODE_11
#define SPRINT2_CRASHSND           NODE_12
#define SPRINT2_SKIDSND1           NODE_13
#define SPRINT2_SKIDSND2           NODE_14
#define SPRINT2_NOISE              NODE_15
#define SPRINT2_FINAL_MIX1         NODE_16
#define SPRINT2_FINAL_MIX2         NODE_17

DISCRETE_SOUND_START(sprint2_discrete_interface)
	/************************************************/
	/* Sprint2 sound system: 5 Sound Sources        */
	/*                     Relative Volume          */
	/*    1/2) Motor           84.69%               */
	/*      2) Crash          100.00%               */
	/*    4/5) Screech/Skid    40.78%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint2           */
	/************************************************/
	DISCRETE_INPUT_LOGIC(SPRINT2_SKIDSND1_EN)
	DISCRETE_INPUT_LOGIC(SPRINT2_SKIDSND2_EN)
	DISCRETE_INPUT_DATA (SPRINT2_MOTORSND1_DATA)
	DISCRETE_INPUT_DATA (SPRINT2_MOTORSND2_DATA)
	DISCRETE_INPUTX_DATA(SPRINT2_CRASHSND_DATA, 1000.0/15.0, 0,   0)
	DISCRETE_INPUT_NOT  (SPRINT2_ATTRACT_EN)
	DISCRETE_INPUT_PULSE(SPRINT2_NOISE_RESET, 1)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 10uf capacitor on the R-ladder.         */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, SPRINT2_MOTORSND1_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 8)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND1, SPRINT2_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Car2 motor sound is basically the same as    */
	/* Car1.  But I shifted the frequencies up for  */
	/* it to sound different from car1.             */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, SPRINT2_MOTORSND2_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 9)
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND2, SPRINT2_ATTRACT_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT2_NOISE, 1, SPRINT2_NOISE_RESET, 15750.0/4, 1.0, 0, 0, &sprint2_lfsr)

	DISCRETE_MULTIPLY(NODE_60, 1, SPRINT2_NOISE, SPRINT2_CRASHSND_DATA)
	DISCRETE_RCFILTER(SPRINT2_CRASHSND, 1, NODE_60, 545, 1e-7)

	/************************************************/
	/* Skid circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, SPRINT2_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_73, 1, NODE_72, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(SPRINT2_SKIDSND1, SPRINT2_SKIDSND1_EN, NODE_73)
	DISCRETE_ONOFF(SPRINT2_SKIDSND2, SPRINT2_SKIDSND2_EN, NODE_73)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, SPRINT2_MOTORSND1, SPRINT2_CRASHSND, SPRINT2_SKIDSND1)
	DISCRETE_ADDER3(NODE_91, 1, SPRINT2_MOTORSND2, SPRINT2_CRASHSND, SPRINT2_SKIDSND2)
	DISCRETE_GAIN(SPRINT2_FINAL_MIX1, NODE_90, 65534.0/(846.9+1000.0+407.8))
	DISCRETE_GAIN(SPRINT2_FINAL_MIX2, NODE_91, 65534.0/(846.9+1000.0+407.8))

	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX2, 100)
	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX1, 100)
DISCRETE_SOUND_END


DISCRETE_SOUND_START(sprint1_discrete_interface)
	/************************************************/
	/* Sprint1 sound system: 3 Sound Sources        */
	/*                     Relative Volume          */
	/*      1) Motor           84.69%               */
	/*      2) Crash          100.00%               */
	/*      3) Screech/Skid    40.78%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint1           */
	/************************************************/
	DISCRETE_INPUT_LOGIC(SPRINT2_SKIDSND1_EN)
	DISCRETE_INPUT_DATA (SPRINT2_MOTORSND1_DATA)
	DISCRETE_INPUTX_DATA(SPRINT2_CRASHSND_DATA, 1000.0/15.0, 0, 0)
	DISCRETE_INPUT_NOT  (SPRINT2_ATTRACT_EN)
	DISCRETE_INPUT_PULSE(SPRINT2_NOISE_RESET, 1)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 10uf capacitor on the R-ladder.         */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, SPRINT2_MOTORSND1_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 8)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND1, SPRINT2_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT2_NOISE, 1, SPRINT2_NOISE_RESET, 15750.0/4, 1.0, 0, 0, &sprint2_lfsr)

	DISCRETE_MULTIPLY(NODE_60, SPRINT2_CRASHSND_DATA, SPRINT2_NOISE, SPRINT2_CRASHSND_DATA)
	DISCRETE_RCFILTER(SPRINT2_CRASHSND, SPRINT2_CRASHSND_DATA, NODE_60, 545, 1e-7)

	/************************************************/
	/* Skid circuit takes the noise output from     */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, SPRINT2_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(SPRINT2_SKIDSND1, SPRINT2_SKIDSND1_EN, NODE_72, 407.8, 31.5, 0, 0.0)

	/************************************************/
	/* Combine all 3 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, SPRINT2_MOTORSND1, SPRINT2_CRASHSND, SPRINT2_SKIDSND1)
	DISCRETE_GAIN(SPRINT2_FINAL_MIX1, NODE_90, 65534.0/(846.9+1000.0+407.8))

	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX1, 100)
DISCRETE_SOUND_END


/************************************************************************/
/* dominos Sound System Analog emulation                               */
/************************************************************************/

/* Nodes - Sounds */
#define DOMINOS_TONE_SND           NODE_10
#define DOMINOS_TOPPLE_SND         NODE_11

DISCRETE_SOUND_START(dominos_discrete_interface)
	/************************************************/
	/* Dominos Effects Relataive Gain Table         */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Tone          3.8      5/(5+10)     1000.0   */
	/* Topple        3.8      5/(33+5)      394.7   */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for dominos           */
	/************************************************/
	DISCRETE_INPUT_LOGIC(DOMINOS_TUMBLE_EN)
	DISCRETE_INPUT_DATA (DOMINOS_FREQ_DATA)
	DISCRETE_INPUTX_DATA(DOMINOS_AMP_DATA, 1000.0/15, 0,     0)
	DISCRETE_INPUT_NOT  (DOMINOS_ATTRACT_EN)

	/************************************************/
	/* Tone Sound                                   */
	/* Note: True freq range has not been tested    */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, DOMINOS_FREQ_DATA, 123000, 1e-7)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (289.0-95.0)/3/15, (4500.0-95.0)/3/15, DISC_LOGADJ, 4)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_ADDER2(NODE_23, 1, NODE_22, 95.0/3)
	DISCRETE_SQUAREWAVE(NODE_24, DOMINOS_ATTRACT_EN, NODE_23, DOMINOS_AMP_DATA, 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(DOMINOS_TONE_SND, 1, NODE_24, 546, 1e-7)

	/************************************************/
	/* Topple sound is just the 4V source           */
	/* 4V = HSYNC/8                                 */
	/*    = 15750/8                                 */
	/************************************************/
	DISCRETE_SQUAREWFIX(DOMINOS_TOPPLE_SND, DOMINOS_TUMBLE_EN, 15750.0/8, 394.7, 50.0, 0, 0)

	/************************************************/
	/* Combine both sound sources.                  */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER2(NODE_90, 1, DOMINOS_TONE_SND, DOMINOS_TOPPLE_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0+394.7))

	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END
