#include "driver.h"
#include "adpcm.h"
#include "Z80.h"
#include "sndhrdw/generic.h"

unsigned char RegsYM[0x100];
static int OutValue;
static int ActualRegYM;

static int inter=0;
static int int_enabled=0; /* interrupts off */

static int transmit=0; /* number of bytes to transmit/receive */
static int tr_mode;    /* transmit mode (1 or 2 bytes) */
static int lasthalf=0; /* in 2 bytes mode this is first nibble (LSB bits 0,1,2,3) received */

static int m_transmit=0; /* as above */
static int m_tr_mode;    /* as above */
static int m_lasthalf=0; /* as above */
static int soundcommand;

//static int tab_p=0;
//static int tabinit[20]={0x01,0x01,0x00,0xef,0x29,0x25,0x1a,0xff};

/***********************************************************************/
/*  looking from sound board point of view ...                         */
/***********************************************************************/

extern int signalf;
const int SmpOffsTab[] = { 0, 0x0200, 0x0700, 0x2800, 0x6300, 0xb100, 0xc700, -1 };

int rastan_sh_init(const char *gamename)
{
        int i;
	struct GameSamples *samples;

	i = 0;
	while (SmpOffsTab[i+1] != -1) i++;

	if ((samples = malloc(sizeof(struct GameSamples) + (i-1)*sizeof(struct GameSample))) == 0)
		return 0;

	samples->total = i;

	for (i = 0; i < samples->total; i++)
		samples->sample[i] = 0;

	for (i = 0; i < samples->total; i++)
	{
                long smplen;

		if ((smplen=(SmpOffsTab[i+1]-SmpOffsTab[i])) > 0)
		{
		   if ((samples->sample[i] = malloc(sizeof(struct GameSample) + (smplen)*sizeof(char))) != 0)
		   {
                           long j;

			   samples->sample[i]->length = smplen;
			   samples->sample[i]->volume = 255;
			   samples->sample[i]->smpfreq = 8000;   /* standard ADPCM voice freq */
			   samples->sample[i]->resolution = 8;

                           InitDecoder();
                           for (j=0; j < smplen; j++)
                           {
                             signalf += ((j%2) ? DecodeAdpcm(Machine->memory_region[3][(SmpOffsTab[i]+j) >> 2] & 0x0f)
                                               : DecodeAdpcm(Machine->memory_region[3][(SmpOffsTab[i]+j) >> 2] >> 4));
                             if (signalf > 2047) signalf = 2047;
                             if (signalf < -2047) signalf = -2047;
                             samples->sample[i]->data[j] = (signalf / 16);     /* for 16-bit samples multiply by 16 */
                           }
		   }
		}
	}

        Machine->samples = samples;

        return 0;
}

void run_sample(int i)
{
	osd_play_sample(7,Machine->samples->sample[i]->data,
	Machine->samples->sample[i]->length,
	Machine->samples->sample[i]->smpfreq,
	Machine->samples->sample[i]->volume,0);
}

void r_wr_b000(int offset, int data)
{
	if (Machine->samples == 0) return;

        switch(data){
        case 0x00:  run_sample(0);
                    break;
        case 0x02:  run_sample(1);
                    break;
        case 0x07:  run_sample(3);
                    break;
        case 0x28:  run_sample(4);
                    break;
        case 0x63:  run_sample(5);
                    break;
        case 0xb1:  run_sample(6);
                    break;
        default:
           if (errorlog) fprintf(errorlog,"Unknown sample %02x",data);
        }
}

void r_wr_c000(int offset, int data)
{
}
void r_wr_d000(int offset, int data)
{
	if (Machine->samples == 0) return;
	if (data==0)
		osd_stop_sample(7);
}

int rastan_s_interrupt(void)
{
  inter = (inter+1) & 1;
  if ( (int_enabled !=0) && (transmit==0) )
  {
	if (inter==0)
	{

#if 0
          if (tabinit[tab_p]!=0xff)
          {
	      //if (errorlog) fprintf(errorlog,"NMI!\n");
		return Z80_NMI_INT;
          }
#endif
          if (pending_commands>0)
          {
                soundcommand = sound_command_r(0);
    if (errorlog) fprintf(errorlog,"scomm = %02x\n",soundcommand);
		return Z80_NMI_INT;
          }
          else
          {
            return Z80_IGNORE_INT;
          }
	}
	else
	{
		//if (errorlog) fprintf(errorlog,"INT!\n");
		return 0;
	}
  }
  else
   return Z80_IGNORE_INT;
}


static unsigned char YMstatus=0;
//static int YMport=0;

int rYMport(int offset)
{
        YMstatus ^= 255;
	return YMstatus;
}

