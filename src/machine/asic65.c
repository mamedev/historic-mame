#include "driver.h"


/*************************************
 *
 *	!$#@$ asic
 *
 *************************************/

static int    asic65_command;
static UINT8  asic65_command_ready;
static UINT16 asic65_param[32];
static UINT8  asic65_param_index;
static UINT16 asic65_result[32];
static UINT8  asic65_result_index;
static UINT8  asic65_result_ready;
static UINT8  asic65_reset_state;

static FILE * asic65_log;

WRITE16_HANDLER( asic65_w );
WRITE16_HANDLER( asic65_data_w );

READ16_HANDLER( asic65_r );
READ16_HANDLER( asic65_io_r );


#define PARAM_WRITE		0
#define COMMAND_WRITE	1
#define DATA_READ		2



/*************************************
 *
 *	Reset the chip
 *
 *************************************/

void asic65_reset(int state)
{
	/* if reset is being signalled, clear everything */
	if (state && !asic65_reset_state)
	{
		asic65_command = -1;
		asic65_result_ready = 0;
		asic65_command_ready = 0;
	}
	
	/* if reset is going high, latch the command */
	else if (!state && asic65_reset_state)
	{
		if (asic65_command != -1)
			asic65_data_w(1, asic65_command, 0);
	}
	
	/* update the state */
	asic65_reset_state = state;
}



/*************************************
 *
 *	Command 01: Writeback
 *
 *************************************/

static UINT16 writeback_data;

static int update_command_01(int action, int data)
{
	int result = 0;
	
	/* on a parameter write, store the data */
	if (action == PARAM_WRITE)
	{
		writeback_data = data;
		asic65_result_ready = 1;
	}
	
	/* on a parameter read, return the data */
	else if (action == DATA_READ)
	{
		result = writeback_data;
		asic65_result_ready = 0;
	}
	
	return result;
}



/*************************************
 *
 *	Command 02: Checksum
 *
 *************************************/

static UINT16 checksum_data;

static void checksum_ready(int param)
{
	checksum_data = param;
	asic65_result_ready = 1;
}

static int update_command_02(int action, int data)
{
	int result = 0;
	
	/* on a command write, start the timer */
	if (action == PARAM_WRITE)
		timer_set(TIME_IN_HZ(30), 0x3159, checksum_ready);
	
	/* on a parameter read, return the data */
	else if (action == DATA_READ)
	{
		result = checksum_data;
		asic65_result_ready = 0;
	}
	
	return result;
}



/*************************************
 *
 *	Handle writes to the chip
 *
 *************************************/

