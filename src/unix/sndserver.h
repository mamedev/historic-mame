/* 
 * Sound structures used by sound server
 */

#ifndef __SNDSERVER_H_
#define __SNDSERVER_H_

#ifndef __SNDSERVER_C_
#define EXTERN	extern
#else 
#define EXTERN
#endif

/* number of maximun available channels  and sample structs */
#define MAX_CHANNELS	16
#define MAX_SAMPLES	32

/* default port to start server listening on */
#define DEFAULT_PORT	"3456"

/* maximum data packet size */
#define MAX_DATA_SIZE	512

/* available status of each audio channel */
typedef enum PMode { 
		OFF,		/* disabled, no play at all 		*/
		ON,		/* playable. stop at end of buffer 	*/
		LOOP,		/* replay when end of buffer reached	*/
		RAW 		/* voice data: free samplebuff when done*/
		} pmode;

/* list of available command parameters over network link */
typedef enum NETOPS { 
		NOP,		/* like areyouthere?. must ACK reply 	*/ 
		ACK,		/* reply message from server		*/
		NACK,		/* reply message from server		*/
		BEGIN,		/* initialize sound system		*/ 
		END, 		/* close sound system and exit		*/
		START, 		/* reset sound system			*/
		STOP,		/* stop playing of specific channel     */ 
		PLAY, 		/* play samples over a given channel    */
		PLAY_STREAMED,  /* syncronous play over a channel	*/
		SYNC,		/* flush and empty audio buffers	*/
		SET_VOL, 	/* set master volume control		*/
		ADJUST 		/* select vol & frec of a given channel */
		} netops;

/* data sample structure definition */
typedef struct Ss_Sample {
	int len;		/* buffer length			*/
	int channel;		/* -1:empty else: channel asigned to	*/
	char *buffer;		/* pointer to malloc'd sample buffer 	*/
} ss_sample;

/* structure of an audio channel */
typedef struct Ss_Channel {
	int chanel_number;	/* not really needed, but...		*/
	pmode mode;		/* status of this voice channel		*/
	int frec;		/* frecuency of sample data		*/
	int volumen;		/* volume of sample data		*/
	char *pt;		/* next sample to send to mixer		*/
	int d_count;		/* audio_frec/channel_frec ratio	*/
	int d_pt;		/* used by mixer to interpolate sample  */
	ss_sample *s_pt; 	/* pointer to the sample struct ....	*/
} ss_channel;

/* structure received/sent  from network */
typedef struct Net_Data {
	int 		msg_id;		/* message indentifier		*/
	netops		netops;		/* message type 		*/
	int 		channel;	/* channel to use ( if any )	*/
	int		frec;		/* channel frecuency		*/	
	int 		volume;		/* channel or master volume	*/
	pmode 		playmode;	/* mode used in play command	*/
	int		length;		/* sample length (play command) */
	int		offset;		/* data sample offset (play)    */	
	char		data[4];	/* data region; expandable	*/
} network_data;

/* global variables */
EXTERN ss_sample 	sample[MAX_SAMPLES];
EXTERN ss_channel 	channel[MAX_CHANNELS];
EXTERN network_data	*dummy_msg;

/* Network related function prototypes */
/* 
 * returns pt 		on success
 *         NULL		on fail
 *	   dummy_msg 	on success with reply
 */
EXTERN	network_data  *nop_func( network_data *pt);
EXTERN	network_data  *ack_func( network_data *pt);
EXTERN	network_data  *nack_func( network_data *pt);
EXTERN	network_data  *begin_func( network_data *pt);
EXTERN	network_data  *end_func( network_data *pt);
EXTERN	network_data  *start_func( network_data *pt);
EXTERN	network_data  *stop_func( network_data *pt);
EXTERN	network_data  *play_func( network_data *pt);
EXTERN	network_data  *play_streamed_func( network_data *pt);
EXTERN	network_data  *sync_func( network_data *pt);
EXTERN	network_data  *set_vol_func( network_data *pt);
EXTERN	network_data  *adjust_func( network_data *pt);
	
#undef EXTERN

#endif

