#ifndef NAMCOS1_H
#define NAMCOS1_H

#define MAX_NAMCOS1 1

struct namcos1_interface
{
	int clock;
	int volume;
	int region;
};

int namcos1_sh_start(struct namcos1_interface *interface);
void namcos1_sh_stop(void);
void namcos1_sh_update(void);
int namcos1_sound_r(int num);
void namcos1_sound_w(int num,int data);
#endif
