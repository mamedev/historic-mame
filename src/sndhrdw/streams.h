#ifndef STREAMS_H
#define STREAMS_H

void set_RC_filter(int channel,int R1,int R2,int R3,int C);

int streams_sh_start(void);
void streams_sh_stop(void);
void streams_sh_update(void);

int stream_init(int sample_rate,int sample_bits,
		int param,void (*callback)(int param,void *buffer,int length));
void stream_set_volume(int channel,int volume);
void stream_update(int channel);

#endif
