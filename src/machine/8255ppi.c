/* INTEL 8255 PPI I/O chip */


/* NOTE: When port is input, then data present on the ports
   outputs is 0xff */

/* KT 10/01/2000 - Added bit set/reset feature for control port */
/*               - Added more accurate port i/o data handling */
/*               - Added output reset when control mode is programmed */



#include "driver.h"
#include "machine/8255ppi.h"


static void	ppi8255_set_intra(int which);

static int num;

/* mode 2 inte flags */
#define PPI8255_INTE1_FLAG (1<<6)
#define PPI8255_INTE2_FLAG (1<<4)

typedef struct
{
	read8_handler portAread;
	read8_handler portBread;
	read8_handler portCread;
	write8_handler portAwrite;
	write8_handler portBwrite;
	write8_handler portCwrite;
	int groupA_mode;
	int groupB_mode;
	int io[3];		/* input output status */
	int latch[3];	/* data written to ports */
	
	/* mode 2 mode data */
	write8_handler obfa_write;
	write8_handler intra_write;
	write8_handler ibfa_write;
	int inte_flags;
} ppi8255;



static ppi8255 chips[MAX_8255];

static void set_mode(int which, int data, int call_handlers);

static void ppi8255_obfa_w(int which, int data)
{
	ppi8255 *chip = &chips[which];

	if (data)
		chip->latch[2]|=(1<<7);
	else
		chip->latch[2]&=~(1<<7);
	
	/* a low on this output indicates that CPU has written data out to port A */
	if (chip->obfa_write)
	{
		logerror("8255 ppi output obfa: %d\n",data & 0x01);
		chip->obfa_write(0,data & 0x01);
	}
}

static void ppi8255_ibfa_w(int which, int data)
{
	ppi8255 *chip = &chips[which];

	if (data)
		chip->latch[2]|=(1<<5);
	else
		chip->latch[2]&=~(1<<5);

	if (chip->ibfa_write)
	{
		logerror("8255 ppi output ibfa: %d\n",data & 0x01);
		chip->ibfa_write(0,data & 0x01);
	}

}


void ppi8255_init( ppi8255_interface *intfce )
{
	int i;

	num = intfce->num;

	for (i = 0; i < num; i++)
	{
		chips[i].portAread = intfce->portAread[i];
		chips[i].portBread = intfce->portBread[i];
		chips[i].portCread = intfce->portCread[i];
		chips[i].portAwrite = intfce->portAwrite[i];
		chips[i].portBwrite = intfce->portBwrite[i];
		chips[i].portCwrite = intfce->portCwrite[i];
		chips[i].obfa_write = NULL;
		chips[i].intra_write = NULL;
		chips[i].ibfa_write = NULL;

		set_mode(i, 0x1b, 0);	/* Mode 0, all ports set to input */
	}
}


int ppi8255_r(int which, int offset)
{
	ppi8255 *chip = &chips[which];

	/* some bounds checking */
	if (which > num)
	{
		logerror("Attempting to access an unmapped 8255 chip.  PC: %04X\n", activecpu_get_pc());
		return 0xff;
	}

	if (offset > 3)
	{
		logerror("Attempting to access an invalid 8255 register.  PC: %04X\n", activecpu_get_pc());
		return 0xff;
	}

  	switch(offset)
  	{
  	case 0: /* Port A read */
  	{
		switch (chip->groupA_mode)
		{
			case 0:
			{
				if (chip->io[0] == 0)
					return chip->latch[0];	/* output */
  				else
					if (chip->portAread)  return (*chip->portAread)(0);	/* input */
  			}
  			break;
  
			case 1:
  			{
  			}
			break;


			case 2:
			{
				int data;

				data = chip->latch[0];

				/* input buffer is now empty */
				ppi8255_ibfa_w(which, 0);

				ppi8255_set_intra(which);

				logerror("8255 chip %d: Read latched data from port A %02x\n",which, data);

				/* return latched data */
				return data;
			}
			break;

			default:
				break;

		}
	}
	break;

	case 1: /* Port B read */
		if (chip->io[1] == 0)
			return chip->latch[1];	/* output */
		else
			if (chip->portBread)  return (*chip->portBread)(0);	/* input */
  		break;
  
  	case 2: /* Port C read */
  	{
		switch (chip->groupA_mode)
		{
			case 0:
			{
				int data;

				data = chip->latch[2] & (~chip->io[2]);

  				if (chip->portCread)
				{
					data |= chip->portCread(0) & chip->io[2];
				}

				return data;
  			}
  			break;
  
			case 2:
			{
				/* return data - combination of input and latched output depending on
				   the input/output status of each half of port C */
				int data;

				data =  (
							/* keep state of obf, ibf, intra */
							(chip->latch[2] & ((1<<7)|(1<<5)|(1<<3))) |
							/* inte1 and inte2 flags */
							(chip->inte_flags & ((1<<6)|(1<<4))) |
							/* state of bits 2..0 depending on input/output status */
							((chip->latch[2] & (~chip->io[2])) & 0x07)
						);
				
				if (chip->portCread)
				{
					data  |= chip->portCread(0) & chip->io[2] & 0x07;
				}

				logerror("port c data: %02x\n",data);

				return data;
			}
			break;

			case 1:
				break;

			default:
				break;
		}
	}
  	break;

	case 3: /* Control word */
		return 0xff;
  	}
  
	logerror("8255 chip %d: Port %c is being read but has no handler.  PC: %04X\n", which, 'A' + offset, activecpu_get_pc());

	return 0xff;
}



