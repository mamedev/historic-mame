/***************************************************************************

	Midway/Williams Audio Board

****************************************************************************/

MACHINE_DRIVER_EXTERN( williams_cvsd_sound );
MACHINE_DRIVER_EXTERN( williams_adpcm_sound );
MACHINE_DRIVER_EXTERN( williams_narc_sound );
MACHINE_DRIVER_EXTERN( williams_dcs_sound );
MACHINE_DRIVER_EXTERN( williams_dcs_uart_sound );

void williams_cvsd_init(int cpunum, int pianum);
void williams_cvsd_data_w(int data);
void williams_cvsd_reset_w(int state);

void williams_adpcm_init(int cpunum);
void williams_adpcm_data_w(int data);
void williams_adpcm_reset_w(int state);

void williams_narc_init(int cpunum);
void williams_narc_data_w(int data);
void williams_narc_reset_w(int state);

void williams_dcs_init(void);
void williams_dcs_set_notify(void (*callback)(int));
int williams_dcs_data_r(void);
int williams_dcs_control_r(void);
void williams_dcs_data_w(int data);
void williams_dcs_reset_w(int state);


