#ifndef INPTPORT_H
#define INPTPORT_H


/* input ports handling */
#define MAX_INPUT_PORTS 16

void load_input_port_settings(void);
void save_input_port_settings(void);

void update_analog_ports(void);
void update_analog_port(int port);
void update_input_ports(void);	/* called by cpu_run() */

int readinputport(int port);
int input_port_0_r(int offset);
int input_port_1_r(int offset);
int input_port_2_r(int offset);
int input_port_3_r(int offset);
int input_port_4_r(int offset);
int input_port_5_r(int offset);
int input_port_6_r(int offset);
int input_port_7_r(int offset);
int input_port_8_r(int offset);
int input_port_9_r(int offset);
int input_port_10_r(int offset);
int input_port_11_r(int offset);
int input_port_12_r(int offset);
int input_port_13_r(int offset);
int input_port_14_r(int offset);
int input_port_15_r(int offset);

int readtrakport(int port);
int input_trak_0_r(int offset);
int input_trak_1_r(int offset);
int input_trak_2_r(int offset);
int input_trak_3_r(int offset);

#endif
