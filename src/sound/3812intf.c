#define NEW_OPLEMU 1

/*$DEADSERIOUSCLAN$*********************************************************************
* FILE
*	Yamaha 3812 emulator interface - MAME VERSION
*
* CREATED BY
*	Ernesto Corvi
*
* UPDATE LOG
*	CHS 1999-01-09	Fixes new ym3812 emulation interface.
*	CHS 1998-10-23	Mame streaming sound chip update
*	EC	1998		Created Interface
*
* NOTES
*
***************************************************************************************/
#include "driver.h"
#include "3812intf.h"
#include "fm.h"
#include "ym3812.h"

#define OPL3CONVERTFREQUENCY

static char *chipname;

#if NEW_OPLEMU
static int chiptype;
#else
/* Emulated YM3812 variables and defines */
static ym3812* ym = 0;
static int ym_channel = 0;
static int sample_bits;
static FMSAMPLE *buffer;
#endif

/* Non-Emulated YM3812 variables and defines */
static int pending_register;
static int register_0xbd;
static unsigned char status_register;
static unsigned char timer_register;
static unsigned int timer1_val;
static unsigned int timer2_val;
static double timer_step;
static int aOPLFreqArray[16];		// Up to 9 channels..

/* These ones are used by both */
static const struct YM3812interface *intf;
static void *timer1;
static void *timer2;

/* Function procs to access the selected YM type */
/* static int ( *sh_start )( const struct MachineSound *msound ); */
static void ( *sh_stop )( void );
static int ( *status_port_0_r )( int offset );
static void ( *control_port_0_w )( int offset, int data );
static void ( *write_port_0_w )( int offset, int data );
static int ( *read_port_0_r )( int offset );

/**********************************************************************************************
	Begin of non-emulated YM3812 interface block
 **********************************************************************************************/

void timer1_callback (int param)
{
	if (!(timer_register & 0x40))
	{
		if (intf->handler) (*intf->handler)();

		/* set the IRQ and timer 1 signal bits */
		status_register |= 0x80|0x40;
	}

	/* next! */
	timer1 = timer_set ((double)timer1_val*4*timer_step, 0, timer1_callback);
}

void timer2_callback (int param)
{
	if (!(timer_register & 0x20))
	{
		if (intf->handler) (*intf->handler)();

		/* set the IRQ and timer 2 signal bits */
		status_register |= 0x80|0x20;
	}

	/* next! */
	timer2 = timer_set ((double)timer2_val*16*timer_step, 0, timer2_callback);
}

int nonemu_YM3812_sh_start(const struct MachineSound *msound)
{
	pending_register = -1;
	timer1 = timer2 = 0;
	status_register = 0x80;
	timer_register = 0;
	timer1_val = timer2_val = 256;
	intf = msound->sound_interface;

	timer_step = TIME_IN_HZ((double)intf->baseclock / 72.0);
	return 0;
}

void nonemu_YM3812_sh_stop(void)
{
}

int nonemu_YM3812_status_port_0_r(int offset)
{
	/* mask out the timer 1 and 2 signal bits as requested by the timer register */
	return status_register & ~(timer_register & 0x60);
}

void nonemu_YM3812_control_port_0_w(int offset,int data)
{
	pending_register = data;

	/* pass through all non-timer registers */
#ifdef OPL3CONVERTFREQUENCY
	if ( ((data==0xbd)||((data&0xe0)!=0xa0)) && ((data<2)||(data>4)) )
#else
	if ( ((data<2)||(data>4)) )
#endif
		osd_ym3812_control(data);
}

