#ifndef PSGINTF_H
#define PSGINTF_H

#include "psg.h"

#define MAX_8910 MAX_PSG
/* OPN use 2(SSG 1,FM 1) channel per chip */
#ifdef OPN_MIX
#define MAX_2203 2
#else
#define MAX_2203 4
#endif

struct PSGinterface
{
	int num;	/* total number of 8910 in the machine */
	int clock;
	int volume[MAX_PSG];
	int (*portAread[MAX_PSG])(int offset);
	int (*portBread[MAX_PSG])(int offset);
	void (*portAwrite[MAX_PSG])(int offset,int data);
	void (*portBwrite[MAX_PSG])(int offset,int data);
	void (*handler[MAX_PSG])(void);	/* IRQ handler for the YM2203 */
};
#define AY8910interface PSGinterface
#define YM2203interface PSGinterface

/* volume level for YM2203 */
#define YM2203_VOL(FM_VOLUME,SSG_VOLUME) (((FM_VOLUME)<<16)+(SSG_VOLUME))

int PSG_read_port_0_r(int offset);
int PSG_read_port_1_r(int offset);
int PSG_read_port_2_r(int offset);
int PSG_read_port_3_r(int offset);
int PSG_read_port_4_r(int offset);
#define AY8910_read_port_0_r PSG_read_port_0_r
#define AY8910_read_port_1_r PSG_read_port_1_r
#define AY8910_read_port_2_r PSG_read_port_2_r
#define AY8910_read_port_3_r PSG_read_port_3_r
#define AY8910_read_port_4_r PSG_read_port_4_r
#define YM2203_read_port_0_r PSG_read_port_0_r
#define YM2203_read_port_1_r PSG_read_port_1_r
#define YM2203_read_port_2_r PSG_read_port_2_r
#define YM2203_read_port_3_r PSG_read_port_3_r
#define YM2203_read_port_4_r PSG_read_port_4_r

int YM2203_status_port_0_r(int offset);
int YM2203_status_port_1_r(int offset);
int YM2203_status_port_2_r(int offset);
int YM2203_status_port_3_r(int offset);
int YM2203_status_port_4_r(int offset);

void PSG_control_port_0_w(int offset,int data);
void PSG_control_port_1_w(int offset,int data);
void PSG_control_port_2_w(int offset,int data);
void PSG_control_port_3_w(int offset,int data);
void PSG_control_port_4_w(int offset,int data);
#define AY8910_control_port_0_w PSG_control_port_0_w
#define AY8910_control_port_1_w PSG_control_port_1_w
#define AY8910_control_port_2_w PSG_control_port_2_w
#define AY8910_control_port_3_w PSG_control_port_3_w
#define AY8910_control_port_4_w PSG_control_port_4_w
#define YM2203_control_port_0_w PSG_control_port_0_w
#define YM2203_control_port_1_w PSG_control_port_1_w
#define YM2203_control_port_2_w PSG_control_port_2_w
#define YM2203_control_port_3_w PSG_control_port_3_w
#define YM2203_control_port_4_w PSG_control_port_4_w


void PSG_write_port_0_w(int offset,int data);
void PSG_write_port_1_w(int offset,int data);
void PSG_write_port_2_w(int offset,int data);
void PSG_write_port_3_w(int offset,int data);
void PSG_write_port_4_w(int offset,int data);
#define AY8910_write_port_0_w PSG_write_port_0_w
#define AY8910_write_port_1_w PSG_write_port_1_w
#define AY8910_write_port_2_w PSG_write_port_2_w
#define AY8910_write_port_3_w PSG_write_port_3_w
#define AY8910_write_port_4_w PSG_write_port_4_w
#define YM2203_write_port_0_w PSG_write_port_0_w
#define YM2203_write_port_1_w PSG_write_port_1_w
#define YM2203_write_port_2_w PSG_write_port_2_w
#define YM2203_write_port_3_w PSG_write_port_3_w
#define YM2203_write_port_4_w PSG_write_port_4_w

int AY8910_sh_start(struct PSGinterface *interface);
int YM2203_sh_start(struct PSGinterface *interface);
#define AY8910_sh_stop PSG_sh_stop
#define YM2203_sh_stop PSG_sh_stop
void PSG_sh_stop(void);
#define AY8910_sh_update PSG_sh_update
#define YM2203_sh_update PSG_sh_update
void PSG_sh_update(void);

#endif
