#ifndef CVSD_H
#define CVSD_H

#define MAX_CVSD 4

struct CVSDinterface
{
	int num;	/* total number of CVSDs */
	int volume[MAX_CVSD];
};

int CVSD_sh_start(struct CVSDinterface *interface);
void CVSD_sh_stop(void);
void CVSD_sh_update(void);
void CVSD_digit_w(int offset, int databit);
void CVSD_dig_and_clk_w(int offset, int databit);
void CVSD_clock_w(int offset, int databit);
void CVSD_set_volume(int num,int volume,int gain);
#endif
