#define MAX_76496 4


struct SN76496interface
{
	int num;	/* total number of 76496 in the machine */
	int clock;
	int volume[MAX_76496];
};

struct SN76496
{
	/* set this before calling SN76496Reset() */
	int Clock;			/* chip clock in Hz     */
	int freqStep;		/* frequency count step */
	int Volume[4];		/* volume of voice 0-2 and noise. Range is 0-0x1fff */
	int NoiseFB;		/* noise feedback mask */
	int Register[8];	/* registers */
	int LastRegister;	/* last writed register */
	int Counter[4];		/* frequency counter    */
	int Turn[4];
	int Dir[4];			/* output direction     */
	int VolTable[16];	/* volume tables        */
	unsigned int NoiseGen;		/* noise generator      */
};

int SN76496_sh_start(struct SN76496interface *interface);
void SN76496_sh_stop(void);
void SN76496_0_w(int offset,int data);
void SN76496_1_w(int offset,int data);
void SN76496_2_w(int offset,int data);
void SN76496_3_w(int offset,int data);
void SN76496_sh_update(void);
