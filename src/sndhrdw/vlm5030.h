#ifndef VLM5030_h
#define VLM5030_h

struct VLM5030interface
{
	int clock;
	int volume;
};

/* use sampling data when speech_rom == 0 */
int VLM5030_sh_start( struct VLM5030interface *interface , unsigned char *speech_rom );
void VLM5030_sh_stop (void);
void VLM5030_sh_update (void);

void VLM5030_update(void);
int VLM5030_busy(void);
void VLM5030_w (int offset,int data);

#endif

