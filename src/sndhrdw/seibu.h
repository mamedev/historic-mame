/***************************************************************************

	Seibu Sound System v1.02, games using this include:

	Dead Angle       1988?	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Dynamite Duke    1989	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Toki             1989	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Raiden           1990	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Blood Brothers   1990	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	D-Con            1992	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."

	Related sound programs (not implemented yet):

	Cross Shooter    1986?	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812)
	Cabal            1988	* "Michel/Seibu    sound 11/04/88" (YM2151 substituted for YM3812, unknown ADPCM)
	Zero Team            	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Legionaire           	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812)
	Raiden 2             	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
	Raiden DX            	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
	Cup Soccer           	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)

	* = encrypted

***************************************************************************/

extern const struct Memory_ReadAddress seibu_sound_readmem[];
extern const struct Memory_WriteAddress seibu_sound_writemem[];

READ16_HANDLER( seibu_main_word_r );
READ_HANDLER( seibu_main_v30_r );
WRITE16_HANDLER( seibu_main_word_w );
WRITE_HANDLER( seibu_main_v30_w );

WRITE_HANDLER( seibu_irq_clear_w );
WRITE_HANDLER( seibu_rst10_ack_w );
WRITE_HANDLER( seibu_rst18_ack_w );
WRITE_HANDLER( seibu_bank_w );
WRITE_HANDLER( seibu_coin_w );
void seibu_ym3812_irqhandler(int linestate);
READ_HANDLER( seibu_soundlatch_r );
READ_HANDLER( seibu_main_data_pending_r );
WRITE_HANDLER( seibu_main_data_w );
void seibu_sound_init_1(void);
void seibu_sound_init_2(void);
void seibu_sound_decrypt(int cpu_region,int length);

/**************************************************************************/

#define SEIBU_COIN_INPUTS											\
	PORT_START														\
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 4 )			\
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 4 )


#define SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(freq1,freq2,region)		\
																	\
static struct YM3812interface ym3812_interface =					\
{																	\
	1,																\
	freq1,															\
	{ 50 },															\
	{ seibu_ym3812_irqhandler },									\
};																	\
																	\
static struct OKIM6295interface okim6295_interface =				\
{																	\
	1,																\
	{ freq2 },														\
	{ region },														\
	{ 40 }															\
}

#define SEIBU_SOUND_SYSTEM_CPU(freq)								\
	CPU_Z80 | CPU_AUDIO_CPU,										\
	freq,															\
	seibu_sound_readmem,seibu_sound_writemem,0,0,					\
	ignore_interrupt,0

#define SEIBU_SOUND_SYSTEM_YM3812_INTERFACE							\
	{																\
		SOUND_YM3812,												\
		&ym3812_interface											\
	},																\
	{																\
		SOUND_OKIM6295,												\
		&okim6295_interface											\
	}

/**************************************************************************/

