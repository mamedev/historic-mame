/*************************************************************************

	Atari Video Pinball hardware

*************************************************************************/

/* Discrete Sound Input Nodes */
#define VIDEOPIN_OCTAVE_DATA	NODE_01
#define VIDEOPIN_NOTE_DATA		NODE_02
#define VIDEOPIN_BELL_EN		NODE_03
#define VIDEOPIN_BONG_EN		NODE_04
#define VIDEOPIN_ATTRACT_EN		NODE_05
#define VIDEOPIN_VOL_DATA		NODE_06


/*----------- defined in sndhrdw/videopin.c -----------*/

extern struct discrete_sound_block videopin_discrete_interface[];
