/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/
#ifndef __KDAC_A_H__
#define __KDAC_A_H__

#define MAX_K007232		2


struct K007232_interface
{
	int num_chips;			/* Number of chips */
	int bank[MAX_K007232];	/* memory regions */
	int volume[MAX_K007232];/* volume */
	int limit;
};

int K007232_sh_start(const struct MachineSound *msound);

void K007232_write_port_0_w(int r,int v);
void K007232_write_port_1_w(int r,int v);
int K007232_read_port_0_r(int r);
int K007232_read_port_1_r(int r);

void K007232_bankswitch_0_w(unsigned char *ptr_A,unsigned char *ptr_B);
void K007232_bankswitch_1_w(unsigned char *ptr_A,unsigned char *ptr_B);

#endif
