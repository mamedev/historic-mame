#include "driver.h"
#include "cpu/z80/z80.h"


/**********************************************************************************************
	Soundboard Status bitfield definition:
	 bit meaning
	  0  Set if theres any data pending that the main cpu sent to the slave
	  1  ??? ( Its not being checked both on Rastan and SSI )
	  2  Set if theres any data pending that the slave sent to the main cpu

***********************************************************************************************

	It seems like 1 nibble commands are only for control purposes.
	2 nibble commands are the real messages passed from one board to the other.

**********************************************************************************************/

/* Some logging defines */
#if 0
#define REPORT_SLAVE_MODE_CHANGE
#define REPORT_SLAVE_MODE_READ_ITSELF
#define REPORT_MAIN_MODE_READ_SLAVE
#define REPORT_DATA_FLOW
#endif

static int irq_enabled=0; /* interrupts off */

/* status of soundboard ( reports any commands pending ) */
static unsigned char SlaveContrStat = 0;

static int transmit=0; /* number of bytes to transmit/receive (SLAVE) */
static int tr_mode;    /* transmit mode (1 or 2 bytes) */
static int lasthalf=0; /* in 2 bytes mode this is first nibble (LSB bits 0,1,2,3) received */

static int m_transmit=0; /* as above but for motherboard (MASTER) */
static int m_tr_mode;    /* as above */
static int m_lasthalf=0; /* as above */

static int irq_req=0; /*no request*/

static unsigned char soundcommand;
static unsigned char soundboarddata;


/***********************************************************************/
/*  looking from sound board point of view ... (SLAVE)                 */
/***********************************************************************/

void Interrupt_Controller(void)
{
	if ( irq_req && irq_enabled )
	{
		cpu_cause_interrupt( 1, Z80_NMI_INT ); //maybe we should use set_nmi_line here ?
		irq_req = 0;
	}
}


WRITE_HANDLER( taitosound_slave_port_w )
{
	int pom;

	if (transmit != 0)
		logerror("taitosnd: Slave mode changed while expecting to transmit! (PC = %04x) \n", cpu_get_pc() );

#ifdef REPORT_SLAVE_MODE_CHANGE
	logerror("taitosnd: Slave changing its mode to %02x (PC = %04x) \n",data, cpu_get_pc());
#endif

	pom = (data >> 2 ) & 0x01;
	transmit = 1 + (1 - pom); /* one or two bytes long transmission */
	lasthalf = 0;
	tr_mode = transmit;

	pom = data & 0x03;
	if (pom == 0x01)
		irq_enabled = 0; /* off */

	if (pom == 0x02)
		irq_enabled = 1; /* on */

	if (pom == 0x03)
		logerror("taitosnd: Int mode = 3! (PC = %04x)\n", cpu_get_pc() );
}

READ_HANDLER( taitosound_slave_comm_r )
{
	static unsigned char pom=0;

	if (transmit == 0)
	{
		logerror("taitosnd: Slave unexpected receiving! (PC = %04x)\n", cpu_get_pc() );
	}
	else
	{
		if (tr_mode == 1)
		{
			pom = SlaveContrStat;
#ifdef REPORT_SLAVE_MODE_READ_ITSELF
			logerror("taitosnd: Slave has read status of itself %02x (PC = %04x)\n",pom, cpu_get_pc() );
#endif
		}
		else
		{            /*2-bytes transmision*/
			if (transmit==2)
			{
				pom = soundcommand & 0x0f;
#ifdef REPORT_DATA_FLOW
				logerror("taitosnd: Slave has read pom1=%02x (PC = %04x)\n",pom, cpu_get_pc() );
#endif
			}
			else
			{
				pom = (soundcommand & 0xf0) >> 4;
#ifdef REPORT_DATA_FLOW
				logerror("taitosnd: Slave has read pom2=%02x (PC = %04x)\n",pom,cpu_get_pc() );
#endif
				SlaveContrStat &= 0xfe; /* Ready to receive new commands */;
			}
		}
		transmit--;
	}

	Interrupt_Controller();

	return pom;
}

