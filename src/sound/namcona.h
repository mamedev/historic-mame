#ifndef NAMCONA_H
#define NAMCONA_H

struct NAMCONAinterface
{
	int frequency;
	int region;
	int mixing_level;
};


int NAMCONA_sh_start( const struct MachineSound *msound );
void NAMCONA_sh_stop( void );

#endif
