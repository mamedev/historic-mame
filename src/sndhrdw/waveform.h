#ifndef waveform_h
#define waveform_h

struct waveform_interface
{
   int samplerate;      /* sample rate */
   int voices;          /* number of voices */
   int gain;            /* 16 * gain adjustment */
	int volume;          /* playback volume */
};

int waveform_sh_start(struct waveform_interface *intf);
void waveform_sh_stop(void);
void waveform_sh_update(void);

int pengo_sh_start(void);
void pengo_sound_enable_w(int offset,int data);
void pengo_sound_w(int offset,int data);

int mappy_sh_start(void);
void mappy_sound_enable_w(int offset,int data);
void mappy_sound_w(int offset,int data);


#endif

