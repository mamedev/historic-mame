#ifndef __FMOPL_H_
#define __FMOPL_H_

typedef void (*OPL_TIMERHANDLER)(int channel,double interval_Sec);
typedef void (*OPL_IRQHANDLER)(int param,int irq);
typedef void (*OPL_UPDATEHANDLER)(int param,int min_interval_us);

#define BUILD_YM3812 (HAS_YM3812)
#define BUILD_YM3526 (HAS_YM3526)
#define BUILD_Y8950  0 /* (HAS_Y8950) */

/* !!!!! here is private section , do not access there member direct !!!!! */

#define OPL_TYPE_WAVESEL   0x01  /* waveform select    */
#define OPL_TYPE_ADPCM     0x02  /* DELTA-T ADPCM unit */
#define OPL_TYPE_KEYBOARD  0x04  /* keyboard interface */
#define OPL_TYPE_IO        0x08  /* I/O port */

/* ---------- OPL one of slot  ---------- */
typedef struct fm_opl_slot {
	signed int TL;			/* total level     :TL << 8            */
	signed int TLL;			/* adjusted now TL                     */
	unsigned char KSR;		/* key scale rate  :(shift down bit)   */
	int *AR;				/* attack rate     :&AR_TABLE[AR<<2]   */
	int *DR;				/* decay rate      :&DR_TALBE[DR<<2]   */
	int  SL;				/* sustin level    :SL_TALBE[SL]       */
	int *RR;				/* release rate    :&DR_TABLE[RR<<2]   */
	unsigned char ksl;		/* keyscale level  :(shift down bits)  */
	unsigned char ksr;		/* key scale rate  :kcode>>KSR         */
	unsigned int mul;		/* multiple        :ML_TABLE[ML]       */
	unsigned int Cnt;		/* frequency count :                   */
	unsigned int Incr;		/* frequency step  :                   */
	/* envelope generator state */
	unsigned char eg_typ;	/* envelope type flag                  */
	unsigned char evm;		/* envelope phase                      */
	signed int evc;			/* envelope counter                    */
	signed int eve;			/* envelope counter end point          */
	signed int evs;			/* envelope counter step               */
	signed int evsa;		/* envelope step for AR :AR[ksr]       */
	signed int evsd;		/* envelope step for DR :DR[ksr]       */
	signed int evsr;		/* envelope step for RR :RR[ksr]       */
	/* LFO */
	int ams;				/* ams flag                            */
	int vib;				/* vibrate flag                        */
	/* wave selector */
	signed int **wavetable;
}OPL_SLOT;

/* ---------- OPL one of channel  ---------- */
typedef struct fm_opl_channel {
	OPL_SLOT SLOT[2];
	unsigned char CON;			/* connection type                     */
	unsigned char FB;			/* feed back       :(shift down bit)   */
	signed int *connect1;		/* slot1 output pointer                */
	signed int *connect2;		/* slot2 output pointer                */
	signed int op1_out[2];		/* slot1 output for selfeedback        */
	/* phase generator state */
	unsigned int  block_fnum;	/* block+fnum      :                   */
	unsigned char kcode;		/* key code        : KeyScaleCode      */
	unsigned int  fc;			/* Freq. Increment base                */
	unsigned int  ksl_base;		/* KeyScaleLevel Base step             */
	unsigned char keyon;		/* key on/off flag                     */
} OPL_CH;

/* OPL state */
typedef struct fm_opl_f {
	unsigned char type;			/* chip type                        */
	int clock;					/* master clock  (Hz)                */
	int rate;					/* sampling rate (Hz)                */
	int freqbase;				/* frequency base                    */
	double TimerBase;			/* Timer base time (==sampling time) */
	unsigned char address;		/* address register                  */
	unsigned char status;		/* status flag                       */
	unsigned char statusmask;	/* status mask                       */
	unsigned int mode;			/* Reg.08 : CSM , notesel,etc.       */
	/* Timer */
	int T[2];					/* timer counter       */
	int st[2];					/* timer enable        */
	/* FM channel slots */
	OPL_CH *P_CH;				/* pointer of CH       */
	int	max_ch;					/* maximum channel     */
	/* Rythm sention */
	unsigned int rythm;			/* Rythm mode , key flag */
	/* Delta-T ADPCM unit (Y8950) */
	void *adpcm;				/* DELTA-T ADPCM       */
	/* Keyboard / I/O interface unit (Y8950) */
	int portDirection;
	int portLatch;
	/* time tables */
	signed int AR_TABLE[75];	/* atttack rate tables */
	signed int DR_TABLE[75];	/* decay rate tables   */
	unsigned int FN_TABLE[1024]; /* fnumber -> increment counter */
	/* LFO */
	unsigned int  *ams_table;
	unsigned int  *vib_table;
	unsigned long amsCnt;
	unsigned long amsIncr;
	unsigned long vibCnt;
	unsigned long vibIncr;
	/* wave selector enable flag */
	unsigned char wavesel;
	/* event callback handler */
	OPL_TIMERHANDLER  TimerHandler;
	int TimerParam;
	OPL_IRQHANDLER    IRQHandler;
	int IRQParam;
	OPL_UPDATEHANDLER UpdateHandler;
	int UpdateParam;
} FM_OPL;

/* ---------- Generic interface section ---------- */
#define OPL_TYPE_YM3526 (0)
#define OPL_TYPE_YM3812 (OPL_TYPE_WAVESEL)
#define OPL_TYPE_Y8950  (OPL_TYPE_ADPCM|OPL_TYPE_KEYBOARD|OPL_TYPE_IO)

FM_OPL *OPLCreate(int type, int clock, int rate);
void OPLDestroy(FM_OPL *OPL);
void OPLSetTimerHandler(FM_OPL *OPL,OPL_TIMERHANDLER TimerHandler,int channelOffset);
void OPLSetIRQHandler(FM_OPL *OPL,OPL_IRQHANDLER IRQHandler,int param);
void OPLSetUpdateHandler(FM_OPL *OPL,OPL_UPDATEHANDLER UpdateHandler,int param);
void OPLResetChip(FM_OPL *OPL);
int OPLWrite(FM_OPL *OPL,int a,int v);
unsigned char OPLRead(FM_OPL *OPL,int a);
int OPLTimerOver(FM_OPL *OPL,int c);

/* YM3626/YM3812 local section */
void YM3812UpdateOne(FM_OPL *OPL, void *buffer, int length);

#endif
