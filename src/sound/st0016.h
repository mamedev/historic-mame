#ifndef _ST0016_H_
#define _ST0016_H_

struct ST0016interface 
{
	data8_t **p_soundram;
};

extern data8_t *st0016_sound_regs;

WRITE8_HANDLER(st0016_snd_w);

#endif

