#include "samples.h"
#include "dac.h"
#include "8910intf.h"
#include "2203intf.h"
#include "2151intf.h"
#include "3812intf.h"
#include "sn76496.h"
#include "pokyintf.h"
#include "namco.h"
#include "nesintf.h"
#include "5220intf.h"
#include "vlm5030.h"
#include "adpcm.h"


void soundlatch_w(int offset,int data);
int soundlatch_r(int offset);
void soundlatch2_w(int offset,int data);
int soundlatch2_r(int offset);
void soundlatch3_w(int offset,int data);
int soundlatch3_r(int offset);
void soundlatch4_w(int offset,int data);
int soundlatch4_r(int offset);

int get_play_channels(int request);
void reset_play_channels(void);

int sound_start(void);
void sound_stop(void);
void sound_update(void);


/* structure for SOUND_CUSTOM sound drivers */
struct CustomSound_interface
{
	int (*sh_start)(void);
	void (*sh_stop)(void);
	void (*sh_update)(void);
};
