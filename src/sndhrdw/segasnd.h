/*************************************************************************

	Sega g80 common sound hardware

*************************************************************************/

WRITE_HANDLER( sega_sh_speechboard_w );

ADDRESS_MAP_EXTERN(sega_speechboard_readmem);
ADDRESS_MAP_EXTERN(sega_speechboard_writemem);
ADDRESS_MAP_EXTERN(sega_speechboard_readport);
ADDRESS_MAP_EXTERN(sega_speechboard_writeport);

extern struct sp0250_interface sega_sp0250_interface;

