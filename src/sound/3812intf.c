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

/* Emulated YM3812 variables and defines */
static int stream[MAX_3812];
static void *Timer[MAX_3812*2];

/* Non-Emulated YM3812 variables and defines */
typedef struct non_emu3812_state {
	int address_register;
	unsigned char status_register;
	unsigned char timer_register;
	unsigned int timer1_val;
	unsigned int timer2_val;
	void *timer1;
	void *timer2;
	int aOPLFreqArray[16];		// Up to 9 channels..
}NE_OPL_STATE;

static double timer_step;
static NE_OPL_STATE *nonemu_state;

/* These ones are used by both */
static const struct YM3812interface *intf = NULL;

/* Function procs to access the selected YM type */
/* static int ( *sh_start )( const struct MachineSound *msound ); */
static void ( *sh_stop )( void );
static int ( *status_port_r )( int chip );
static void ( *control_port_w )( int chip, int data );
static void ( *write_port_w )( int chip, int data );
static int ( *read_port_r )( int chip );

/**********************************************************************************************
	Begin of non-emulated YM3812 interface block
 **********************************************************************************************/

static void timer1_callback (int chip)
{
	NE_OPL_STATE *st = &nonemu_state[chip];
	if (!(st->timer_register & 0x40))
	{
		if(!(st->status_register&0x80))
			if (intf->handler[chip]) (intf->handler[chip])(ASSERT_LINE);
		/* set the IRQ and timer 1 signal bits */
		st->status_register |= 0x80|0x40;
	}

	/* next! */
	st->timer1 = timer_set ((double)st->timer1_val*4*timer_step, chip, timer1_callback);
}

static void timer2_callback (int chip)
{
	NE_OPL_STATE *st = &nonemu_state[chip];
	if (!(st->timer_register & 0x20))
	{
		if(!(st->status_register&0x80))
			if (intf->handler[chip]) (intf->handler[chip])(ASSERT_LINE);
		/* set the IRQ and timer 2 signal bits */
		st->status_register |= 0x80|0x20;
	}

	/* next! */
	st->timer2 = timer_set ((double)st->timer2_val*16*timer_step, chip, timer2_callback);
}

static int nonemu_YM3812_sh_start(const struct MachineSound *msound)
{
	int i;

	intf = msound->sound_interface;

	nonemu_state = malloc(intf->num * sizeof(NE_OPL_STATE) );
	if(nonemu_state==NULL) return 1;
	memset(nonemu_state,0,intf->num * sizeof(NE_OPL_STATE));
	for(i=0;i<intf->num;i++)
	{
		nonemu_state[i].address_register = 0;
		nonemu_state[i].timer1 =
		nonemu_state[i].timer2 = 0;
		nonemu_state[i].status_register = 0;
		nonemu_state[i].timer_register = 0;
		nonemu_state[i].timer1_val =
		nonemu_state[i].timer2_val = 256;
	}
	timer_step = TIME_IN_HZ((double)intf->baseclock / 72.0);
	return 0;
}

static void nonemu_YM3812_sh_stop(void)
{
	YM3812_sh_reset();
	free(nonemu_state);
}

static int nonemu_YM3812_status_port_r(int chip)
{
	NE_OPL_STATE *st = &nonemu_state[chip];
	/* mask out the timer 1 and 2 signal bits as requested by the timer register */
	return st->status_register & ~(st->timer_register & 0x60);
}

static void nonemu_YM3812_control_port_w(int chip,int data)
{
	NE_OPL_STATE *st = &nonemu_state[chip];
	st->address_register = data;

	/* pass through all non-timer registers */
#ifdef OPL3CONVERTFREQUENCY
	if ( ((data==0xbd)||((data&0xe0)!=0xa0)) && ((data<2)||(data>4)) )
#else
	if ( ((data<2)||(data>4)) )
#endif
		osd_opl_control(chip,data);
}

static void nonemu_WriteConvertedFrequency( int chip,int nFrq, int nCh )
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
	osd_opl_control(chip,0xa0|nCh);
	osd_opl_write(chip,((int)vRealFrq)&0xff);
	osd_opl_control(chip,0xb0|nCh);
	osd_opl_write(chip,((((int)vRealFrq)>>8)&3)|(nRealOctave<<2)|((nFrq&0x8000)>>10) );
}

