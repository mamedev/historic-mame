#ifndef INPTPORT_H
#define INPTPORT_H


/* input ports handling */
#define MAX_INPUT_PORTS 8

void load_input_port_settings(const char *name);
void save_input_port_settings(const char *name);

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

int readtrakport(int port);
int input_trak_0_r(int offset);
int input_trak_1_r(int offset);
int input_trak_2_r(int offset);
int input_trak_3_r(int offset);

#endif
