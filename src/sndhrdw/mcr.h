/***************************************************************************

	sndhrdw/mcr.c

	Functions to emulate general the various MCR sound cards.

***************************************************************************/

#include "machine/6821pia.h"



/************ Generic MCR routines ***************/

void mcr_sound_init(void);

void ssio_data_w(int offset, int data);
int ssio_status_r(int offset);
void ssio_reset_w(int state);

void csdeluxe_data_w(int offset, int data);
void csdeluxe_reset_w(int state);

void turbocs_data_w(int offset, int data);
void turbocs_reset_w(int state);

void soundsgood_data_w(int offset, int data);
void soundsgood_reset_w(int state);

void squawkntalk_data_w(int offset, int data);
void squawkntalk_reset_w(int state);

void advaudio_data_w(int offset, int data);
void advaudio_reset_w(int state);



/************ Sound Configuration ***************/

extern UINT8 mcr_sound_config;

#define MCR_SSIO				0x01
#define MCR_CHIP_SQUEAK_DELUXE	0x02
#define MCR_SOUNDS_GOOD			0x04
#define MCR_TURBO_CHIP_SQUEAK	0x08
#define MCR_SQUAWK_N_TALK		0x10
#define MCR_ADVANCED_AUDIO		0x20

#define MCR_CONFIGURE_SOUND(x) \
	mcr_sound_config = x



/************ SSIO CPU and sound definitions ***************/

extern struct MemoryReadAddress ssio_readmem[];
extern struct MemoryWriteAddress ssio_writemem[];

extern struct AY8910interface ssio_ay8910_interface;

#define SOUND_CPU_SSIO(mem)							\
	{												\
		CPU_Z80 | CPU_AUDIO_CPU,					\
		2000000,	/* 2 Mhz */						\
		mem,										\
		ssio_readmem,ssio_writemem,0,0,				\
		interrupt,26								\
	}

#define SOUND_SSIO 									\
	{												\
		SOUND_AY8910,								\
		&ssio_ay8910_interface						\
	}



/************ Chip Squeak Deluxe CPU and sound definitions ***************/

extern struct MemoryReadAddress csdeluxe_readmem[];
extern struct MemoryWriteAddress csdeluxe_writemem[];

extern struct DACinterface csdeluxe_dac_interface;
extern struct DACinterface turbocs_plus_csdeluxe_dac_interface;

#define SOUND_CPU_CHIP_SQUEAK_DELUXE(mem)			\
	{												\
		CPU_M68000 | CPU_AUDIO_CPU,					\
		7500000,	/* 7.5 Mhz */					\
		mem,										\
		csdeluxe_readmem,csdeluxe_writemem,0,0,		\
		ignore_interrupt,1							\
	}

#define SOUND_CHIP_SQUEAK_DELUXE					\
	{												\
		SOUND_DAC,									\
		&csdeluxe_dac_interface						\
	}

#define SOUND_TURBO_CHIP_SQUEAK_PLUS_CSDELUXE		\
	{												\
		SOUND_DAC,									\
		&turbocs_plus_csdeluxe_dac_interface		\
	}



/************ Sounds Good CPU and sound definitions ***************/

extern struct MemoryReadAddress soundsgood_readmem[];
extern struct MemoryWriteAddress soundsgood_writemem[];

#define SOUND_CPU_SOUNDS_GOOD(mem)					\
	{												\
		CPU_M68000 | CPU_AUDIO_CPU,					\
		7500000,	/* 7.5 Mhz */					\
		mem,										\
		soundsgood_readmem,soundsgood_writemem,0,0,	\
		ignore_interrupt,1							\
	}

#define SOUND_SOUNDS_GOOD SOUND_CHIP_SQUEAK_DELUXE



/************ Turbo Chip Squeak CPU and sound definitions ***************/

extern struct MemoryReadAddress turbocs_readmem[];
extern struct MemoryWriteAddress turbocs_writemem[];

#define SOUND_CPU_TURBO_CHIP_SQUEAK(mem)			\
	{												\
		CPU_M6809 | CPU_AUDIO_CPU,					\
		2250000,	/* 2.25 Mhz??? */				\
		mem,										\
		turbocs_readmem,turbocs_writemem,0,0,		\
		ignore_interrupt,1							\
	}

#define SOUND_TURBO_CHIP_SQUEAK SOUND_CHIP_SQUEAK_DELUXE



/************ Squawk & Talk CPU and sound definitions ***************/

extern struct MemoryReadAddress squawkntalk_readmem[];
extern struct MemoryWriteAddress squawkntalk_writemem[];

extern struct TMS5220interface squawkntalk_tms5220_interface;

#define SOUND_CPU_SQUAWK_N_TALK(mem)				\
	{												\
		CPU_M6802 | CPU_AUDIO_CPU,					\
		3580000/4,	/* .8 Mhz */					\
		mem,										\
		squawkntalk_readmem,squawkntalk_writemem,0,0,\
		ignore_interrupt,1							\
	}

#define SOUND_SQUAWK_N_TALK							\
	{												\
		SOUND_TMS5220,								\
		&squawkntalk_tms5220_interface				\
	}



/************ Advanced Audio CPU and sound definitions ***************/

extern struct MemoryReadAddress advaudio_readmem[];
extern struct MemoryWriteAddress advaudio_writemem[];

extern struct YM2151interface advaudio_ym2151_interface;
extern struct DACinterface advaudio_dac_interface;
extern struct CVSDinterface advaudio_cvsd_interface;

#define SOUND_CPU_ADVANCED_AUDIO(mem)				\
	{												\
		CPU_M6809 | CPU_AUDIO_CPU,					\
		8000000/2,	/* 4 Mhz */						\
		mem,										\
		advaudio_readmem,advaudio_writemem,0,0,		\
		ignore_interrupt,1							\
	}

#define SOUND_ADVANCED_AUDIO						\
	{												\
		SOUND_YM2151,								\
		&advaudio_ym2151_interface					\
	},												\
	{												\
		SOUND_DAC,									\
		&advaudio_dac_interface						\
	},												\
	{												\
		SOUND_HC55516,								\
		&advaudio_cvsd_interface					\
	}
