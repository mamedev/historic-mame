#ifndef _CDDA_H_
#define _CDDA_H_

#define MAX_CDDA	(2)

struct CDDAinterface 
{
	int num;		  	/* number of CD drives to play audio */
	int mixing_level[MAX_CDDA];	/* volume */
};

int CDDA_sh_start( const struct MachineSound *msound );
void CDDA_sh_stop( void );
void CDDA_set_cdrom(int num, void *file);

#endif

