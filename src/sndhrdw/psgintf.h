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

/*-------------------- AY-3-8910 -------------------- */
int AY8910_read_port_0_r(int offset);
int AY8910_read_port_1_r(int offset);
int AY8910_read_port_2_r(int offset);
int AY8910_read_port_3_r(int offset);
int AY8910_read_port_4_r(int offset);

void AY8910_control_port_0_w(int offset,int data);
void AY8910_control_port_1_w(int offset,int data);
void AY8910_control_port_2_w(int offset,int data);
void AY8910_control_port_3_w(int offset,int data);
void AY8910_control_port_4_w(int offset,int data);

void AY8910_write_port_0_w(int offset,int data);
void AY8910_write_port_1_w(int offset,int data);
void AY8910_write_port_2_w(int offset,int data);
void AY8910_write_port_3_w(int offset,int data);
void AY8910_write_port_4_w(int offset,int data);

int AY8910_sh_start(struct PSGinterface *interface);
void AY8910_sh_stop(void);
void AY8910_sh_update(void);

/*-------------------- YM2203 -------------------- */

int YM2203_status_port_0_r(int offset);
int YM2203_status_port_1_r(int offset);
int YM2203_status_port_2_r(int offset);
int YM2203_status_port_3_r(int offset);
int YM2203_status_port_4_r(int offset);

int YM2203_read_port_0_r(int offset);
int YM2203_read_port_1_r(int offset);
int YM2203_read_port_2_r(int offset);
int YM2203_read_port_3_r(int offset);
int YM2203_read_port_4_r(int offset);

void YM2203_control_port_0_w(int offset,int data);
void YM2203_control_port_1_w(int offset,int data);
void YM2203_control_port_2_w(int offset,int data);
void YM2203_control_port_3_w(int offset,int data);
void YM2203_control_port_4_w(int offset,int data);

void YM2203_write_port_0_w(int offset,int data);
void YM2203_write_port_1_w(int offset,int data);
void YM2203_write_port_2_w(int offset,int data);
void YM2203_write_port_3_w(int offset,int data);
void YM2203_write_port_4_w(int offset,int data);

int YM2203_sh_start(struct PSGinterface *interface);
void YM2203_sh_stop(void);
void YM2203_sh_update(void);

/*-------------------- YM2608 -------------------- */
#define YM2608interface PSGinterface

int YM2608_address0_0_r(int offset); /* OPN Status port */
int YM2608_address0_1_r(int offset);
int YM2608_address1_0_r(int offset); /* SSG Read port   */
int YM2608_address1_1_r(int offset);
int YM2608_address2_0_r(int offset); /* OPNA Status port */
int YM2608_address2_1_r(int offset);
int YM2608_address3_0_r(int offset); /* unknown          */
int YM2608_address3_1_r(int offset);

void YM2608_address0_0_w(int offset,int data); /* Register port0 */
void YM2608_address0_1_w(int offset,int data);
void YM2608_address1_0_w(int offset,int data); /* data port 0    */
void YM2608_address1_1_w(int offset,int data);
void YM2608_address2_0_w(int offset,int data); /* Register port1 */
void YM2608_address2_1_w(int offset,int data);
void YM2608_address3_0_w(int offset,int data); /* data port 1    */
void YM2608_address3_1_w(int offset,int data);

int  YM2608_sh_start(struct YM2608interface *interface);
void YM2608_sh_stop(void);
void YM2608_sh_update(void);

/*-------------------- YM2612 -------------------- */
#define YM2612interface PSGinterface

int YM2612_address0_0_r(int offset); /* OPN Status port */
int YM2612_address0_1_r(int offset);
int YM2612_address1_0_r(int offset); /* SSG Read port   */
int YM2612_address1_1_r(int offset);
int YM2612_address2_0_r(int offset); /* OPNA Status port */
int YM2612_address2_1_r(int offset);
int YM2612_address3_0_r(int offset); /* unknown          */
int YM2612_address3_1_r(int offset);

void YM2612_address0_0_w(int offset,int data); /* Register port0 */
void YM2612_address0_1_w(int offset,int data);
void YM2612_address1_0_w(int offset,int data); /* data port 0    */
void YM2612_address1_1_w(int offset,int data);
void YM2612_address2_0_w(int offset,int data); /* Register port1 */
void YM2612_address2_1_w(int offset,int data);
void YM2612_address3_0_w(int offset,int data); /* data port 1    */
void YM2612_address3_1_w(int offset,int data);

int  YM2612_sh_start(struct YM2612interface *interface);
void YM2612_sh_stop(void);
void YM2612_sh_update(void);

#endif
