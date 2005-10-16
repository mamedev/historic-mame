#include "driver.h"
#include "sound/custom.h"

/* flags for init_pc_common */
#define PCCOMMON_KEYBOARD_PC	0
#define PCCOMMON_KEYBOARD_AT	1
#define PCCOMMON_DMA8237_PC		0
#define PCCOMMON_DMA8237_AT		2
#define PCCOMMON_TIMER_8253     0
#define PCCOMMON_TIMER_8254     4

void init_pc_common(UINT32 flags);


void pc_mda_init(void);

void pc_keyboard(void);
UINT8 pc_keyb_read(void);
void pc_keyb_set_clock(int on);
void pc_keyb_clear(void);

READ8_HANDLER ( pc_COM1_r );
WRITE8_HANDLER( pc_COM1_w );
READ8_HANDLER ( pc_COM2_r );
WRITE8_HANDLER( pc_COM2_w );
READ8_HANDLER ( pc_COM3_r );
WRITE8_HANDLER( pc_COM3_w );
READ8_HANDLER ( pc_COM4_r );
WRITE8_HANDLER( pc_COM4_w );

READ32_HANDLER ( pc32_COM1_r );
WRITE32_HANDLER( pc32_COM1_w );
READ32_HANDLER ( pc32_COM2_r );
WRITE32_HANDLER( pc32_COM2_w );
READ32_HANDLER ( pc32_COM3_r );
WRITE32_HANDLER( pc32_COM3_w );
READ32_HANDLER ( pc32_COM4_r );
WRITE32_HANDLER( pc32_COM4_w );

/* from sndhrdw/pc.c */
extern struct CustomSound_interface pc_sound_interface;
void pc_sh_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples);
void pc_sh_speaker(int mode);

void pc_sh_speaker_change_clock(double pc_clock);

READ8_HANDLER ( pc_JOY_r );
WRITE8_HANDLER ( pc_JOY_w );

#define PC_JOYSTICK
INPUT_PORTS_EXTERN( pc_joystick_none );
INPUT_PORTS_EXTERN( pc_joystick );

READ8_HANDLER(pc_page_r);
WRITE8_HANDLER(pc_page_w);

READ8_HANDLER(at_page8_r);
WRITE8_HANDLER(at_page8_w);

READ32_HANDLER(at_page32_r);
WRITE32_HANDLER(at_page32_w);

int pc_turbo_setup(int cpunum, int port, int mask, double off_speed, double on_speed);