int rYMdata(int offset)
{
        YMstatus ^= 255;
	return YMstatus;
}

void wYMport(int offset, int data)
{
	ActualRegYM = data;
}

void wYMdata(int offset,int data)
{
static int ch=0;
static int lastkckf=0;
int pomkckf;

  OutValue = data;
  RegsYM[ActualRegYM]=data;


pomkckf = (RegsYM[0x28+ch]*256) + RegsYM[0x30+ch];

if (errorlog)
{
    if ( pomkckf != lastkckf )
	fprintf(errorlog,"KCKF=%04x MUL1=%02x MUL2=%02x MUL3=%02x MUL4=%02x\n",pomkckf,RegsYM[0x40+ch],RegsYM[0x48+ch],RegsYM[0x50+ch],RegsYM[0x58+ch]);
    lastkckf = pomkckf;
}

//#if 0
if (errorlog)
  if (ActualRegYM != 0x1b)
    if ( (ActualRegYM>=0x20) && (ActualRegYM<=0x27) )
	fprintf(errorlog,"R%02x=%02x\n",ActualRegYM,data);
//#endif

//  OUTPRO_C();
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
    if (tr_mode==1)
    {
       pom=0x00;
    }
    else
    {            /*2-bytes transmision*/
      if ( (regs.PC.D >= 0x66) && (regs.PC.D <= 0x86) )
      {
	        if (transmit==2)
        	{
	          //pom=0x0f;   /*lower nibble*/
        	  pom = soundcommand & 0x0f ;
		if (errorlog)
			fprintf(errorlog,"pom1=%02x\n",pom);
	        }
        	else
	        {
        	  pom = (soundcommand & 0xf0) >> 4;
	          //pom=0x0e;  /*higher nibble*/
        	if (errorlog)
			fprintf(errorlog,"pom2=%02x\n",pom);

	        }
#if 0
        if (tabinit[tab_p] != 0xff )
        {
	        if (transmit==2)
        	{
	          //pom=0x0f;   /*lower nibble*/
        	  pom = (tabinit[tab_p] & 0x0f );
	        }
        	else
	        {
        	  pom=( (tabinit[tab_p] & 0xf0) >> 4);
	          //pom=0x0e;  /*higher nibble*/

        	  tab_p++;
	        }
	}
#endif
      }
    }

#if 0
  if (tr_mode==2)
    if (errorlog) fprintf(errorlog,"commread pom=%02x, PC=%04x \n",pom,regs.PC.D);
#endif
    transmit--;
  }
	return pom;
}


void r_wr_a000(int offset,int data)
{
int pom;

if (transmit!=0)
  if (errorlog) fprintf(errorlog,"Mode changed while expecting to transmit !\n");

pom = (data & 0x04) >> 2;
transmit = 1 + (1- pom); /* one or two bytes long transmission */
lasthalf=0;
tr_mode=transmit;

pom = (data & 0x03);
if (pom==0x01)
  int_enabled = 0; /* off */
if (pom==0x02)
  int_enabled = 1; /* on */
if (pom==0x03)
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
/*
 if (errorlog)
	fprintf(errorlog,"P%02x pc%08x\n",data,cpu_getpc());
*/

if (m_transmit!=0)
  if (errorlog) fprintf(errorlog,"Main mode changed while expecting to transmit !\n");

pom = (data & 0x04) >> 2;
m_transmit = 1 + (1- pom); /* one or two bytes long transmission */
m_lasthalf = 0;
m_tr_mode = m_transmit;

}

void rastan_sound_comm_w(int offset,int data)
{
  data=data&0x0f;

  if (m_transmit == 0)
  {
    if (errorlog) fprintf(errorlog,"unexpected transmission!\n");
  }
  else
  {

   if (m_transmit==2)
     m_lasthalf = data;

    m_transmit--;

    if (m_transmit==0)
    {
      if (m_tr_mode==2)
      {
	sound_command_w(0,m_lasthalf+(data<<4) );
       	if (errorlog) fprintf(errorlog,"double %02x!\n",m_lasthalf+(data<<4) );
      }
      else
      {
	sound_command_w(0,data);
       	if (errorlog) fprintf(errorlog,"single %02x!\n",data );
      }
    }

  }

}



int rastan_sound_port_r(int offset)
{
static unsigned char wyn[4]={0,0,0,0};

  if (errorlog)
	fprintf(errorlog,"Dr\n");
  return wyn[0];
}

int rastan_sound_comm_r(int offset)
{
static unsigned char wyn[4]={0,0,0,0};

wyn[0] ^= 0xff;

  return wyn[0];
}
