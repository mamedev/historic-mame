#ifndef namco_63701x_h
#define namco_63701x_h

struct namco_63701x_interface
{
	int baseclock;		/* clock */
	int mixing_level;	/* volume */
	int region;			/* memory region; region MUST be 0x80000 bytes long */
};

int namco_63701x_sh_start(const struct MachineSound *msound);
void namco_63701x_sh_stop(void);

void namco_63701x_write(int offset,int data);

#endif

