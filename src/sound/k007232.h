/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/
#ifndef __KDAC_A_H__
#define __KDAC_A_H__


struct K007232_interface
{
	int bank[2];	/* memory regions for channel A and B */
	int volume[2];	/* volume for channel A and B */
	int limit;
};


int K007232_sh_start(const struct MachineSound *msound);
void K007232_WriteReg(int r,int v);
int K007232_ReadReg(int r);

#endif
