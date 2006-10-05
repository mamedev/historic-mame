extern DRIVER_INIT( megadrie );
extern DRIVER_INIT( megadriv );
extern DRIVER_INIT( megadrij );
extern DRIVER_INIT( _32x );

extern void construct_ipt_megadriv(input_port_init_params *param);
extern void construct_ipt_megadri6(input_port_init_params *param);
extern void construct_ipt_ssf2ghw(input_port_init_params *param);

MACHINE_DRIVER_EXTERN(megadriv);
MACHINE_DRIVER_EXTERN(megadpal);
MACHINE_DRIVER_EXTERN(_32x);

extern UINT16* megadriv_backupram;
extern int megadriv_backupram_length;
extern UINT16* megadrive_ram;