WRITE_HANDLER( taitosound_slave_comm_w )
{
	data &= 0x0f;

	if (transmit == 0)
	{
		logerror("taitosnd: Slave unexpected transmission! (PC = %04x)\n", cpu_get_pc() );
	}
	else
	{
		if (transmit == 2)
			lasthalf = data;
		transmit--;
		if (transmit==0)
		{
			if (tr_mode == 2)
			{
				soundboarddata = lasthalf + (data << 4);
				SlaveContrStat |= 4; /* report data pending on main */
				cpu_spin(); /* writing should take longer than emulated, so spin */
#ifdef REPORT_DATA_FLOW
				logerror("taitosnd: Slave sent double %02x (PC = %04x)\n",lasthalf+(data<<4), cpu_get_pc() );
#endif
			}
			else
			{
#ifdef REPORT_DATA_FLOW
				logerror("taitosnd: Slave issued control value %02x (PC = %04x)\n",data, cpu_get_pc() );
#endif
			}
		}
	}

	Interrupt_Controller();
}



/***********************************************************************/
/*  now looking from main board point of view (MASTER)                 */
/***********************************************************************/

WRITE_HANDLER( taitosound_port_w )
{
	if ((data&0xff) != 0x01)
	{
		int pom = (data >> 2 ) & 0x01;
		m_transmit = 1 + (1 - pom); /* one or two bytes long transmission */
		m_lasthalf = 0;
		m_tr_mode = m_transmit;
	}
	else
	{
		if (m_transmit == 1)
		{
			/*logerror("taitosnd: single-doubled (first was=%02x)\n",m_lasthalf);*/
		}
		else
		{
			logerror("taitosnd: taitosound_port_w() - unknown innerworking\n");
		}
	}
}

WRITE_HANDLER( taitosound_comm_w )
{
	data &= 0x0f;

	if (m_transmit == 0)
	{
		logerror("taitosnd: Main unexpected transmission! (PC = %08x)\n", cpu_get_pc() );
	}
	else
	{
		if (m_transmit == 2)
			m_lasthalf = data;

		m_transmit--;

		if (m_transmit==0)
		{
			if (m_tr_mode == 2)
			{
				soundcommand = m_lasthalf + (data << 4);
				SlaveContrStat |= 1; /* report data pending for slave */
				irq_req = 1;
#ifdef REPORT_DATA_FLOW
				logerror("taitosnd: Main sent double %02x (PC = %08x) \n",m_lasthalf+(data<<4), cpu_get_pc() );
#endif
			}
			else
			{
#ifdef REPORT_DATA_FLOW
				logerror("taitosnd: Main issued control value %02x (PC = %08x) \n",data, cpu_get_pc() );
#endif
				/* this does a hi-lo transition to reset the sound cpu */
				if (data)
					cpu_set_reset_line(1,ASSERT_LINE);
				else
					cpu_set_reset_line(1,CLEAR_LINE);

				m_transmit++;
			}
		}
	}
}

READ_HANDLER( taitosound_comm_r )
{

	m_transmit--;
	if (m_tr_mode==2)
	{
#ifdef REPORT_DATA_FLOW
		logerror("taitosnd: Main read double %02x (PC = %08x)\n",soundboarddata, cpu_get_pc() );
#endif
		SlaveContrStat &= 0xfb; /* clear pending data for main bit */

		if ( m_transmit == 1 )
			return soundboarddata & 0x0f;

		return ( soundboarddata >> 4 ) & 0x0f;

	}
	else
	{
#ifdef REPORT_MAIN_MODE_READ_SLAVE
		logerror("taitosnd: Main read status of Slave %02x (PC = %08x)\n",SlaveContrStat, cpu_get_pc() );
#endif
		m_transmit++;
		return SlaveContrStat;
	}
}



/* wrapper functions for 16bit handlers */

WRITE16_HANDLER( taitosound_port16_lsb_w )
{
	if (ACCESSING_LSB)
		taitosound_port_w(0,data & 0xff);
}
WRITE16_HANDLER( taitosound_comm16_lsb_w )
{
	if (ACCESSING_LSB)
		taitosound_comm_w(0,data & 0xff);
}
READ16_HANDLER( taitosound_comm16_lsb_r )
{
	return taitosound_comm_r(0);
}


WRITE16_HANDLER( taitosound_port16_msb_w )
{
	if (ACCESSING_MSB)
		taitosound_port_w(0,data >> 8);
}
WRITE16_HANDLER( taitosound_comm16_msb_w )
{
	if (ACCESSING_MSB)
		taitosound_comm_w(0,data >> 8);
}
READ16_HANDLER( taitosound_comm16_msb_r )
{
	return taitosound_comm_r(0) << 8;
}

