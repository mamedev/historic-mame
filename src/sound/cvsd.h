#ifndef CVSD_H
#define CVSD_H

#define MAX_CVSD 4

struct CVSDinterface
{
	int num;	/* total number of CVSDs */
	int volume[MAX_CVSD];
};

int CVSD_sh_start(const struct MachineSound *msound);
void CVSD_digit_w(int offset, int databit);
void CVSD_dig_and_clk_w(int offset, int databit);
void CVSD_clock_w(int offset, int databit);
#endif
