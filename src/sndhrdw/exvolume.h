#ifndef EXVOLUME_H
#define EXVOLUME_H

#define MAX_EXVOLUME 4

#define EXVOLUME_VOL(PAN,VOL) (((PAN)<<8)|(VOL))

struct EXVOLUMEinterface
{
	int num;			        /* total number of input channels */
	char *name[MAX_EXVOLUME];   /* input stream sound name     */
	int volume1[MAX_EXVOLUME];  /* pan & volume to output1     */
	int volume2[MAX_EXVOLUME];  /* pan & volume to output2     */
};

int  EXVOLUME_sh_start (struct EXVOLUMEinterface *interface);
void EXVOLUME_sh_stop (void);
void EXVOLUME_sh_update (void);

/* change pan and base volume (for able to switch external analog circuit) */
void EXVOLUME_set_base_volume(int num, int channel,int volume);
void EXVOLUME_set_base_volume(int num, int channel,int volume);

/* mame I/O interface */
void EXVOLUME_write_CH0_0(int offset,int data); /* chip1 left  to left  */
void EXVOLUME_write_CH1_0(int offset,int data); /* chip1 left  to right */
void EXVOLUME_write_CH0_1(int offset,int data); /* chip1 right to left  */
void EXVOLUME_write_CH1_1(int offset,int data); /* chip1 right to left  */
void EXVOLUME_write_CH0_2(int offset,int data); /* chip2 left  to left  */
void EXVOLUME_write_CH1_2(int offset,int data); /* chip2 left  to right */
void EXVOLUME_write_CH0_3(int offset,int data); /* chip2 right to left  */
void EXVOLUME_write_CH1_3(int offset,int data); /* chip2 left  to right */

#if 0
/* standard interface for TAITO YM2610 mixer */
struct EXVOLUMEinterface
{
	2, /* num */
	{ "YM2610 #0 Ch1","YM2610 #0 Ch2"},
	{ EXVOLUME_VOL(OSD_PAN_LEFT,LVol),EXVOLUME_VOL(OSD_PAN_LEFT,RVol) },
	{ EXVOLUME_VOL(OSD_PAN_RIGHT,RVol),EXVOLUME_VOL(OSD_PAN_RIGHT,LVol) }
};
#endif

#endif
