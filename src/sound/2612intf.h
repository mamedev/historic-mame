#ifndef __2612INTF_H__
#define __2612INTF_H__

#ifdef BUILD_YM2612
  void YM2612UpdateRequest(int chip);
#endif

#define   MAX_2612    (2)

/************************************************/
/* Chip 0 functions								*/
/************************************************/
int YM2612_status_port_0_A_r( int offset );
int YM2612_status_port_0_B_r( int offset );
int YM2612_read_port_0_r(int offset);
void YM2612_control_port_0_A_w(int offset,int data);
void YM2612_control_port_0_B_w(int offset,int data);
void YM2612_data_port_0_A_w(int offset,int data);
void YM2612_data_port_0_B_w(int offset,int data);

/************************************************/
/* Chip 1 functions								*/
/************************************************/
int YM2612_status_port_1_A_r( int offset );
int YM2612_status_port_1_B_r( int offset );
int YM2612_read_port_1_r(int offset);
void YM2612_control_port_1_A_w(int offset,int data);
void YM2612_control_port_1_B_w(int offset,int data);
void YM2612_data_port_1_A_w(int offset,int data);
void YM2612_data_port_1_B_w(int offset,int data);

/**************************************************/
/*   YM2612 left/right position change (TAITO)    */
/**************************************************/

#endif
/**************** end of file ****************/