static void nonemu_YM3812_write_port_w(int chip,int data)
{
	NE_OPL_STATE *st = &nonemu_state[chip];
	int nCh = st->address_register&0x0f;

#ifdef OPL3CONVERTFREQUENCY
	if( (nCh<9) )
	{
		if( (st->address_register&0xf0) == 0xa0 )
		{
			st->aOPLFreqArray[nCh] = (st->aOPLFreqArray[nCh] & 0xf300)|(data&0xff);
			nonemu_WriteConvertedFrequency(chip, st->aOPLFreqArray[nCh], nCh );
			return;
		}
		else if( (st->address_register&0xf0)==0xb0 )
		{
			st->aOPLFreqArray[st->address_register&0xf] = (st->aOPLFreqArray[nCh] & 0x00ff)|((data&0x3)<<8)|((data&0x1c)<<10)|((data&0x20)<<10);
			nonemu_WriteConvertedFrequency(chip, st->aOPLFreqArray[nCh], nCh );
			return;
		}
	}
#endif
	switch (st->address_register)
	{
		case 2:
			st->timer1_val = 256 - data;
			break;
		case 3:
			st->timer2_val = 256 - data;
			break;
		case 4:
			/* bit 7 means reset the IRQ signal and status bits, and ignore all the other bits */
			if (data & 0x80)
			{
				if(st->status_register&0x80)
					if (intf->handler[chip]) (intf->handler[chip])(CLEAR_LINE);
				st->status_register = 0;
			}
			else
			{
				/* set the new timer register */
				st->timer_register = data;
					/*  bit 0 starts/stops timer 1 */
				if (data & 0x01)
				{
					if (!st->timer1)
						st->timer1 = timer_set ((double)st->timer1_val*4*timer_step, chip, timer1_callback);
				}
				else if (st->timer1)
				{
					timer_remove (st->timer1);
					st->timer1 = 0;
				}
				/*  bit 1 starts/stops timer 2 */
				if (data & 0x02)
				{
					if (!st->timer2)
						st->timer2 = timer_set ((double)st->timer2_val*16*timer_step, chip, timer2_callback);
				}
				else if (st->timer2)
				{
					timer_remove (st->timer2);
					st->timer2 = 0;
				}
				/* bits 5 & 6 clear and mask the appropriate bit in the status register */
				st->status_register &= ~(data & 0x60);

				if(!(st->status_register&0x7f))
				{
					if(st->status_register&0x80)
						if (intf->handler[chip]) (intf->handler[chip])(CLEAR_LINE);
					st->status_register &=0x7f;
				}
			}
			break;
		default:
			osd_opl_write(chip,data);
	}
}

static int nonemu_YM3812_read_port_r( int chip ) {
	return 0;
}

/**********************************************************************************************
	End of non-emulated YM3812 interface block
 **********************************************************************************************/

#if NEW_OPLEMU

#include "sound/fmopl.h"

typedef void (*STREAM_HANDLER)(int param,void *buffer,int length);

static int chiptype;
static FM_OPL *F3812[MAX_3812];

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if (intf->handler[n]) (intf->handler[n])(irq ? ASSERT_LINE : CLEAR_LINE);
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
	int i;
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
		sprintf(name,"%s #%d",sound_name(msound),i);
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
static void emu_YM3812_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
	{
		OPLDestroy(F3812[i]);
	}
}

/* reset */
static void emu_YM3812_sh_reset(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
		OPLResetChip(F3812[i]);
}

static int emu_YM3812_status_port_r(int chip)
{
	return OPLRead(F3812[chip],0);
}
static void emu_YM3812_control_port_w(int chip,int data)
{
	OPLWrite(F3812[chip],0,data);
}
static void emu_YM3812_write_port_w(int chip,int data)
{
	OPLWrite(F3812[chip],1,data);
}

static int emu_YM3812_read_port_r(int chip)
{
	return OPLRead(F3812[chip],1);
}

#else /* NEW_OPLEMU */
/**********************************************************************************************
	Begin of emulated YM3812 interface block
 **********************************************************************************************/

/* Emulated YM3812 variables and defines */
static ym3812* V3812[MAX_3812];
static int sample_bits;

static void timer_callback( int param )
{
	int chip = param/2;
	int timer_sel = (param&1)+1;

	if ( ym3812_TimerEvent( V3812[chip], timer_sel ) )
	{ /* update sr */
		if (intf->handler[chip]) (intf->handler[chip])(PULSE_LINE);
	}
}

static void timer_handler(int num, double period, ym3812 *pOPL, int Tremove )
{
	if ( Tremove )
	{
		if ( Timer[num] )
		{
			timer_remove( Timer[num] );
			Timer[num] = 0;
		}
	}
	else Timer[num] = timer_pulse( period, num, timer_callback );
}

static void timer_handler_0( int timer_sel, double period, ym3812 *pOPL, int Tremove )
{
	timer_handler(0*2+(timer_sel-1),period,pOPL,Tremove);
}

static void timer_handler_1( int timer_sel, double period, ym3812 *pOPL, int Tremove )
{
	timer_handler(1*2+(timer_sel-1),period,pOPL,Tremove);
}

static void emu_ym3812_fixed_pointer_problem_update( int chip, void *pBuffer, int nLength )
{
	// The ym3812 supports several ym chips, update the interface if you want to use this feature!!!
	ym3812_Update( V3812[chip], pBuffer, nLength );
}

