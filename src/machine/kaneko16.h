/***************************************************************************


							MCU Code Simulation


***************************************************************************/

extern data16_t *mcu_ram; /* for calc3 and toybox */


/***************************************************************************
								Gals Panic (set 2)
								Gals Panic (set 3)
								Sand Scorpion
***************************************************************************/

READ16_HANDLER( galpanib_calc_r );
WRITE16_HANDLER( galpanib_calc_w );


/***************************************************************************
								CALC3 MCU:

								Shogun Warriors
								Fujiyama Buster
								B.Rap Boys
***************************************************************************/

void calc3_mcu_init(void);
WRITE16_HANDLER( calc3_mcu_ram_w );
WRITE16_HANDLER( calc3_mcu_com0_w );
WRITE16_HANDLER( calc3_mcu_com1_w );
WRITE16_HANDLER( calc3_mcu_com2_w );
WRITE16_HANDLER( calc3_mcu_com3_w );


/***************************************************************************
								TOYBOX MCU:

								Blood Warrior
								Great 1000 Miles Rally
								...
***************************************************************************/

void toybox_mcu_init(void);
WRITE16_HANDLER( toybox_mcu_com0_w );
WRITE16_HANDLER( toybox_mcu_com1_w );
WRITE16_HANDLER( toybox_mcu_com2_w );
WRITE16_HANDLER( toybox_mcu_com3_w );


