/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/
#ifndef __KDAC_A_H__
#define __KDAC_A_H__


struct K007232_interface
{
	int bankA, bankB;	/* memory regions for channel A and B */
	int volume;
	int limit;
};


int K007232_sh_start(const struct K007232_interface *intf);
void K007232_WriteReg(int r,int v);
int K007232_ReadReg(int r);

#endif