static int emu_YM3812_sh_start(const struct MachineSound *msound)
{
	int rate = Machine->sample_rate;
	int i;

	if ( intf )		/* duplicate entry */
		return 1;
	intf = msound->sound_interface;

	sample_bits = Machine->sample_bits;

	for(i=0;i < intf->num;i++)
	{
		char name[40];
		V3812[i] = ym3812_Init( rate, intf->baseclock, ( sample_bits == 16 ) );
		if ( V3812[i] == NULL )
		{
			for(;i>=0;i--)
			{
				ym3812_DeInit( V3812[i] );
				V3812[i]=0;
			}
			return -1;
		}
		Timer[i*2] = Timer[i*2+1] = 0;
		V3812[i]->SetTimer = i ? timer_handler_1 : timer_handler_0;

		/* stream setup */
		sprintf(name,"%s #%d",sound_name(msound),i);
		stream[i] = stream_init(name,intf->mixing_level[i],rate,sample_bits,
			i,emu_ym3812_fixed_pointer_problem_update);
	}
	return 0;
}

static void emu_YM3812_sh_stop( void )
{
	int i;

	for(i=0;i < intf->num*2;i++)
	{
		if ( Timer[i] )
			timer_remove( Timer[i] );
	}
	for(i=0;i < intf->num;i++)
	{
		ym3812_DeInit(V3812[i] );
		V3812[i] = 0;
	}
}

static void emu_YM3812_sh_reset( void ) {
}

static int emu_YM3812_status_port_r(int chip)
{
	return ym3812_ReadStatus( V3812[chip] );
}

static void emu_YM3812_control_port_w( int chip, int data )
{
	ym3812_SetReg( V3812[chip], data );
}

static void emu_YM3812_write_port_w( int chip, int data ) {
	stream_update( stream[chip], 0 );	// Update the buffer before writing new regs
	ym3812_WriteReg( V3812[chip], data );
}

static int emu_YM3812_read_port_r( int chip )
{
	return ym3812_ReadReg( V3812[chip] );
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
		sh_stop  = emu_YM3812_sh_stop;
		status_port_r = emu_YM3812_status_port_r;
		control_port_w = emu_YM3812_control_port_w;
		write_port_w = emu_YM3812_write_port_w;
		read_port_r = emu_YM3812_read_port_r;
		return emu_YM3812_sh_start(msound);
	} else {
		sh_stop = nonemu_YM3812_sh_stop;
		status_port_r = nonemu_YM3812_status_port_r;
		control_port_w = nonemu_YM3812_control_port_w;
		write_port_w = nonemu_YM3812_write_port_w;
		read_port_r = nonemu_YM3812_read_port_r;
		return nonemu_YM3812_sh_start(msound);
	}
}

int YM3812_sh_start(const struct MachineSound *msound)
{
#if NEW_OPLEMU
	chiptype = OPL_TYPE_YM3812;
#endif
	return OPL_sh_start(msound);
}

void YM3812_sh_stop( void ) {
	(*sh_stop)();
}

void YM3812_sh_reset(void)
{
	int i;

	for(i=0xff;i<=0;i--)
	{
		YM3812_control_port_0_w(0,i);
		YM3812_write_port_0_w(0,0);
	}
	/* IRQ clear */
	YM3812_control_port_0_w(0,4);
	YM3812_write_port_0_w(0,0x80);
}

void YM3812_control_port_0_w( int offset, int data ) {
	(*control_port_w)( 0, data );
}

void YM3812_write_port_0_w( int offset, int data ) {
	(*write_port_w)( 0, data );
}

int YM3812_status_port_0_r( int offset ) {
	return (*status_port_r)( 0 );
}

int YM3812_read_port_0_r( int offset ) {
	return (*read_port_r)( 0 );
}

void YM3812_control_port_1_w( int offset, int data ) {
	(*control_port_w)( 1, data );
}

void YM3812_write_port_1_w( int offset, int data ) {
	(*write_port_w)( 1, data );
}

int YM3812_status_port_1_r( int offset ) {
	return (*status_port_r)( 1 );
}

int YM3812_read_port_1_r( int offset ) {
	return (*read_port_r)( 1 );
}

/**********************************************************************************************
	End of YM3812 interface stubs block
 **********************************************************************************************/

/**********************************************************************************************
	Begin of YM3526 interface stubs block
 **********************************************************************************************/
int YM3526_sh_start(const struct MachineSound *msound)
{
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
#if NEW_OPLEMU
	chiptype = OPL_TYPE_Y8950;
#endif
	if( OPL_sh_start(msound) ) return 1;
	/* !!!!! port handler set !!!!! */
	/* !!!!! delta-t memory address set !!!!! */
	return 0;
}

/**********************************************************************************************
	End of Y8950 interface stubs block
 **********************************************************************************************/
