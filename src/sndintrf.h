#include "sound/streams.h"
#include "sound/samples.h"
#include "sound/dac.h"
#include "sound/psgintf.h"
#include "sound/2151intf.h"
#include "sound/2610intf.h"
#include "sound/3812intf.h"
#include "sound/2413intf.h"
#include "sound/sn76496.h"
#include "sound/pokey.h"
#include "sound/namco.h"
#include "sound/nesintf.h"
#include "sound/5220intf.h"
#include "sound/vlm5030.h"
#include "sound/adpcm.h"
#include "sound/upd7759.h"
#include "sound/astrocde.h"
#include "sound/k007232.h"
#include "sound/cvsd.h"


void soundlatch_w(int offset,int data);
int soundlatch_r(int offset);
void soundlatch_clear_w(int offset,int data);
void soundlatch2_w(int offset,int data);
int soundlatch2_r(int offset);
void soundlatch2_clear_w(int offset,int data);
void soundlatch3_w(int offset,int data);
int soundlatch3_r(int offset);
void soundlatch3_clear_w(int offset,int data);
void soundlatch4_w(int offset,int data);
int soundlatch4_r(int offset);
void soundlatch4_clear_w(int offset,int data);

/* If you're going to use soundlatchX_clear_w, and the cleared value is
   something other than 0x00, use this function from machine_init. Note
   that this one call effects all 4 latches */
void soundlatch_setclearedvalue(int value);

int get_play_channels(int request);
void reset_play_channels(void);

int sound_start(void);
void sound_stop(void);
void sound_update(void);

int sound_scalebufferpos(int value);


/* structure for SOUND_CUSTOM sound drivers */
struct CustomSound_interface
{
	int (*sh_start)(void);
	void (*sh_stop)(void);
	void (*sh_update)(void);
};
