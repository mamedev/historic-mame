

#ifndef DAC_H
#define DAC_H

#define MAX_DAC 4

struct DACinterface
{
	int num;			/* total number of 8910 in the machine */
	int rate;			/* DAC sampling rate  */
	int volume[MAX_DAC];/* master volume rate */
	int filter[MAX_DAC];/* filter rate (rate = Register(ohm)*Capaciter(F)*1000000) */
};

int DAC_sh_start(struct DACinterface *interface);
void DAC_sh_stop(void);
void DAC_sh_update(void);
void DAC_data_w(int num , int newdata);
void DAC_volume(int num,int vol );
#endif
