#include "driver.h"
#include "Z80/Z80.h"


/*#define REPORT_SLAVE_MODE_CHANGE      */
/*#define REPORT_SLAVE_MODE_READ_ITSELF */
/*#define REPORT_MAIN_MODE_READ_SLAVE */
/*#define REPORT_DATA_FLOW */


static int nmi_enabled=0; /* interrupts off */

static int SlaveContrStat; /*last control byte written to controller by soundboard*/
static int MainContrStat;  /*last control byte written to controller by motherboard*/

static int transmit=0; /* number of bytes to transmit/receive */
static int tr_mode;    /* transmit mode (1 or 2 bytes) */
static int lasthalf=0; /* in 2 bytes mode this is first nibble (LSB bits 0,1,2,3) received */

static int m_transmit=0; /* as above but for motherboard*/
static int m_tr_mode;    /* as above */
static int m_lasthalf=0; /* as above */

static int NMI_req=0; /*no request*/
static int IRQ_req=0; /*no request*/

static int soundcommand;
static int soundboarddata;

/***********************************************************************/
/*  looking from sound board point of view ...                         */
/***********************************************************************/

void r_wr_c000(int offset, int data)
/* Meaning of this address is unknown !!! */
{
}

void r_wr_d000(int offset, int data)
/* ADPCM chip used in Rastan does not stop playing the sample by itself
** it must be said to stop instead. This is the address that does it.
*/
{
	if (Machine->samples == 0) return;
#if 0
	if (data==0)
		osd_stop_sample(channel);
#endif
}

void Interrupt_Controller(void)
{
	if (transmit==0)
	{
		if (IRQ_req)
		{
			cpu_cause_interrupt(1,0);
			IRQ_req = 0;
		}
		else
		{
			if (NMI_req==1)
			{
				cpu_cause_interrupt (1, Z80_NMI_INT);
				NMI_req = 2;
			}
		}
	}
}


int r_rd_a001(int offset)
{

	static unsigned char pom=0;

	if (transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"Slave unexpected receiving!\n");
	}
	else
	{
		if (tr_mode == 1)
		{
			pom = SlaveContrStat;
#ifdef REPORT_SLAVE_MODE_READ_ITSELF
			if (errorlog) fprintf(errorlog,"Slave has read status of itself %02x\n",pom);
#endif
		}
		else
		{            /*2-bytes transmision*/
			if (transmit==2)
			{
				pom = soundcommand & 0x0f;
#ifdef REPORT_DATA_FLOW
				if (errorlog) fprintf(errorlog,"Slave has read pom1=%02x\n",pom);
#endif
			}
			else
			{
				pom = (soundcommand & 0xf0) >> 4;
#ifdef REPORT_DATA_FLOW
				if (errorlog) fprintf(errorlog,"Slave has read pom2=%02x\n",pom);
#endif
			}
		}
		transmit--;
	}

	Interrupt_Controller();

	return pom;
}


void r_wr_a000(int offset,int data)
{
	int pom;

	if (transmit != 0)
		if (errorlog) fprintf(errorlog,"Slave mode changed while expecting to transmit !\n");

#ifdef REPORT_SLAVE_MODE_CHANGE
	if (errorlog) fprintf(errorlog,"Slave changing its mode to %02x\n",data);
#endif

	SlaveContrStat = data;
	pom = (data >> 2 ) & 0x01;
	transmit = 1 + (1 - pom); /* one or two bytes long transmission */
	lasthalf = 0;
	tr_mode = transmit;

	pom = data & 0x03;
	if (pom == 0x01)
		nmi_enabled = 0; /* off */

	if (pom == 0x02)
	{
		nmi_enabled = 1; /* on */
		if (NMI_req==2)
			NMI_req=0;
	}

	if (pom == 0x03)
		if (errorlog) fprintf(errorlog,"Int mode = 3!\n");
}


void r_wr_a001(int offset,int data)
{
	data &= 0x0f;

	if (transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"Slave unexpected transmission!\n");
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
#ifdef REPORT_DATA_FLOW
				if (errorlog) fprintf(errorlog,"Slave sent double %02x\n",lasthalf+(data<<4) );
#endif
			}
			else
			{
				/*soundboarddata = data;*/
				/*if (errorlog) fprintf(errorlog,"What now? (Slave sent single %02x)\n",data );*/
			}
		}
	}

	Interrupt_Controller();
}

void rastan_irq_handler (void)
{
	IRQ_req = 1;
/*	Interrupt_Controller();*/
}

/***********************************************************************/
/*  now looking from main board point of view                          */
/***********************************************************************/

void rastan_sound_port_w(int offset,int data)
{
	int pom;

	if (m_transmit != 0)
		if (errorlog) fprintf(errorlog,"Main mode changed while expecting to transmit !\n");

	MainContrStat = data;
	pom = (data >> 2 ) & 0x01;
	m_transmit = 1 + (1 - pom); /* one or two bytes long transmission */
	m_lasthalf = 0;
	m_tr_mode = m_transmit;
}


