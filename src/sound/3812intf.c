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

/* This frequency is from Yamaha 3812 and 2413 documentation */
#define ym3812_StdClock 3579545


/* Emulated YM3812 variables and defines */
static int stream[MAX_3812];
static void *Timer[MAX_3812*2];

/*static const struct YM3812interface *intf = NULL; */
static const struct Y8950interface *intf = NULL;

/* Function procs to access the selected YM type */
/* static int ( *sh_start )( const struct MachineSound *msound ); */
static void ( *sh_stop )( void );
static int ( *status_port_r )( int chip );
static void ( *control_port_w )( int chip, int data );
static void ( *write_port_w )( int chip, int data );
static int ( *read_port_r )( int chip );


#include "sound/fmopl.h"

typedef void (*STREAM_HANDLER)(int param,void *buffer,int length);

static int chiptype;
static FM_OPL *F3812[MAX_3812];

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if (intf->handler[n]) (intf->handler[n])(irq ? ASSERT_LINE : CLEAR_LINE);
}

/* update handler */
static void YM3812UpdateHandler(int n, INT16 *buf, int length)
{	YM3812UpdateOne(F3812[n],buf,length); }

#if (HAS_Y8950)
static void Y8950UpdateHandler(int n, INT16 *buf, int length)
{	Y8950UpdateOne(F3812[n],buf,length); }

static unsigned char Y8950PortHandler_r(int chip)
{	return intf->portread[chip](chip); }

static void Y8950PortHandler_w(int chip,unsigned char data)
{	intf->portwrite[chip](chip,data); }

static unsigned char Y8950KeyboardHandler_r(int chip)
{	return intf->keyboardread[chip](chip); }

static void Y8950KeyboardHandler_w(int chip,unsigned char data)
{	intf->keyboardwrite[chip](chip,data); }
#endif

/* Timer overflow callback from timer.c */
static void timer_callback_3812(int param)
{
	int n=param>>1;
	int c=param&1;
	OPLTimerOver(F3812[n],c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust(Timer[c], period, c, 0);
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
#if (HAS_Y8950)
		/* ADPCM ROM DATA */
		if(chiptype == OPL_TYPE_Y8950)
		{
			F3812[i]->deltat->memory = (unsigned char *)(memory_region(intf->rom_region[i]));
			F3812[i]->deltat->memory_size = memory_region_length(intf->rom_region[i]);
			stream[i] = stream_init(name,vol,rate,i,Y8950UpdateHandler);
			/* port and keyboard handler */
			OPLSetPortHandler(F3812[i],Y8950PortHandler_w,Y8950PortHandler_r,i);
			OPLSetKeyboardHandler(F3812[i],Y8950KeyboardHandler_w,Y8950KeyboardHandler_r,i);
		}
		else
#endif
		stream[i] = stream_init(name,vol,rate,i,YM3812UpdateHandler);
		/* YM3812 setup */
		OPLSetTimerHandler(F3812[i],TimerHandler,i*2);
		OPLSetIRQHandler(F3812[i]  ,IRQHandler,i);
		OPLSetUpdateHandler(F3812[i],stream_update,stream[i]);

		Timer[i*2+0] = timer_alloc(timer_callback_3812);
		Timer[i*2+1] = timer_alloc(timer_callback_3812);
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

/**********************************************************************************************
	Begin of YM3812 interface stubs block
 **********************************************************************************************/

static int OPL_sh_start(const struct MachineSound *msound)
{
	sh_stop  = emu_YM3812_sh_stop;
	status_port_r = emu_YM3812_status_port_r;
	control_port_w = emu_YM3812_control_port_w;
	write_port_w = emu_YM3812_write_port_w;
	read_port_r = emu_YM3812_read_port_r;
	return emu_YM3812_sh_start(msound);
}

int YM3812_sh_start(const struct MachineSound *msound)
{
	chiptype = OPL_TYPE_YM3812;
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

WRITE_HANDLER( YM3812_control_port_0_w ) {
	(*control_port_w)( 0, data );
}

WRITE_HANDLER( YM3812_write_port_0_w ) {
	(*write_port_w)( 0, data );
}

READ_HANDLER( YM3812_status_port_0_r ) {
	return (*status_port_r)( 0 );
}

READ_HANDLER( YM3812_read_port_0_r ) {
	return (*read_port_r)( 0 );
}

WRITE_HANDLER( YM3812_control_port_1_w ) {
	(*control_port_w)( 1, data );
}

WRITE_HANDLER( YM3812_write_port_1_w ) {
	(*write_port_w)( 1, data );
}

READ_HANDLER( YM3812_status_port_1_r ) {
	return (*status_port_r)( 1 );
}

READ_HANDLER( YM3812_read_port_1_r ) {
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
	chiptype = OPL_TYPE_YM3526;
	return OPL_sh_start(msound);
}

/**********************************************************************************************
	End of YM3526 interface stubs block
 **********************************************************************************************/

/**********************************************************************************************
	Begin of Y8950 interface stubs block
 **********************************************************************************************/
#if (HAS_Y8950)
int Y8950_sh_start(const struct MachineSound *msound)
{
	chiptype = OPL_TYPE_Y8950;
	if( OPL_sh_start(msound) ) return 1;
	/* !!!!! port handler set !!!!! */
	/* !!!!! delta-t memory address set !!!!! */
	return 0;
}
#endif

/**********************************************************************************************
	End of Y8950 interface stubs block
 **********************************************************************************************/
