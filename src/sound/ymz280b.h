/**********************************************************************************************
 *
 *   Yamaha YMZ280B driver
 *   by Aaron Giles
 *
 **********************************************************************************************/

#ifndef YMZ280B_H
#define YMZ280B_H

struct YMZ280Binterface
{
	int region;						/* memory region where the sample ROM lives */
	void (*irq_callback)(int state);	/* irq callback */
};

READ8_HANDLER ( YMZ280B_status_0_r );
WRITE8_HANDLER( YMZ280B_register_0_w );
WRITE8_HANDLER( YMZ280B_data_0_w );

READ16_HANDLER ( YMZ280B_status_0_lsb_r );
READ16_HANDLER ( YMZ280B_status_0_msb_r );
WRITE16_HANDLER( YMZ280B_register_0_lsb_w );
WRITE16_HANDLER( YMZ280B_register_0_msb_w );
WRITE16_HANDLER( YMZ280B_data_0_lsb_w );
WRITE16_HANDLER( YMZ280B_data_0_msb_w );

READ8_HANDLER ( YMZ280B_status_1_r );
WRITE8_HANDLER( YMZ280B_register_1_w );
WRITE8_HANDLER( YMZ280B_data_1_w );

READ16_HANDLER ( YMZ280B_status_1_lsb_r );
READ16_HANDLER ( YMZ280B_status_1_msb_r );
WRITE16_HANDLER( YMZ280B_register_1_lsb_w );
WRITE16_HANDLER( YMZ280B_register_1_msb_w );
WRITE16_HANDLER( YMZ280B_data_1_lsb_w );
WRITE16_HANDLER( YMZ280B_data_1_msb_w );

#endif
