/* sound:yiear.c */

#include "driver.h"

/* A nice macro which saves us a lot of typing */
#define M_OSD_PLAY_SAMPLE(channel, soundnum, loop) { \
	if (Machine->samples->sample[soundnum] != 0) \
		osd_play_sample(channel, \
			Machine->samples->sample[soundnum]->data, \
			Machine->samples->sample[soundnum]->length, \
			Machine->samples->sample[soundnum]->smpfreq, \
			Machine->samples->sample[soundnum]->volume,loop); }


void yiear_audio_out_w( int offset, int val );

const char *yiear_sample_names[] = {
	"gong.sam","fall.sam","health.sam",
	"star.sam","nuncha.sam","chain.sam",
	"impact.sam","fan.sam","pole.sam","jump.sam",
	"intro.sam","stage1.sam","stage2.sam","panick.sam",
	"win.sam","lose.sam","gameover.sam","highscor.sam",
	"punch.sam","kick.sam","shout.sam","oof.sam",
	"buchu.sam","shisha.sam","perfect.sam",
	0
};

enum {
	gong = 0,fall,health,star,nuncha,chain,impact,fan,pole,jump,
	intro,stage1,stage2,panick,win,lose,gameover,highscore,
	punch,kick,shout,oof,buchu,shisha,perfect
};

unsigned char *yiear_soundport;

int yiear_sound1, yiear_sound2, yiear_timer1; /* simulate a simple queue */

int yiear_sh_start( void );
int yiear_sh_start( void ){
	yiear_timer1 = yiear_sound1 = yiear_sound2 = -1;
	return 0;
}

void yiear_sh_update( void );
void yiear_sh_update( void ){
	if( Machine->samples == 0 ) return;
	if( yiear_timer1>0 ){
		yiear_timer1--; /* wait for current sound to complete */
	}
	else if( yiear_sound1 >= 0 ){
		struct GameSample *sample = Machine->samples->sample[yiear_sound1];
		if (sample)
		{
			M_OSD_PLAY_SAMPLE (2, yiear_sound1, 0);
			yiear_timer1 = sample->length*60/sample->smpfreq; /* compute ticks until finished */
			yiear_sound1 = yiear_sound2;
			yiear_sound2 = -1;
		}
	}
}

void yiear_QueueSound( int which );
void yiear_QueueSound( int which ){
	if( yiear_sound1<0 ) yiear_sound1 = which;
	else if( yiear_sound2<0 ) yiear_sound2 = which;
}

void yiear_NoSound( void );
void yiear_NoSound( void ){
	osd_stop_sample(0); /* immediate sounds */
	osd_stop_sample(1);

	osd_stop_sample(2); /* queued sound */

	osd_stop_sample(3); /* music */

//	yiear_sh_init(0); /* clear queue */
}

void yiear_PlaySound( int which );
void yiear_PlaySound( int which ){
	static int chan = 0;

	M_OSD_PLAY_SAMPLE (chan, which, 0);
	chan = 1-chan;
}

void yiear_PlayMusic( int which, int loop ){
	M_OSD_PLAY_SAMPLE (3, which, loop);
}

void yiear_audio_out_w( int offset, int val ){
	yiear_soundport[offset] = val; /* write so RAM test doesn't fail */

	if( Machine->samples == 0 ) return;

	switch( val ){
		case 0:		yiear_NoSound(); break;

		/* junk - these are sent only during the RAM test
		case 85:
		case 170:
		case 255:
		case 87:
		break;
		*/

		case 1:		yiear_QueueSound( gong ); break;
		case 4:		yiear_QueueSound( fall ); break;
		case 5:		yiear_PlaySound( health ); break;
		case 6:		yiear_PlaySound( star ); break;
		case 7:		yiear_PlaySound( nuncha ); break;
		case 8:		yiear_PlaySound( chain ); break;
		case 9:		yiear_PlaySound( impact ); break;
		case 10:	yiear_PlaySound( fan ); break;
		case 11:	yiear_PlaySound( pole ); break;
		case 12:	yiear_PlaySound( jump ); break;

		case 64:	yiear_PlayMusic( intro,0 ); break;
		case 65:	yiear_PlayMusic( stage1,1 ); break;
		case 66:	yiear_PlayMusic( stage2,1 ); break;
		case 67:	yiear_PlayMusic( panick,1 ); break;
		case 68:	yiear_PlayMusic( win,0 ); break;
		case 69:	yiear_PlayMusic( lose,0 ); break;
		case 70:	yiear_PlayMusic( gameover,0 ); break;
		case 71:	yiear_PlayMusic( highscore,1 ); break;

		case 128:	yiear_PlaySound( punch ); break;
		case 129:	yiear_PlaySound( kick ); break;
		case 131:	yiear_PlaySound( shout ); break;
		case 133:	yiear_PlaySound( oof ); break;
		case 135:	yiear_QueueSound( buchu ); break;
		case 136:	yiear_QueueSound( shisha ); break;

		case 153:	yiear_QueueSound( perfect ); break;
	}
}