void nonemu_WriteConvertedFrequency( int nFrq, int nCh )
{
	int		nRealOctave;
	double	vRealFrq;

	vRealFrq = (((nFrq&0x3ff)<<((nFrq&0x7000)>>12))) * (double)intf->baseclock / (double)ym3812_StdClock;
	nRealOctave = 0;

	while( (vRealFrq>1023.0)&&(nRealOctave<7) )
	{
		vRealFrq /= 2.0;
		nRealOctave++;
	}
	osd_ym3812_control(0xa0|nCh);
	osd_ym3812_write(((int)vRealFrq)&0xff);
	osd_ym3812_control(0xb0|nCh);
	osd_ym3812_write( ((((int)vRealFrq)>>8)&3)|(nRealOctave<<2)|((nFrq&0x8000)>>10) );
}

void nonemu_YM3812_write_port_0_w(int offset,int data)
{
//	if (pending_register < 2 || pending_register > 4)
	int nCh = pending_register&0x0f;

	if( pending_register == 0xbd ) register_0xbd = data;
#ifdef OPL3CONVERTFREQUENCY
	if( (nCh<9) )// if( (((~register_0xbd)&0x20)||(nCh<6)) )
	{
		if( (pending_register&0xf0) == 0xa0 )
		{
			aOPLFreqArray[nCh] = (aOPLFreqArray[nCh] & 0xf300)|(data&0xff);
			nonemu_WriteConvertedFrequency( aOPLFreqArray[nCh], nCh );
			return;
		}
		else if( (pending_register&0xf0)==0xb0 )
		{
			aOPLFreqArray[pending_register&0xf] = (aOPLFreqArray[nCh] & 0x00ff)|((data&0x3)<<8)|((data&0x1c)<<10)|((data&0x20)<<10);
			nonemu_WriteConvertedFrequency( aOPLFreqArray[nCh], nCh );
			return;
		}
	}
#endif
	if ( (pending_register<2)||(pending_register>4) ) osd_ym3812_write(data);
	else
	{
		switch (pending_register)
		{
			case 2:
				timer1_val = 256 - data;
				break;

			case 3:
				timer2_val = 256 - data;
				break;

			case 4:
				/* bit 7 means reset the IRQ signal and status bits, and ignore all the other bits */
				if (data & 0x80)
				{
					status_register = 0;
				}
				else
				{
					/* set the new timer register */
					timer_register = data;

					/*  bit 0 starts/stops timer 1 */
					if (data & 0x01)
					{
						if (!timer1)
							timer1 = timer_set ((double)timer1_val*4*timer_step, 0, timer1_callback);
					}
					else if (timer1)
					{
						timer_remove (timer1);
						timer1 = 0;
					}

					/*  bit 1 starts/stops timer 2 */
					if (data & 0x02)
					{
						if (!timer2)
							timer2 = timer_set ((double)timer2_val*16*timer_step, 0, timer2_callback);
					}
					else if (timer2)
					{
						timer_remove (timer2);
						timer2 = 0;
					}

					/* bits 5 & 6 clear and mask the appropriate bit in the status register */
					status_register &= ~(data & 0x60);
				}
				break;
		}
	}
	pending_register = -1;
}

int nonemu_YM3812_read_port_0_r( int offset ) {
	return pending_register;
}

/**********************************************************************************************
	End of non-emulated YM3812 interface block
 **********************************************************************************************/

#if NEW_OPLEMU

#include "sound/fmopl.h"

typedef void (*STREAM_HANDLER)(int param,void *buffer,int length);

static int stream[MAX_3812];
static FM_OPL *F3812[MAX_3812];
static void *Timer[MAX_3812*2];

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if(irq)
	{	/* Eddge sense */
		if(intf->handler) intf->handler();
	}
}

#if 0
static void UpdateHandler(int n, void *buf, int length)
{
	YM3812UpdateOne(F3812[n],buf,length);
}
#endif

/* Timer overflow callback from timer.c */
static void timer_callback_3812(int param)
{
	int n=param>>1;
	int c=param&1;
	Timer[param] = 0;
	OPLTimerOver(F3812[n],c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		if( Timer[c] )
		{
	 		timer_remove (Timer[c]);
			Timer[c] = 0;
		}
	}
	else
	{	/* Start FM Timer */
		Timer[c] = timer_set(period, c, timer_callback_3812 );
	}
}

