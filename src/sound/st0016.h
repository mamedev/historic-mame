#ifndef _ST0016_H_
#define _ST0016_H_

struct ST0016interface 
{
	int mixing_level;			/* volume */
};

int st0016_sh_start( const struct MachineSound *msound );
void st0016_sh_stop( void );

extern data8_t *st0016_sound_regs;

WRITE_HANDLER(st0016_snd_w);

#endif

