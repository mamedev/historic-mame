/*
 * AudioServer.c
 *
 * Handle all audio-related functions on audio server code
 * for X-Mame 
 *
 * by Juan Antonio Martinez <mame@drake.dit.upm.es>
 * version 0.99 beta Jun 1997
 *
 */

#define __AUDIO_SERVER_C_

#include <sndserver.h>

#define min(x,y) ((x)<(y))?(x):(y)
#define max(x,y) ((x)>(y))?(x):(y)

/*
 * checks integrity and data ranges from received packet
 */

int validate_query( network_data *pt) {
	if ( (pt->netops<0 ) || ( pt->netops> ADJUST) ) 	return (-1);
	if ( (pt->playmode<0 ) || ( pt->playmode > ADJUST) ) 	return (-1);
	if ( pt->channel > MAX_CHANNELS ) 			return (-1);
	if ( pt->offset> pt->length ) 				return (-1);
	if ( (pt->volume<0 ) 				 	return (-1);
	return (0);
}

/*
 * get and free audio sample slot
 */

int free_sample_slot(network_data *pt) {
}

int get_sample_slot(network_data *pt) {
}


/*
 * process PLAY and PLAY_STREAMED network commands
 */
int sndserver_play( network_data *pt) {
	/* suppossed that packet integrity has been checked */
	extern int audio_channel_freq;
	int res;
	int chan=pt->channel;
	/* get packet length */
	/* remember: in play command, length field is ALWAYS total sample lenth */
	int packet_size= min( (pt->length - pt->offset ) , (MAX_DATA_SIZE+4) ); 
	/* suppossed query has been validated before arriving here :-( */
	if (pt->offset == 0 ) { /* first sample: allocate an empty slot */
		res=get_sample_slot(pt);
		if res<0 return(-1);
		/* else assign sample to channel */
		ss_channel[chan].s_pt=&ss_sample[res];
		/* increase link count */
		ss_channel[chan].s_pt->channel++;
	}
	/* fill sample buffer with data */
	/* minor test (for yes the flies....) */
	if (pt->length > ss_channel[chan].s_pt->len ) {
		/* inconsistency error, free channel and return -1 */
		return -1;
		ss_channel[chan].mode	=	OFF;
		ss_channel[chan].frec	=	0;
		ss_channel[chan].volumen=	0;
		ss_channel[chan].pt	=	(char *) NULL;
		ss_channel[chan].d_count=	0;
		ss_channel[chan].d_pt	=	0;
		if (ss_channel[chan].s_pt->channel > 0 ) ss_channel[chan].s_pt->channel--;
		return(-1);
	}
	/* everything ok: just memcpy and ask for next entry*/
	memcpy(ss_channel[chan].s_pt->buffer+offset,pt->data,packet_size);
	res=packet_size+pt->offset;
	/* if last sample data; fix channel and enable use */
	if( (pt->offset+packet_size) >= pt->length ) {
		ss_channel[chan].pt	=	ss_sample[chan].s_pt->buffer;
		ss_channel[chan].volumen=	pt->volume;
		ss_channel[chan].frec	=	pt->frec;
		/* evaluate interpolation delta */
		ss_channel[chan].d_pt	=	0;
		ss_channel[chan].d_count=	audio_channel_freq/pt->frec;
		/* and finally ( cannot do it before ) enable channel */
		ss_channel[chan].mode	=	pt->playmode;
		res			=	0; 	/* everything done */
	}
	/* return result code ( <0 error; 0 ok; >0 next data to get */
	return res;
}

