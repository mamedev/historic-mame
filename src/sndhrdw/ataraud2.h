/***************************************************************************

	Atari Audio Board II Interface

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"

void ataraud2_reset(int cpunum, int inputport, int testport, int testmask);

extern struct MemoryReadAddress ataraud2_readmem[];
extern struct MemoryWriteAddress ataraud2_writemem[];

extern struct TMS5220interface ataraud2_tms5220_interface;
extern struct YM2151interface ataraud2_ym2151_interface;
extern struct POKEYinterface ataraud2_pokey_interface;

#define ATARI_AUDIO_2_CPU(mem_region)						\
		CPU_M6502,											\
		1789790,											\
		mem_region,											\
		ataraud2_readmem,ataraud2_writemem,0,0,				\
		0,0,												\
		atarigen_6502_irq_gen,250

#define ATARI_AUDIO_2_INTERFACES 							\
	SOUND_SUPPORTS_STEREO,0,0,0,							\
	{														\
		{													\
			SOUND_YM2151, 									\
			&ataraud2_ym2151_interface 						\
		}, 													\
		{ 													\
			SOUND_POKEY, 									\
			&ataraud2_pokey_interface 						\
		}, 													\
		{ 													\
			SOUND_TMS5220, 									\
			&ataraud2_tms5220_interface 					\
		}													\
	}

#define ATARI_AUDIO_2_PORT									\
	PORT_START												\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* self test */\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* input buffer full */\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* output buffer full */\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* speech chip ready */\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )			\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )				\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )				\
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )

