#ifndef _C6280_H_
#define _C6280_H_

typedef struct {
    UINT16 frequency;
    UINT8 control;
    UINT8 balance;
    UINT8 waveform[32];
    UINT8 index;
    INT16 dda;
    UINT8 noise_control;
    UINT32 noise_counter;
    UINT32 counter;
} t_channel;

typedef struct {
	sound_stream *stream;
    UINT8 select;
    UINT8 balance;
    UINT8 lfo_frequency;
    UINT8 lfo_control;
    t_channel channel[8];
    INT16 volume_table[32];
    UINT32 noise_freq_tab[32];
    UINT32 wave_freq_tab[4096];
} c6280_t;

/* Function prototypes */
WRITE8_HANDLER( C6280_0_w );
WRITE8_HANDLER( C6280_1_w );

#endif /* _C6280_H_ */
