/*
 * X-Mame system independent sound code
 */

#define __SOUND_C_
#include "xmame.h"

static int MasterVolume=100;

#define AUDIO_BUFF_SIZE 131072

int start_timer() {
	struct itimerval	timer_value ;
	void audio_timer(int arg);
	/* set and configure timer catcher */
	sig_action.sa_handler = audio_timer;
#ifdef hpux
	sig_action.sa_flags   = SA_RESETHAND;
#else
	sig_action.sa_flags   = SA_RESTART;
#endif

	if(sigaction (SIGALRM, &sig_action, NULL)< 0) {
		fprintf(stderr,"Sigaction Failed \n");
		return FALSE;
	}
	/* set and configure realtime alarm */
  	timer_value.it_interval.tv_sec =
 	timer_value.it_value.tv_sec    = 0L;
  	timer_value.it_interval.tv_usec=
  	timer_value.it_value.tv_usec   = 1000000L / audio_timer_freq;

	if (setitimer (ITIMER_REAL, &timer_value, NULL)) {
 		printf ("Setting the timer failed.\n");
  		return (FALSE);
  	}
	return (TRUE);
}

void audio_timer (int arg) {
        static int  in = FALSE;
        int   i, j;
        static byte buf[AUDIO_BUFF_SIZE];
        static long intbuf[AUDIO_BUFF_SIZE];
        int   j1, j2;

        if (!in) {
                in = TRUE;
                j1 = abuf_ptr;  /* calc pointers for chunk of buffer */
                j2 = j1 + abuf_inc; /* to be filled this time around */
                for (i = 0; i < AUDIO_NUM_VOICES; i++) {
                    if ( (audio_on[i]) && ( audio_len[i]) ) {
                        for (j = j1; j < j2; j++) {
                            if (i != 0) intbuf[j] += (audio_vol[i]/16) * 
                                        audio_data[i][((j * audio_freq[i]) / 
                                        audio_sample_freq) % audio_len[i]];
                            else        intbuf[j] = (audio_vol[0]/16) * 
                                        audio_data[0][((j * audio_freq[0]) / 
                                        audio_sample_freq) % audio_len[0]];
                            }
                        audio_dvol[i]  = FALSE;
                        audio_dfreq[i] = FALSE;
                    }
                }
		sysdep_fill_audio_buffer( intbuf,(char *)buf,j1,j2);
                if (j2 == abuf_size) {  /* buffer full - dump to audio */
			i=sysdep_play_audio(buf,abuf_size);
                        abuf_ptr = 0;    /* and reset */
                } else {
                        abuf_ptr = j2;   /* not full - write more next time */
                }
        in = FALSE;
        } /* else {
          fprintf(stderr, ".");
        } */
#if defined solaris || defined sgi
	/* rearm alarm timer */
	sigaction (SIGALRM, &sig_action, NULL);
#endif
}

/*
 * Audio shit.
 */

int sysdep_audio_initvars(void)
{
	audio_sample_freq = AUDIO_SAMPLE_FREQ;
	audio_timer_freq  = AUDIO_TIMER_FREQ;
	return (0);
}

void osd_update_audio (void)
{
	/* Not used. */
}

void osd_set_mastervolume(int volume)
{
	int channel;
	MasterVolume = volume;
#if 0
	/* not implemented yet */
	for (channel=0; channel < AUDIO_NUM_VOICES; channel++) {
		audio_vol[channel] *= (MasterVolume/100) ;
	}
#endif
}

void osd_play_sample (int channel, byte *data, int len, int freq,
			int volume, int loop)
{
	if (!play_sound || (channel >= AUDIO_NUM_VOICES))
	{
		return;
 	}
	audio_on[channel]	= TRUE;
	audio_data[channel] 	= data;
	audio_len[channel]  	= len;
	audio_freq[channel]	= freq;
	audio_vol[channel]  	= volume;
}

void osd_play_streamed_sample (int channel, byte *data, int len, int freq, int volume)
{
	/* Not used. */
}

void osd_adjust_sample (int channel, int freq, int volume)
{
	if (play_sound && (channel < AUDIO_NUM_VOICES))
	{
		audio_dfreq[channel] |= (freq    != audio_freq[channel]);
		audio_dvol[channel]  |= (volume  != audio_vol[channel]);
		audio_freq[channel] = freq;
		audio_vol[channel]  = volume;
	}
}

void osd_stop_sample (int channel)
{
	if (play_sound && (channel < AUDIO_NUM_VOICES))
	{
		audio_on[channel] = FALSE;
	}
}
