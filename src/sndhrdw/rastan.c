#include "driver.h"
#include "Z80/Z80.h"

static int int_enabled=0; /* interrupts off */

static int transmit=0; /* number of bytes to transmit/receive */
static int tr_mode;    /* transmit mode (1 or 2 bytes) */
static int lasthalf=0; /* in 2 bytes mode this is first nibble (LSB bits 0,1,2,3) received */

static int m_transmit=0; /* as above */
static int m_tr_mode;    /* as above */
static int m_lasthalf=0; /* as above */

static int soundcommand;

/***********************************************************************/
/*  looking from sound board point of view ...                         */
/***********************************************************************/

void r_wr_c000(int offset, int data)
{
}

void r_wr_d000(int offset, int data)
{
	if (Machine->samples == 0) return;
#if 0
	if (data==0)
		osd_stop_sample(channel);
#endif
}


int r_rd_a000(int offset)
{
	return 0;
}


int r_rd_a001(int offset)
{
	Z80_Regs regs;

	static unsigned char pom=0;

	Z80_GetRegs(&regs);

	if (transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"unexpected receiving!\n");
	}
	else
	{
		if (tr_mode == 1)
		{
			pom = 0x00;
		}
		else
		{            /*2-bytes transmision*/
			if ((regs.PC.D >= 0x66) && (regs.PC.D <= 0x86))
			{
				if (transmit==2)
				{
					pom = soundcommand & 0x0f;
					if (errorlog) fprintf(errorlog,"pom1=%02x\n",pom);
				}
				else
				{
					pom = (soundcommand & 0xf0) >> 4;
					if (errorlog) fprintf(errorlog,"pom2=%02x\n",pom);
				}

			}
		}

		transmit--;
	}
	return pom;
}


void r_wr_a000(int offset,int data)
{
	int pom;

	if (transmit != 0)
		if (errorlog) fprintf(errorlog,"Mode changed while expecting to transmit !\n");

	pom = (data & 0x04) >> 2;
	transmit = 1 + (1 - pom); /* one or two bytes long transmission */
	lasthalf = 0;
	tr_mode = transmit;

	pom = (data & 0x03);
	if (pom == 0x01)
		int_enabled = 0; /* off */
	if (pom == 0x02)
		int_enabled = 1; /* on */
	if (pom == 0x03)
		if (errorlog) fprintf(errorlog,"Int mode = 3!\n");
}


void r_wr_a001(int offset,int data)
{
	int pom;

	if (transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"unexpected transmission!\n");
	}
	else
	{
		pom = data;
		//if (errorlog) fprintf(errorlog,"wrcomm pom=%02x\n",pom);
		transmit--;
	}
}



/***********************************************************************/
/*  now looking from main board point of view                          */
/***********************************************************************/

void rastan_sound_port_w(int offset,int data)
{
  int pom;


	//if (errorlog) fprintf(errorlog,"P%02x pc%08x\n",data,cpu_getpc());


	if (m_transmit != 0)
		if (errorlog) fprintf(errorlog,"Main mode changed while expecting to transmit !\n");

	pom = (data & 0x04) >> 2;
	m_transmit = 1 + (1 - pom); /* one or two bytes long transmission */
	m_lasthalf = 0;
	m_tr_mode = m_transmit;
}


void rastan_delayed_callback (int param)
{
	soundcommand = param;
	cpu_cause_interrupt (1, Z80_NMI_INT);
}


void rastan_sound_comm_w(int offset,int data)
{
	data &= 0x0f;

	if (m_transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"unexpected transmission!\n");
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
				timer_set (TIME_NOW, m_lasthalf + (data << 4), rastan_delayed_callback);
				if (errorlog) fprintf(errorlog,"double %02x!\n",m_lasthalf+(data<<4) );
			}
			else
			{
				timer_set (TIME_NOW, data, rastan_delayed_callback);
				if (errorlog) fprintf(errorlog,"single %02x!\n",data );
			}
		}
	}
}



int rastan_sound_comm_r(int offset)
{
	static int wyn=0;

	wyn ^= 0xff;
	m_transmit--;
	return wyn;
}


void rastan_sound_w(int offset,int data)
{
	if (offset == 0)
		rastan_sound_port_w(0,data & 0xff);
	else if (offset == 2)
		rastan_sound_comm_w(0,data & 0xff);
}

int rastan_sound_r(int offset)
{
	if (offset == 2)
		return rastan_sound_comm_r(0);
	else return 0;
}















/* --------------------------- CUT HERE -------------------------------*/
/*               THIS IS HERE JUST TO TEST YM2151 EMULATOR             */
#if 0
static int tab_p=0;
static int tabinit[20]={0xef,0x1b,0xff};

int r_rd_a001mus(int offset)
{
	Z80_Regs regs;

	static unsigned char pom=0;

	Z80_GetRegs(&regs);

	if (transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"unexpected receiving!\n");
	}
	else
	{
		if (tr_mode == 1)
		{
			pom = 0x00;
		}
		else
		{            /*2-bytes transmision*/
			if ((regs.PC.D >= 0x66) && (regs.PC.D <= 0x86))
			{
				if (tabinit[tab_p] != 0xff )
				{
					if (transmit==2)
					{
						pom = (tabinit[tab_p] & 0x0f );
					}
					else
					{
						pom=( (tabinit[tab_p] & 0xf0) >> 4);
						tab_p++;
					}
				}
			}
		}

		transmit--;
	}
	return pom;
}


int rastan_smus_interrupt(void)
{

  if ( (int_enabled !=0) && (transmit==0) )
  {
	if (tabinit[tab_p]!=0xff)
	{
		return Z80_NMI_INT;
	}
  }

  return Z80_IGNORE_INT;

}

#endif