void rastan_delayed_callback (int param)
{
	if (NMI_req==0)
	{
		soundcommand = param;
		NMI_req = 1 ;
/*		Interrupt_Controller();*/
	}
	else
	{
		if (errorlog) fprintf(errorlog,"Command write skipped because last command hasn't been read!\n");
	}
}


void rastan_sound_comm_w(int offset,int data)
{
	data &= 0x0f;

	if (m_transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"Main unexpected transmission!\n");
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
#ifdef REPORT_DATA_FLOW
				if (errorlog) fprintf(errorlog,"Main sent double %02x\n",m_lasthalf+(data<<4) );
#endif
			}
			else
			{
				/*timer_set (TIME_NOW, data, rastan_delayed_callback);*/
				/*if (errorlog) fprintf(errorlog,"What now? (Main sent single %02x)\n",data );*/
			}
		}
	}
}



int rastan_sound_comm_r(int offset)
{

	m_transmit--;
	if (m_tr_mode==2)
	{
#ifdef REPORT_DATA_FLOW
		if (errorlog) fprintf(errorlog,"Main read double %02x\n",soundboarddata );
#endif
		return soundboarddata; /*wyn;*/
	}
	else
	{
#ifdef REPORT_MAIN_MODE_READ_SLAVE
		if (errorlog) fprintf(errorlog,"Main read status of Slave %02x\n",SlaveContrStat );
#endif
		return SlaveContrStat;
	}
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





#if 0
/* --------------------------- CUT HERE -------------------------------*/
/*               THIS IS HERE JUST TO TEST YM2151 EMULATOR             */

static int tab_p=0;
static int tabinit[]={0xef, //0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
			// 0x10,0x11,0x12, 0x13,0x25, 0x14, 0x15,
			//0x16, 0x17, 0x18,
			//0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x21,0x22,
			//0x23,0x25, 0x24,0x25, 0x26,0x25,
			//0x27,0x25, 0x28,0x25, 0x29,0x25,

			0x1a,

			0xff};
static int delay=0;

/* Rastan sound commands :
04-insert coin
05-titditiditatidddadibuuuhuuuuuu
06-heart beat (ADPCM)
07-knight (in the castle) hitten in his metal cloths
08-stone hitten by Rastan's sword, etc.
09-Rastan attacking with his sword
0a-Rastan jumping (hooo!) (ADPCM)
0b-Rastan landing on the ground (after jumping)
0c-Rastan hitten by an alien or anything
0d-Rastan dying (aaarrrrgghhhhh!) (ADPCM)
0e-Rastan picking up a weapon
0f-Rastan picking up a "diamond"
10-Rastan killing an alien
11-Lion-snake shooting fire-balls
12-water falling (in the castle)
13-fire in the castle (continous effect that needs stop sound command) (0x25)
14-big flying enemy
15-flying bats
16-falling stone hitting the earth surface
17-Rastan crushing the stone block (at the beginning of the 1st stage)
18-"Vertical door" closing (in the castle)

19-entering music (before the 1st stage start in Rastan Saga)
1a-first stage music (with intro)
1b-second stage music
1c-enemy music (before the end of every second stage)
1d-enemy music (before the end of every second stage) (same as 0x1c)

1e-some screamings (ADPCM)

1f-nothing (some bad music)
20-as 0x1f (some bad music)
21-thunder storm (ADPCM)
22-Rastan shooting his fire-sword
23-some noises (probably the dragon dying at the end of game)
24-nothing

25-sound stop sommand

26-explosion (ADPCM)
27-nothing (some BAD MUSIC but the GAME USES THIS COMMAND at the end of every second stage)
28-first stage music (without intro)

29-stop insert coin sound effect

ef-enable sound board
*/


void rastan_irq_mus_handler (void)
{
	cpu_cause_interrupt(0,0);
}

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

void r_wr_a000mus(int offset,int data)
{
	int pom;

	if (transmit != 0)
		if (errorlog) fprintf(errorlog,"Mode changed while expecting to transmit !\n");

	pom = (data >> 2 ) & 0x01;
	transmit = 1 + (1 - pom); /* one or two bytes long transmission */
	lasthalf = 0;
	tr_mode = transmit;

	pom = (data & 0x03);
	if (pom == 0x01)
		nmi_enabled = 0; /* off */
	if (pom == 0x02)
		nmi_enabled = 1; /* on */
}


void r_wr_a001mus(int offset,int data)
{
	int pom;

	if (transmit == 0)
	{
		if (errorlog) fprintf(errorlog,"unexpected transmission!\n");
	}
	else
	{
		pom = data;
		transmit--;
	}
}


int rastan_smus_interrupt(void)
{
  if ( (nmi_enabled !=0) && (transmit==0) )
  {
	if (tabinit[tab_p]!=0xff)
	{
		delay--;
		if (delay<0){
			delay=0;
			if (osd_key_pressed(OSD_KEY_ENTER)){
				if (errorlog) fprintf(errorlog,"Start sound %02x\n",tabinit[tab_p]);
				delay=10;
				return Z80_NMI_INT;
			}
		}
	}
  }
  return Z80_IGNORE_INT;
}

#endif
