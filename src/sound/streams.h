#ifndef STREAMS_H
#define STREAMS_H

#define MAX_STREAM_CHANNELS 16

void set_RC_filter(int channel,int R1,int R2,int R3,int C);

int streams_sh_start(void);
void streams_sh_stop(void);
void streams_sh_update(void);

int stream_init(const struct MachineSound *msound,
		const char *name,int sample_rate,int sample_bits,
		int param,void (*callback)(int param,void *buffer,int length));
int stream_init_multi(const struct MachineSound *msound,
		int channels,const char **name,int sample_rate,int sample_bits,
		int param,void (*callback)(int param,void **buffer,int length));
void stream_update(int channel,int min_interval);	/* min_interval is in usec */
void stream_set_volume(int channel,int volume);
int stream_get_volume(int channel);
void stream_set_pan(int channel,int pan);
int stream_get_pan(int channel);
const char *stream_get_name(int channel);

void stream_set_name(int channel,const char *name);
int stream_get_sample_bits(int channel);
int stream_get_sample_rate(int channel);
void *stream_get_buffer(int channel);
int stream_get_buffer_len(int channel);

#endif