/************************************************/
/* Sound Hardware Start							*/
/************************************************/
static int emu_YM3812_sh_start(const struct MachineSound *msound)
{
	int i,j;
	int rate = Machine->sample_rate;

	intf = msound->sound_interface;
	if( intf->num > MAX_3812 ) return 1;

	/* Timer state clear */
	memset(Timer,0,sizeof(Timer));

	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		/* stream setup */
		char name[40];
		int vol = intf->mixing_level[i];
		/* emulator create */
		F3812[i] = OPLCreate(chiptype,intf->baseclock,rate);
		if(F3812[i] == NULL) return 1;
		/* stream setup */
		sprintf(name,"%s #%d",chipname,i);
		//stream[i] = stream_init(name,vol,rate,FM_OUTPUT_BIT,i,UpdateHandler);
		stream[i] = stream_init(name,vol,rate,FM_OUTPUT_BIT,(int)F3812[i],(STREAM_HANDLER)YM3812UpdateOne);
		/* YM3812 setup */
		OPLSetTimerHandler(F3812[i],TimerHandler,i*2);
		OPLSetIRQHandler(F3812[i]  ,IRQHandler,i);
		OPLSetUpdateHandler(F3812[i],stream_update,stream[i]);
	}
	return 0;
}

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void emu_YM3812_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
	{
		OPLDestroy(F3812[i]);
	}
}

/* reset */
void emu_YM3812_sh_reset(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
		OPLResetChip(F3812[i]);
}

int emu_YM3812_status_port_0_r(int offset)
{
	return OPLRead(F3812[0],0);
}
void emu_YM3812_control_port_0_w(int offset,int data)
{
	OPLWrite(F3812[0],0,data);
}
void emu_YM3812_write_port_0_w(int offset,int data)
{
	OPLWrite(F3812[0],1,data);
}

int emu_YM3812_read_port_0_r( int offset )
{
	return OPLRead(F3812[0],1);
}

#else /* NEW_OPLEMU */
/**********************************************************************************************
	Begin of emulated YM3812 interface block
 **********************************************************************************************/

void timer_callback( int timer ) {

	if ( ym3812_TimerEvent( ym, timer ) ) { /* update sr */
			if (intf->handler)
				(*intf->handler)();
	}
}

void timer_handler( int timer, double period, ym3812 *pOPL, int Tremove ) {
	switch( timer ) {
		case 1:
            if ( Tremove ) {
				if ( timer1 ) {
					timer_remove( timer1 );
					timer1 = 0;
				}
			} else
				timer1 = timer_pulse( period, timer, timer_callback );
		break;

		case 2:
            if ( Tremove ) {
				if ( timer2 ) {
					timer_remove( timer2 );
					timer2 = 0;
				}
			} else
				timer2 = timer_pulse( period, timer, timer_callback );
		break;
	}
}

void emu_ym3812_fixed_pointer_problem_update( int nNoll, void *pBuffer, int nLength )
{
	// The ym3812 supports several ym chips, update the interface if you want to use this feature!!!
	ym3812_Update( ym, pBuffer, nLength );
}

int emu_YM3812_sh_start(const struct MachineSound *msound)
{
	int rate = Machine->sample_rate;

	if ( ym )		/* duplicate entry */
		return 1;

	intf = msound->sound_interface;

	sample_bits = Machine->sample_bits;

    ym = ym3812_Init( rate, intf->baseclock, ( sample_bits == 16 ) );

	if ( ym == NULL )
		return -1;

	timer1 = timer2 = 0;
	ym->SetTimer = timer_handler;

   ym_channel = stream_init("%s #%d",chipname,intf->mixing_level[0],rate,sample_bits,
			0,emu_ym3812_fixed_pointer_problem_update);
	return 0;
}

