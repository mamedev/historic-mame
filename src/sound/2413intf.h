#ifndef YM2413INTF_H
#define YM2413INTF_H

#define MAX_2413 	(4)

struct YM2413interface
{
	int num;
	int baseclock;
	int mixing_level[MAX_2413];
};

WRITE_HANDLER( YM2413_register_port_0_w );
WRITE_HANDLER( YM2413_register_port_1_w );
WRITE_HANDLER( YM2413_register_port_2_w );
WRITE_HANDLER( YM2413_register_port_3_w );
WRITE_HANDLER( YM2413_data_port_0_w );
WRITE_HANDLER( YM2413_data_port_1_w );
WRITE_HANDLER( YM2413_data_port_2_w );
WRITE_HANDLER( YM2413_data_port_3_w );

int YM2413_sh_start (const struct MachineSound *msound);
void YM2413_sh_stop (void);
void YM2413_sh_reset (void);

#endif

