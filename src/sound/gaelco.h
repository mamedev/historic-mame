#ifndef gaelco_snd_h
#define gaelco_snd_h

struct gaelcosnd_interface
{
	int region;				/* memory region */
	int banks[4];			/* start of each ROM bank */
};

#ifdef __cplusplus
extern "C" {
#endif

WRITE16_HANDLER( gaelcosnd_w );
READ16_HANDLER( gaelcosnd_r );

#ifdef __cplusplus
}
#endif

#endif