WRITE16_HANDLER( asic65_data_w )
{
	int is_command = offset & 1;
	
	/* logging */
	if (!asic65_log) asic65_log = fopen("asic65.log", "w");
	
	/* if it's being written to the command register, latch it */
	if (is_command)
	{
//		if (asic65_log) fprintf(asic65_log, "\n(%06X) %04X:", activecpu_get_previouspc(), data);
		asic65_command = data;
		asic65_command_ready = 1;
	}
	else
	{
//		if (asic65_log) fprintf(asic65_log, " W=%04X", data);
	}
	
	/* switch off the command number */
	switch (asic65_command)
	{
		case 0x01:	/* writeback */
			update_command_01(is_command, data);
			break;

		case 0x02:	/* checksum */
			update_command_02(is_command, data);
			break;
	}
	
	/* parameters go to offset 0 */
	if (offset == 0)
	{
		if (asic65_log) fprintf(asic65_log, " W=%04X", data);

		asic65_param[asic65_param_index++] = data;
		if (asic65_param_index >= 32)
			asic65_param_index = 32;
	}

	/* commands go to offset 2 */
	else
	{
		if (asic65_log) fprintf(asic65_log, "\n(%06X) %04X:", activecpu_get_previouspc(), data);

		asic65_command = data;
		asic65_result_index = asic65_param_index = 0;
	}

	/* update results */
	switch (asic65_command)
	{
		case 0x01:	/* reflect data */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x02:	/* compute checksum (should be XX27) */
			asic65_result[0] = 0x0027;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x03:	/* get version (returns 1.3) */
			asic65_result[0] = 0x0013;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x04:	/* internal RAM test (result should be 0) */
			asic65_result[0] = 0;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x10:	/* matrix multiply */
			if (asic65_param_index >= 9+6)
			{
				INT64 element, result;

				element = (INT32)((asic65_param[9] << 16) | asic65_param[10]);
				result = element * (INT16)asic65_param[0] +
						 element * (INT16)asic65_param[1] +
						 element * (INT16)asic65_param[2];
				result >>= 14;
				asic65_result[0] = result >> 16;
				asic65_result[1] = result & 0xffff;

				element = (INT32)((asic65_param[11] << 16) | asic65_param[12]);
				result = element * (INT16)asic65_param[3] +
						 element * (INT16)asic65_param[4] +
						 element * (INT16)asic65_param[5];
				result >>= 14;
				asic65_result[2] = result >> 16;
				asic65_result[3] = result & 0xffff;

				element = (INT32)((asic65_param[13] << 16) | asic65_param[14]);
				result = element * (INT16)asic65_param[6] +
						 element * (INT16)asic65_param[7] +
						 element * (INT16)asic65_param[8];
				result >>= 14;
				asic65_result[4] = result >> 16;
				asic65_result[5] = result & 0xffff;
			}
			break;

		case 0x14:	/* ??? */
			if (asic65_param_index >= 1)
			{
				if (asic65_param[0] != 0)
					asic65_result[0] = (0x40000000 / (INT16)asic65_param[0]) >> 16;
				else
					asic65_result[0] = 0x7fff;
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x15:	/* ??? */
			if (asic65_param_index >= 1)
			{
				if (asic65_param[0] != 0)
					asic65_result[0] = (0x40000000 / (INT16)asic65_param[0]);
				else
					asic65_result[0] = 0xffff;
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x17:	/* vector scale */
			if (asic65_param_index >= 2)
			{
				asic65_result[0] = ((INT16)asic65_param[0] * (INT16)asic65_param[1]) >> 15;
				asic65_result_index = 0;
				asic65_param_index = 1;
			}
			break;
	}
}


#if 0
WRITE16_HANDLER( asic65_w )
{
	if (!asic65_log) asic65_log = fopen("asic65.log", "w");
	
	/* parameters go to offset 0 */
	if (offset == 0)
	{
		if (asic65_log) fprintf(asic65_log, " W=%04X", data);

		asic65_param[asic65_param_index++] = data;
		if (asic65_param_index >= 32)
			asic65_param_index = 32;
	}

	/* commands go to offset 2 */
	else
	{
		if (asic65_log) fprintf(asic65_log, "\n(%06X) %04X:", activecpu_get_previouspc(), data);

		asic65_command = data;
		asic65_result_index = asic65_param_index = 0;
	}

	/* update results */
	switch (asic65_command)
	{
		case 0x01:	/* reflect data */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x02:	/* compute checksum (should be XX27) */
			asic65_result[0] = 0x0027;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x03:	/* get version (returns 1.3) */
			asic65_result[0] = 0x0013;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x04:	/* internal RAM test (result should be 0) */
			asic65_result[0] = 0;
			asic65_result_index = asic65_param_index = 0;
			break;

		case 0x10:	/* matrix multiply */
			if (asic65_param_index >= 9+6)
			{
				INT64 element, result;

				element = (INT32)((asic65_param[9] << 16) | asic65_param[10]);
				result = element * (INT16)asic65_param[0] +
						 element * (INT16)asic65_param[1] +
						 element * (INT16)asic65_param[2];
				result >>= 14;
				asic65_result[0] = result >> 16;
				asic65_result[1] = result & 0xffff;

				element = (INT32)((asic65_param[11] << 16) | asic65_param[12]);
				result = element * (INT16)asic65_param[3] +
						 element * (INT16)asic65_param[4] +
						 element * (INT16)asic65_param[5];
				result >>= 14;
				asic65_result[2] = result >> 16;
				asic65_result[3] = result & 0xffff;

				element = (INT32)((asic65_param[13] << 16) | asic65_param[14]);
				result = element * (INT16)asic65_param[6] +
						 element * (INT16)asic65_param[7] +
						 element * (INT16)asic65_param[8];
				result >>= 14;
				asic65_result[4] = result >> 16;
				asic65_result[5] = result & 0xffff;
			}
			break;

		case 0x14:	/* ??? */
			if (asic65_param_index >= 1)
			{
				if (asic65_param[0] != 0)
					asic65_result[0] = (0x40000000 / (INT16)asic65_param[0]) >> 16;
				else
					asic65_result[0] = 0x7fff;
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x15:	/* ??? */
			if (asic65_param_index >= 1)
			{
				if (asic65_param[0] != 0)
					asic65_result[0] = (0x40000000 / (INT16)asic65_param[0]);
				else
					asic65_result[0] = 0xffff;
				asic65_result_index = asic65_param_index = 0;
			}
			break;

		case 0x17:	/* vector scale */
			if (asic65_param_index >= 2)
			{
				asic65_result[0] = ((INT16)asic65_param[0] * (INT16)asic65_param[1]) >> 12;
				asic65_result_index = 0;
				asic65_param_index = 1;
			}
			break;
	}
}
#endif


READ16_HANDLER( asic65_r )
{
	int result;

	if (!asic65_log) asic65_log = fopen("asic65.log", "w");
	if (asic65_log) fprintf(asic65_log, " (R=%04X)", asic65_result[asic65_result_index]);

	/* return the next result */
	result = asic65_result[asic65_result_index++];
	if (asic65_result_index >= 32)
		asic65_result_index = 32;
	return result;
}


READ16_HANDLER( asic65_io_r )
{
	/* indicate that we always are ready to accept data and always ready to send */
	return 0x4000;
}