void emu_YM3812_sh_stop( void ) {

	if ( ym ) {
		ym = ym3812_DeInit( ym );
		ym = 0;
	}

	if ( timer1 ) {
		timer_remove( timer1 );
		timer1 = 0;
	}

	if ( timer2 ) {
		timer_remove( timer2 );
		timer2 = 0;
	}

	if ( buffer )
		free( buffer );
}

int emu_YM3812_status_port_0_r( int offset ) {
	return ym3812_ReadStatus( ym );
}

void emu_YM3812_control_port_0_w( int offset, int data ) {
	ym3812_SetReg( ym, data );
}

void emu_YM3812_write_port_0_w( int offset, int data ) {
	stream_update( ym_channel, 0 );	// Update the buffer before writing new regs
	ym3812_WriteReg( ym, data );
}

int emu_YM3812_read_port_0_r( int offset ) {
	return ym3812_ReadReg( ym );
}

/**********************************************************************************************
	End of emulated YM3812 interface block
 **********************************************************************************************/
#endif /* NEW_OPLEMU */

/**********************************************************************************************
	Begin of YM3812 interface stubs block
 **********************************************************************************************/

static int OPL_sh_start(const struct MachineSound *msound)
{
	if ( options.use_emulated_ym3812 ) {
		sh_stop = emu_YM3812_sh_stop;
		status_port_0_r = emu_YM3812_status_port_0_r;
		control_port_0_w = emu_YM3812_control_port_0_w;
		write_port_0_w = emu_YM3812_write_port_0_w;
		read_port_0_r = emu_YM3812_read_port_0_r;
		return emu_YM3812_sh_start(msound);
	} else {
		sh_stop = nonemu_YM3812_sh_stop;
		status_port_0_r = nonemu_YM3812_status_port_0_r;
		control_port_0_w = nonemu_YM3812_control_port_0_w;
		write_port_0_w = nonemu_YM3812_write_port_0_w;
		read_port_0_r = nonemu_YM3812_read_port_0_r;
		return nonemu_YM3812_sh_start(msound);
	}
}

int YM3812_sh_start(const struct MachineSound *msound)
{
	chipname = "YM3812";
#if NEW_OPLEMU
	chiptype = OPL_TYPE_YM3812;
#endif
	return OPL_sh_start(msound);
}

void YM3812_sh_stop( void ) {
	(*sh_stop)();
}

int YM3812_status_port_0_r( int offset ) {
	return (*status_port_0_r)( offset );
}

void YM3812_control_port_0_w( int offset, int data ) {
	(*control_port_0_w)( offset, data );
}

void YM3812_write_port_0_w( int offset, int data ) {
	(*write_port_0_w)( offset, data );
}

int YM3812_read_port_0_r( int offset ) {
	return (*read_port_0_r)( offset );
}

/**********************************************************************************************
	End of YM3812 interface stubs block
 **********************************************************************************************/

/**********************************************************************************************
	Begin of YM3526 interface stubs block
 **********************************************************************************************/
int YM3526_sh_start(const struct MachineSound *msound)
{
	chipname = "YM3526";
#if NEW_OPLEMU
	chiptype = OPL_TYPE_YM3526;
#endif
	return OPL_sh_start(msound);
}

/**********************************************************************************************
	End of YM3526 interface stubs block
 **********************************************************************************************/

/**********************************************************************************************
	Begin of Y8950 interface stubs block
 **********************************************************************************************/
int Y8950_sh_start(const struct MachineSound *msound)
{
	chipname = "Y8950";
	if( OPL_sh_start(msound) ) return 1;
#if NEW_OPLEMU
	chiptype = OPL_TYPE_Y8950;
#endif
	/* !!!!! port handler set !!!!! */
	/* !!!!! delta-t memory address set !!!!! */
	return 0;
}

/**********************************************************************************************
	End of Y8950 interface stubs block
 **********************************************************************************************/
