#ifndef __MultiPCM_H__
#define __MultiPCM_H__

// banking types (done via an external PAL, so can vary per game)
#define MULTIPCM_MODE_MODEL1	(0)	// Model 1 banking method
#define MULTIPCM_MODE_MULTI32	(1)	// Multi32 banking method
#define MULTIPCM_MODE_STADCROSS (2)	// Stadium Cross banking method

struct MultiPCM_interface
{
	int type;
	int banksize;
	int region;
};


WRITE8_HANDLER( MultiPCM_reg_0_w );
READ8_HANDLER( MultiPCM_reg_0_r);
WRITE8_HANDLER( MultiPCM_reg_1_w );
READ8_HANDLER( MultiPCM_reg_1_r);
WRITE8_HANDLER( MultiPCM_bank_0_w );
WRITE8_HANDLER( MultiPCM_bank_1_w );

#endif