#define PPI8255_PORT_A_WRITE()							\
{														\
	int write_data;										\
														\
	write_data = (chip->latch[0] & ~chip->io[0]) |		\
				 (0xff & chip->io[0]);					\
														\
	if (chip->portAwrite)								\
		(*chip->portAwrite)(0, write_data);				\
	else												\
		logerror("8255 chip %d: Port A is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data);	\
}

#define PPI8255_PORT_B_WRITE()							\
{														\
	int write_data;										\
														\
	write_data = (chip->latch[1] & ~chip->io[1]) |		\
				 (0xff & chip->io[1]);					\
														\
	if (chip->portBwrite)								\
		(*chip->portBwrite)(0, write_data);				\
	else												\
		logerror("8255 chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data);	\
}

#define PPI8255_PORT_C_WRITE()							\
{														\
	int write_data;										\
														\
	write_data = (chip->latch[2] & ~chip->io[2]) |		\
				 (0xff & chip->io[2]);					\
														\
	if (chip->portCwrite)								\
		(*chip->portCwrite)(0, write_data);				\
	else												\
		logerror("8255 chip %d: Port C is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data);	\
}


void ppi8255_w(int which, int offset, int data)
{
	ppi8255	*chip;


	/* Some bounds checking */
	if (which > num)
	{
		logerror("Attempting to access an unmapped 8255 chip.  PC: %04X\n", activecpu_get_pc());
		return;
	}

	chip = &chips[which];


	if (offset > 3)
	{
		logerror("Attempting to access an invalid 8255 register.  PC: %04X\n", activecpu_get_pc());
		return;
	}


  	switch( offset )
  	{
  	case 0: /* Port A write */
	{
  		chip->latch[0] = data;

		
		switch (chip->groupA_mode)
		{
			default:
			case 0:
			{
  				PPI8255_PORT_A_WRITE();
			}
			break;

			case 1:
			{
				logerror("mode not supported by emulation\n");

			}
			break;


			case 2:
			{
				/* mode 2 */
				logerror("8255 chip %d: Write to port A latch %02x\n",which, chip->latch[0]);

				if (chip->portAwrite)
					chip->portAwrite(0,chip->latch[0]);

				ppi8255_obfa_w(which, 0);

				ppi8255_set_intra(which);

			}
			break;
		}
	}
  	break;

		case 1: /* Port B write */
			chip->latch[1] = data;
			PPI8255_PORT_B_WRITE();
			break;

  	case 2: /* Port C write */
	{
		switch (chip->groupA_mode)
		{
			case 0:
			{
  				chip->latch[2] = data;
  				PPI8255_PORT_C_WRITE();
			}
			break;

			case 1:
			{


			}
			break;

			case 2:
			{
				/* not possible to set flag states with write */
				chip->latch[2] = (data & 0x07) | chip->latch[2];
				PPI8255_PORT_C_WRITE();
			}
			break;

			default:
				break;
		}
	}
  	break;

		case 3: /* Control word */
			if (data & 0x80)
			{
				set_mode(which, data & 0x7f, 1);
			}
			else
			{
  			/* bit set/reset */
  			int bit;
			int newData;
  
  			bit = (data >> 1) & 0x07;
  			
  			if (data & 1)
			{
				newData = chip->latch[2] | (1<<bit);
			}
  			else
			{
				newData = chip->latch[2] & (~(1<<bit));
			}
  
			switch (chip->groupA_mode)
			{
				case 0:
				{
				}
				break;

				case 1:
				{
				}
				break;

				case 2:
				{

					/* set inte flags from bits 6 and 4 */
					chip->inte_flags = newData & ((1<<6)|(1<<4));

					/* bit set, reset can be used to set the signal of any output bit obf, ibf, intra */
					/* states of inputs remain */
					newData = 
						(
						/* keep state of ack and stb */
						(chip->latch[2] & ((1<<6)|(1<<4))) |
						/* allow changing of output flags and bits 2..0 */
						(newData & ((1<<7)|(1<<5)|(1<<3)|0x07))
						);

					/* enable intra output */
					ppi8255_set_intra(which);

				}
				break;
			
				default:
					break;
			}

			chip->latch[2] = newData;
  		}
	}
}

#ifdef MESS
data8_t ppi8255_peek( int which, offs_t offset )
{
	ppi8255 *chip;


	/* Some bounds checking */
	if (which > num)
	{
		logerror("Attempting to access an unmapped 8255 chip.  PC: %04X\n", activecpu_get_pc());
		return 0xff;
	}

	chip = &chips[which];


	if (offset > 2)
	{
		logerror("Attempting to access an invalid 8255 port.  PC: %04X\n", activecpu_get_pc());
		return 0xff;
	}


	chip = &chips[which];

	return chip->latch[offset];
}
#endif


void ppi8255_set_portAread(int which, read8_handler portAread)
{
	chips[which].portAread = portAread;
}

void ppi8255_set_portBread(int which, read8_handler portBread)
{
	chips[which].portBread = portBread;
}

void ppi8255_set_portCread(int which, read8_handler portCread)
{
	chips[which].portCread = portCread;
}


void ppi8255_set_portAwrite(int which, write8_handler portAwrite)
{
	chips[which].portAwrite = portAwrite;
}

void ppi8255_set_portBwrite(int which, write8_handler portBwrite)
{
	chips[which].portBwrite = portBwrite;
}

void ppi8255_set_portCwrite(int which, write8_handler portCwrite)
{
	chips[which].portCwrite = portCwrite;
}


static void set_mode(int which, int data, int call_handlers)
{
	ppi8255 *chip = &chips[which];

	chip->groupA_mode = (data >> 5) & 3;
	chip->groupB_mode = (data >> 2) & 1;

	if (chip->groupA_mode == 3)
		chip->groupA_mode = 2;

        logerror("8255 chip %d: group A mode %d\n",which,chip->groupA_mode);
        logerror("8255 chip %d: group B mode %d\n",which,chip->groupB_mode);

	if (((chip->groupA_mode != 0) && (chip->groupA_mode!=2)) || (chip->groupB_mode != 0))
  	{
  		logerror("8255 chip %d: Setting an unsupported mode %02X.  PC: %04X\n", which, data & 0x62, activecpu_get_pc());
  		return;
  	}
  
	chip->io[0] = 0;
	chip->io[1] = 0;
	chip->io[2] = 0;

  	/* Port A direction */
  	if ( data & 0x10 )
        {
                chip->io[0] = 0xff;             /* input */
                logerror("8255 chip %d: port A input\n",which);
          }
          else
          {
                logerror("8255 chip %d: port A output\n",which);
          }
  
  	/* Port B direction */
  	if ( data & 0x02 )
        {
		chip->io[1] = 0xff;
                logerror("8255 chip %d: port B input\n",which);
         }
           else
         {
                logerror("8255 chip %d: port B output\n",which);
         }
	/* Port C upper direction */
	if ( data & 0x08 )
		chip->io[2] |= 0xf0;
  
  	/* Port C lower direction */
  	if ( data & 0x01 )
		chip->io[2] |= 0x0f;

	/* KT: 25-Dec-99 - 8255 resets latches when mode set */
	chip->latch[0] = chip->latch[1] = chip->latch[2] = 0;

	if (call_handlers)
	{
		if (chip->portAwrite) PPI8255_PORT_A_WRITE();
		if (chip->portBwrite) PPI8255_PORT_B_WRITE();
	
		if (chip->groupA_mode == 0)
		{
  			if (chip->portCwrite)  PPI8255_PORT_C_WRITE();
  		}
		else
		{
			/* all output registers and status flip-flops are reset on mode change or reset */
			chip->inte_flags = 0;

			/* don't know if this is correct or not */

			/* output buffer empty */
			ppi8255_obfa_w(which, 0);
			/* input buffer empty */
			ppi8255_ibfa_w(which, 0);
			
			ppi8255_set_intra(which);
		}

	}
}


/* Helpers */
READ8_HANDLER( ppi8255_0_r ) { return ppi8255_r( 0, offset ); }
READ8_HANDLER( ppi8255_1_r ) { return ppi8255_r( 1, offset ); }
READ8_HANDLER( ppi8255_2_r ) { return ppi8255_r( 2, offset ); }
READ8_HANDLER( ppi8255_3_r ) { return ppi8255_r( 3, offset ); }
READ8_HANDLER( ppi8255_4_r ) { return ppi8255_r( 4, offset ); }
READ8_HANDLER( ppi8255_5_r ) { return ppi8255_r( 5, offset ); }
READ8_HANDLER( ppi8255_6_r ) { return ppi8255_r( 6, offset ); }
READ8_HANDLER( ppi8255_7_r ) { return ppi8255_r( 7, offset ); }
WRITE8_HANDLER( ppi8255_0_w ) { ppi8255_w( 0, offset, data ); }
WRITE8_HANDLER( ppi8255_1_w ) { ppi8255_w( 1, offset, data ); }
WRITE8_HANDLER( ppi8255_2_w ) { ppi8255_w( 2, offset, data ); }
WRITE8_HANDLER( ppi8255_3_w ) { ppi8255_w( 3, offset, data ); }
WRITE8_HANDLER( ppi8255_4_w ) { ppi8255_w( 4, offset, data ); }
WRITE8_HANDLER( ppi8255_5_w ) { ppi8255_w( 5, offset, data ); }
WRITE8_HANDLER( ppi8255_6_w ) { ppi8255_w( 6, offset, data ); }
WRITE8_HANDLER( ppi8255_7_w ) { ppi8255_w( 7, offset, data ); }

/*************************/

void ppi8255_set_input_acka(int which, int data)
{
	ppi8255 *chip = &chips[which];

	if (chip->groupA_mode==2)
	{
		logerror("8255 ppi input acka: %d\n",data);

		if (data)
			chip->latch[2] |= (1<<6);
		else
			chip->latch[2] &= ~(1<<6);

		/* low on pin latches out data to port A */
		if (data==0)
		{
			/* output buffer is now empty */
			ppi8255_obfa_w(which, 1);


	//		/* set output latch to high impedance */
	//		chip->latch[0] = 0x0ff;
		}

		ppi8255_set_intra(which);

	}
}

/* a low on the strobe input (/stb)  loads data into the input latch */
void ppi8255_set_input_stba(int which, int data)
{
	ppi8255 *chip = &chips[which];

	if (chip->groupA_mode==2)
	{
		logerror("8255 ppi input stba: %d\n",data);

		if (data)
			chip->latch[2] |= (1<<4);
		else
			chip->latch[2] &= ~(1<<4);

		if (data==0)
		{
			/* latch data from port A */
			if (chip->portAread)  chip->latch[0] = chip->portAread(0);

			logerror("8255 chip %d: Received /STBA, just latched data into port A %02x\n",which, chip->latch[0]);

			/* input buffer is full */
			ppi8255_ibfa_w(which, 1);

			/* "high" indicates data has been loaded into the input latch */

		}
		
		ppi8255_set_intra(which);
	}
}

/* high indicates interrupt */
static void	ppi8255_set_intra(int which)
{
	ppi8255 *chip = &chips[which];

	int state;

	state = 0;

	if (
		/* /stb = 1, ibf = 1, inte = 1 */
		/* ack = 1, /obf = 1, inte = 1 */
		(
			((chip->latch[2] & ((1<<5)|(1<<4)))==((1<<5)|(1<<4))) &&
			((chip->inte_flags & (1<<4))==(1<<4))
		) ||
		(
			((chip->latch[2] & ((1<<7)|(1<<6)))==((1<<7)|(1<<6))) &&
			((chip->inte_flags & (1<<6))==(1<<6))
		)
		)	
	{
		state = 1;
	}

	if (state)
		chip->latch[2]|=(1<<3);
	else
		chip->latch[2]&=~(1<<3);

	if (chip->intra_write)
	{
		logerror("8255 ppi output intra: %d\n",state);
		chip->intra_write(0,state);
	}
}


void ppi8255_set_mode2_interface( ppi8255_mode2_interface *intfce)
{
	int i;

	for (i = 0; i < num; i++)
	{
		chips[i].obfa_write = intfce->obfa_write[i];
		chips[i].intra_write = intfce->intra_write[i];
		chips[i].ibfa_write = intfce->ibfa_write[i];
	}
}
