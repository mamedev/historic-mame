#ifndef MIXER_H
#define MIXER_H

#define MIXER_MAX_CHANNELS 16

/*
  When you allocate a channel, you pass a default mixing level setting.
  The mixing level is in the range 0-100, and is passed down to the OS dependant
  code. A channel playing at 100% has full access to the complete dynamic range
  of the sound card. When more than one channel is playing, clipping may occur
  so the levels have to decreased to avoid that, and adjusted to get the correct
  balance.

  By default, channels play on both speakers. They can also be directed to only
  one speaker. Note that in that case the sound will be perceived by the
  listener at half intensity, since it is coming from only one speaker.
  Use the MIXER() macro to select which speaker the channel should go to. E.g.
  mixer_allocate_channel(MIXER(50,MIXER_PAN_LEFT));

  The MIXER() macro uses 16 bits because the YM3012_VOL() macro stuffs two
  MIXER() values for left and right channel into a long.
*/
#define MIXER_PAN_CENTER  0
#define MIXER_PAN_LEFT    1
#define MIXER_PAN_RIGHT   2
#define MIXER(level,pan) ((level & 0xff) | ((pan & 0x03) << 8))

#define MIXER_GAIN_1x  0
#define MIXER_GAIN_2x  1
#define MIXER_GAIN_4x  2
#define MIXER_GAIN_8x  3
#define MIXERG(level,gain,pan) ((level & 0xff) | ((gain & 0x03) << 10) | ((pan & 0x03) << 8))

#define MIXER_GET_LEVEL(mixing_level)  ((mixing_level) & 0xff)
#define MIXER_GET_PAN(mixing_level)    (((mixing_level) >> 8) & 0x03)
#define MIXER_GET_GAIN(mixing_level)   (((mixing_level) >> 10) & 0x03)


int mixer_sh_start(void);
void mixer_sh_stop(void);
void mixer_sh_update(void);
int mixer_allocate_channel(int default_mixing_level);
int mixer_allocate_channels(int channels,const int *default_mixing_levels);
void mixer_set_name(int channel,const char *name);
const char *mixer_get_name(int channel);

/*
  This function sets the volume of a channel. This is *NOT* the mixing level,
  which is a private value set only at startup.
  By default, all channels play at volume 100 (the maximum). If there is some
  external circuitry which can alter the volume of the sound source, you can
  use this function to emulate that. The resolution provided by this function
  is VERY LOW, lower than the 100 steps it allows. That's because it is
  scaled by the mixing level (which is typically around 25%) before being sent
  to the OS dependant code, and because the resolution provided by the OS
  dependant code is not guaranteed (for example, the DOS port only has 64
  levels).
  Therefore, this function should only be used for rough control of the output,
  or to just turn channels on and off. If precise control is needed, it should
  be implemented in the stream generation.
*/
void mixer_set_volume(int channel,int volume);

void mixer_play_sample(int channel,signed char *data,int len,int freq,int loop);
void mixer_play_sample_16(int channel,INT16 *data,int len,int freq,int loop);
void mixer_stop_sample(int channel);
int mixer_is_sample_playing(int channel);
void mixer_set_sample_frequency(int channel,int freq);

void mixer_play_streamed_sample_16(int channel,INT16 *data,int len,int freq);

/* private functions for user interface only - don't call them from drivers! */
void mixer_set_mixing_level(int channel,int level);
int mixer_get_mixing_level(int level);

void mixer_read_config(void *f);
void mixer_write_config(void *f);
#endif
