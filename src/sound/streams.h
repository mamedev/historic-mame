#ifndef STREAMS_H
#define STREAMS_H

void set_RC_filter(int channel,int R1,int R2,int R3,int C);

int streams_sh_start(void);
void streams_sh_stop(void);
void streams_sh_update(void);

int stream_init(const char *name,int default_mixing_level,
		int sample_rate,int sample_bits,
		int param,void (*callback)(int param,void *buffer,int length));
int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		int sample_rate,int sample_bits,
		int param,void (*callback)(int param,void **buffer,int length));
void stream_update(int channel,int min_interval);	/* min_interval is in usec */
int stream_get_sample_bits(int channel);
int stream_get_sample_rate(int channel);
void *stream_get_buffer(int channel);
int stream_get_buffer_len(int channel);

#endif
