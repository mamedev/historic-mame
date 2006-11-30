/*************************************************************************

    laserdsc.c

    Generic laserdisc support.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*************************************************************************/

#include "driver.h"
#include "laserdsc.h"



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define PRINTF_COMMANDS				1

#if PRINTF_COMMANDS
#define CMDPRINTF(x)				mame_printf_debug x
#else
#define CMDPRINTF(x)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* fractional frame handling */
#define FRACBITS                    12
#define FRAC_ONE                    (1 << FRACBITS)
#define INT_TO_FRAC(x)              ((x) << FRACBITS)
#define FRAC_TO_INT(x)              ((x) >> FRACBITS)

/* general laserdisc states */
#define LASERDISC_PARKED			0			/* parked with head at 0 */
#define LASERDISC_PLAYING			1			/* actively playing at normal rate */
#define LASERDISC_STOPPED			2			/* stopped (paused) with video visible and no sound */
#define LASERDISC_SEARCHING			3			/* searching (seeking) to a new location */
#define LASERDISC_SEARCH_FINISHED	4			/* finished searching; same as stopped */
#define LASERDISC_AUTOSTOPPED		5			/* autostopped; same as stopped */
#define LASERDISC_SCANNING			6			/* scanning forward/reverse at the scan rate */
#define LASERDISC_SLOW_FORWARD      7           /* playing forward slowly */
#define LASERDISC_SLOW_REVERSE      8           /* playing backward slowly */
#define LASERDISC_PLAYING_ALTSPEED	9			/* actively playing at an alternate rate */
#define LASERDISC_LOADING			10			/* "loading" the disc */
#define LASERDISC_LOAD_FINISHED		11			/* finished loading the disc */
#define LASERDISC_REJECTING			12			/* "rejecting" the disc */

/* Pioneer PR-7820 specific states */
#define PR7820_MODE_MANUAL          0
#define PR7820_MODE_AUTOMATIC       1
#define PR7820_MODE_PROGRAM         2

/* Pioneer LD-V1000 specific states */
#define LDV1000_MODE_STATUS			0
#define LDV1000_MODE_GET_FRAME		1
#define LDV1000_MODE_GET_1ST		2
#define LDV1000_MODE_GET_2ND		3
#define LDV1000_MODE_GET_RAM		4



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _laserdisc_state
{
	/* disc parameters */
	chd_file *      disc;                   /* handle to the disc itself */
	UINT32          maxfracframe;           /* maximum frame number */

	/* core states */
	UINT8			type;					/* laserdisc type */
	UINT8			state;					/* current player state */
	UINT8           display;                /* display state: TRUE if frame display is on */
	UINT8           audio;                  /* audio state: bit 0 = audio 1, bit 1 = audio 2 */
	mame_time       lastvsynctime;          /* time of the last vsync */

	/* loading states */
	mame_time		holdfinished;			/* time when current state will advance */
	UINT8			postholdstate;			/* state to switch into after holding */

	/* input data */
	UINT8			datain;					/* current input data value */
	UINT8			linein[LASERDISC_INPUT_LINES]; /* current input line state */

	/* output data */
	UINT8			dataout;				/* current output data value */
	UINT8			lineout[LASERDISC_OUTPUT_LINES]; /* current output line state */

	/* command and parameter buffering */
	UINT8			command;				/* current command */
	INT32			parameter;				/* command parameter */

	/* playback/search/scan speeds */
	INT32			altspeed;				/* alternate speed in fractional frames */
	INT32			slowspeed;				/* slow speed in fractional frames */
	INT32			scanspeed;				/* scan speed in fractional frames */
	INT32			searchspeed;			/* search speed in fractional frames */

	/* frame numbers */
	INT32			curfracframe;			/* current frame */
	INT32			searchfracframe;		/* searching frame */
	INT32			autostopfracframe;		/* autostop frame */

	/* debugging */
	char			text[100];				/* buffer for the state */

	/* filled in by player-specific init */
	void 			(*writedata)(laserdisc_state *state, UINT8 prev, UINT8 new); /* write callback */
	void 			(*writeline[LASERDISC_INPUT_LINES])(laserdisc_state *state, UINT8 new); /* write line callback */
	UINT8			(*readdata)(laserdisc_state *state);	/* status callback */
	UINT8			(*readline[LASERDISC_OUTPUT_LINES])(laserdisc_state *state); /* read line callback */

	/* some player-specific data */
	UINT8           mode;		            /* PR7820: current mode */
	UINT32          curpc;                  /* PR7820: current PC for automatic execution */
	UINT32          activereg;              /* PR7820, LDV1000: active register index */
	UINT8			ram[1024];				/* PR7820, LDV1000: RAM */
	UINT32			readpos;				/* LDV1000: current read position */
	UINT8			readbuf[256];			/* LDV1000: temporary read buffer */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void pr7820_init(laserdisc_state *state);
static void pr7820_soft_reset(laserdisc_state *state);
static void pr7820_enter_w(laserdisc_state *state, UINT8 data);
static UINT8 pr7820_ready_r(laserdisc_state *state);
static UINT8 pr7820_status_r(laserdisc_state *state);

static void ldv1000_init(laserdisc_state *state);
static void ldv1000_soft_reset(laserdisc_state *state);
static void ldv1000_data_w(laserdisc_state *state, UINT8 prev, UINT8 data);
static UINT8 ldv1000_status_strobe_r(laserdisc_state *state);
static UINT8 ldv1000_command_strobe_r(laserdisc_state *state);
static UINT8 ldv1000_status_r(laserdisc_state *state);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    add_to_current_frame - add a value to the
    current frame, stopping if we hit the min or
    max
-------------------------------------------------*/

INLINE int add_to_current_frame(laserdisc_state *state, INT32 delta)
{
	state->curfracframe += delta;
	if (state->curfracframe < INT_TO_FRAC(1))
	{
	    state->curfracframe = INT_TO_FRAC(1);
	    return TRUE;
	}
	else if (state->curfracframe >= state->maxfracframe)
	{
	    state->curfracframe = state->maxfracframe - INT_TO_FRAC(1);
	    return TRUE;
	}
	return FALSE;
}


/*-------------------------------------------------
    read_16bits_from_ram_be - read 16 bits from
    player RAM in big-endian format
-------------------------------------------------*/

INLINE UINT16 read_16bits_from_ram_be(laserdisc_state *state, UINT32 offset)
{
	return (state->ram[offset + 0] << 8) | state->ram[offset + 1];
}


/*-------------------------------------------------
    write_16bits_to_ram_be - write 16 bits to
    player RAM in big endian format
-------------------------------------------------*/

INLINE void write_16bits_to_ram_be(laserdisc_state *state, UINT32 offset, UINT16 data)
{
	state->ram[offset + 0] = data >> 8;
	state->ram[offset + 1] = data >> 0;
}



/***************************************************************************
    GENERIC IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    laserdisc_init - initialize state for
    laserdisc playback
-------------------------------------------------*/

laserdisc_state *laserdisc_init(int type)
{
	laserdisc_state *state;
	int i;

	/* initialize the state */
	state = auto_malloc(sizeof(*state));
	memset(state, 0, sizeof(*state));

	/* set up the general state */
	state->type = type;
	state->state = LASERDISC_PARKED;
	state->display = FALSE;
	state->audio = 0x03;

	for (i = 0; i < LASERDISC_INPUT_LINES; i++)
		state->linein[i] = CLEAR_LINE;
	for (i = 0; i < LASERDISC_OUTPUT_LINES; i++)
		state->lineout[i] = CLEAR_LINE;

	/* set up the disc state */
	state->maxfracframe = INT_TO_FRAC(100000);

	/* each player can init */
	switch (type)
	{
		case LASERDISC_TYPE_PR7820:
			pr7820_init(state);
			break;

		case LASERDISC_TYPE_LDV1000:
			ldv1000_init(state);
			break;

		default:
			fatalerror("Invalid laserdisc player type!\n");
			break;
	}

	/* default to frame 1 */
	state->curfracframe = INT_TO_FRAC(1);
	return state;
}


/*-------------------------------------------------
    laserdisc_vsync - call this once per frame
    on the VSYNC signal
-------------------------------------------------*/

void laserdisc_vsync(laserdisc_state *state)
{
	INT32 delta;

	/* remember the time */
	state->lastvsynctime = mame_timer_get_time();

	/* if we're holding, stay in this state until finished */
	if (state->holdfinished.seconds != 0 || state->holdfinished.subseconds != 0)
	{
		if (compare_mame_times(mame_timer_get_time(), state->holdfinished) < 0)
		    return;
		state->state = state->postholdstate;
		state->holdfinished.seconds = 0;
		state->holdfinished.subseconds = 0;
	}

	/* switch off the state */
	switch (state->state)
	{
		/* parked: do nothing */
		case LASERDISC_PARKED:
			sprintf(state->text, "Parked");
			break;

		/* playing: increment to the next frame */
		case LASERDISC_PLAYING:

			/* advance by one frame */
			add_to_current_frame(state, INT_TO_FRAC(1));

			/* autostop if we hit the autostop frame */
			if (FRAC_TO_INT(state->curfracframe) == FRAC_TO_INT(state->autostopfracframe))
				state->state = LASERDISC_AUTOSTOPPED;

			sprintf(state->text, "Playing: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* stopped: do nothing */
		case LASERDISC_STOPPED:
			sprintf(state->text, "Stopped: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* searching; keep seeking until we hit the target */
		case LASERDISC_SEARCHING:

			/* seek in the appropriate direction toward the searchfracframe */
			if (state->curfracframe < state->searchfracframe)
				delta = MIN(state->searchspeed, state->searchfracframe - state->curfracframe);
			else
				delta = -MIN(state->searchspeed, state->curfracframe - state->searchfracframe);
			add_to_current_frame(state, delta);

			/* if we hit the target, go into search finished state */
			if (state->curfracframe == state->searchfracframe)
				state->state = LASERDISC_SEARCH_FINISHED;
			sprintf(state->text, "Searching: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* search finished: do nothing */
		case LASERDISC_SEARCH_FINISHED:
			sprintf(state->text, "Search Finished: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* autostopped: do nothing */
		case LASERDISC_AUTOSTOPPED:
			sprintf(state->text, "Autostopped: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* scanning: advance by the requested amount */
		case LASERDISC_SCANNING:

			/* apply the scan direction */
			add_to_current_frame(state, state->scanspeed);
			sprintf(state->text, "Scanning: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* slow forward: play at the slow speed */
		case LASERDISC_SLOW_FORWARD:

			/* advance by the alternate playback speed */
			add_to_current_frame(state, state->slowspeed);

			/* autostop if we hit the autostop frame */
			if (FRAC_TO_INT(state->curfracframe) == FRAC_TO_INT(state->autostopfracframe))
				state->state = LASERDISC_AUTOSTOPPED;

			sprintf(state->text, "SlowFwd: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* slow reverse: play at the slow speed */
		case LASERDISC_SLOW_REVERSE:

			/* advance by the alternate playback speed */
			add_to_current_frame(state, -state->slowspeed);

			/* autostop if we hit the autostop frame */
			if (FRAC_TO_INT(state->curfracframe) == FRAC_TO_INT(state->autostopfracframe))
				state->state = LASERDISC_AUTOSTOPPED;

			sprintf(state->text, "SlowRev: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* playing: increment to the next frame */
		case LASERDISC_PLAYING_ALTSPEED:

			/* advance by the alternate playback speed */
			add_to_current_frame(state, state->altspeed);

			/* autostop if we hit the autostop frame */
			if (FRAC_TO_INT(state->curfracframe) == FRAC_TO_INT(state->autostopfracframe))
				state->state = LASERDISC_AUTOSTOPPED;

			sprintf(state->text, "AltSpeed: %05d", FRAC_TO_INT(state->curfracframe));
			break;

		/* loading: do nothing */
		case LASERDISC_LOADING:
			sprintf(state->text, "Loading");
			break;

		/* load finished: do nothing */
		case LASERDISC_LOAD_FINISHED:
			sprintf(state->text, "Loaded");
			break;
	}
}


/*-------------------------------------------------
    laserdisc_reset - reset the laserdisc;
    generally this causes the disc to be ejected
-------------------------------------------------*/

void laserdisc_reset(laserdisc_state *state)
{
	state->state = LASERDISC_PARKED;
	state->holdfinished.seconds = 0;
	state->holdfinished.subseconds = 0;
}


/*-------------------------------------------------
    laserdisc_describe_state - return a text
    string describing the current state
-------------------------------------------------*/

const char *laserdisc_describe_state(laserdisc_state *state)
{
	return state->text;
}


/*-------------------------------------------------
    laserdisc_data_w - write data to the given
    laserdisc player
-------------------------------------------------*/

void laserdisc_data_w(laserdisc_state *state, UINT8 data)
{
	UINT8 prev = state->datain;
	state->datain = data;

	/* call through to the player-specific write handler */
	if (state->writedata != NULL)
		(*state->writedata)(state, prev, data);
}


/*-------------------------------------------------
    laserdisc_line_w - control an input line
-------------------------------------------------*/

void laserdisc_line_w(laserdisc_state *state, UINT8 line, UINT8 newstate)
{
	assert(line < LASERDISC_INPUT_LINES);
	assert(newstate == ASSERT_LINE || newstate == CLEAR_LINE || newstate == PULSE_LINE);

	/* assert */
	if (newstate == ASSERT_LINE || newstate == PULSE_LINE)
	{
		if (state->linein[line] != ASSERT_LINE)
		{
			/* call through to the player-specific line handler */
			if (state->writeline[line] != NULL)
				(*state->writeline[line])(state, ASSERT_LINE);
		}
		state->linein[line] = ASSERT_LINE;
	}

	/* deassert */
	if (newstate == CLEAR_LINE || newstate == PULSE_LINE)
	{
		if (state->linein[line] != CLEAR_LINE)
		{
			/* call through to the player-specific line handler */
			if (state->writeline[line] != NULL)
				(*state->writeline[line])(state, CLEAR_LINE);
		}
		state->linein[line] = CLEAR_LINE;
	}
}


/*-------------------------------------------------
    laserdisc_data_r - return the current
    data byte
-------------------------------------------------*/

UINT8 laserdisc_data_r(laserdisc_state *state)
{
	UINT8 result = state->dataout;

	/* call through to the player-specific data handler */
	if (state->readdata != NULL)
		result = (*state->readdata)(state);

	return result;
}


/*-------------------------------------------------
    laserdisc_line_r - return the current state
    of an output line
-------------------------------------------------*/

UINT8 laserdisc_line_r(laserdisc_state *state, UINT8 line)
{
	UINT8 result;

	assert(line < LASERDISC_OUTPUT_LINES);

	result = state->lineout[line];

	/* call through to the player-specific data handler */
	if (state->readline[line] != NULL)
		result = (*state->readline[line])(state);

	return result;
}



/***************************************************************************
    GENERIC HELPER FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    process_number - scan a list of bytecodes
    and treat them as single digits if we get
    a match
-------------------------------------------------*/

static int process_number(laserdisc_state *state, UINT8 byte, const UINT8 numbers[])
{
	int value;

	/* look for a match in the list of number values; if we got one, append it to the parameter */
	for (value = 0; value < 10; value++)
	    if (numbers[value] == byte)
	    {
	        state->parameter = (state->parameter == -1) ? value : (state->parameter * 10 + value);
	        return TRUE;
		}

	/* no match; return FALSE */
	return FALSE;
}



/***************************************************************************
    PIONEER PR-7820 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    pr7820_init - Pioneer PR-7820-specific
    initialization
-------------------------------------------------*/

static void pr7820_init(laserdisc_state *state)
{
	/* set up the write callbacks */
	state->writeline[LASERDISC_LINE_ENTER] = pr7820_enter_w;

	/* set up the read callbacks */
	state->readdata = pr7820_status_r;
	state->readline[LASERDISC_LINE_READY] = pr7820_ready_r;

	/* configure the default speeds */
	state->slowspeed = INT_TO_FRAC(1) / 2;
	state->searchspeed = INT_TO_FRAC(1000);

	/* do a soft reset */
	pr7820_soft_reset(state);
}


/*-------------------------------------------------
    pr7820_soft_reset - Pioneer PR-7820-specific
    soft reset
-------------------------------------------------*/

static void pr7820_soft_reset(laserdisc_state *state)
{
	state->audio = 0x03;
	state->display = FALSE;
	state->mode = PR7820_MODE_MANUAL;
	state->activereg = 0;
	write_16bits_to_ram_be(state, 0, 1);
}


/*-------------------------------------------------
    pr7820_enter_w - write callback when the
    ENTER state is asserted
-------------------------------------------------*/

static void pr7820_enter_w(laserdisc_state *state, UINT8 newstate)
{
	static const UINT8 numbers[10] = { 0x3f, 0x0f, 0x8f, 0x4f, 0x2f, 0xaf, 0x6f, 0x1f, 0x9f, 0x5f };
	UINT8 data = (state->mode == PR7820_MODE_AUTOMATIC) ? state->ram[state->curpc++ % 1024] : state->datain;

	/* we only care about assertions */
	if (newstate != ASSERT_LINE)
	    return;

	/* if we're in program mode, just write data */
	if (state->mode == PR7820_MODE_PROGRAM && data != 0xef)
	{
	    state->ram[state->parameter++ % 1024] = data;
	    return;
	}

	/* look for and process numbers */
	if (process_number(state, data, numbers))
	    return;

	/* handle commands */
	switch (data)
	{
		case 0x7f:  CMDPRINTF(("pr7820: %d Recall\n", state->parameter));
		    /* set the active register */
			state->activereg = state->parameter;
			break;

		case 0xa0:
		case 0xa1:
		case 0xa2:
		case 0xa3:	CMDPRINTF(("pr7820: Direct audio control %d\n", data & 0x03));
		    /* control both channels directly */
			state->audio = data & 0x03;
			break;

		case 0xbf:	CMDPRINTF(("pr7820: %d Halt\n", state->parameter));
		    /* stop automatic mode */
		    state->mode = PR7820_MODE_MANUAL;
			break;

		case 0xcc:	CMDPRINTF(("pr7820: Load\n"));
		    /* load program from disc -- not implemented */
			break;

		case 0xcf:	CMDPRINTF(("pr7820: %d Branch\n", state->parameter));
		    /* branch to a new PC */
		    if (state->mode == PR7820_MODE_AUTOMATIC)
			    state->curpc = (state->parameter == -1) ? 0 : state->parameter;
			break;

		case 0xdf:	CMDPRINTF(("pr7820: %d Write program\n", state->parameter));
		    /* enter program mode */
		    state->mode = PR7820_MODE_PROGRAM;
			break;

		case 0xe1:  CMDPRINTF(("pr7820: Soft Reset\n"));
		    /* soft reset */
			pr7820_soft_reset(state);
			break;

		case 0xe3:  CMDPRINTF(("pr7820: Display off\n"));
		    /* turn off frame display */
			state->display = FALSE;
			break;

		case 0xe4:  CMDPRINTF(("pr7820: Display on\n"));
		    /* turn on frame display */
			state->display = TRUE;
			break;

		case 0xe5:  CMDPRINTF(("pr7820: Audio 2 off\n"));
		    /* turn off audio channel 2 */
			state->audio &= ~0x02;
			break;

		case 0xe6:  CMDPRINTF(("pr7820: Audio 2 on\n"));
		    /* turn on audio channel 2 */
			state->audio |= 0x02;
			break;

		case 0xe7:  CMDPRINTF(("pr7820: Audio 1 off\n"));
		    /* turn off audio channel 1 */
			state->audio &= ~0x01;
			break;

		case 0xe8:  CMDPRINTF(("pr7820: Audio 1 on\n"));
		    /* turn on audio channel 1 */
			state->audio |= 0x01;
			break;

		case 0xe9:	CMDPRINTF(("pr7820: %d Dump RAM\n", state->parameter));
		    /* not implemented */
			break;

		case 0xea:	CMDPRINTF(("pr7820: %d Dump frame\n", state->parameter));
		    /* not implemented */
			break;

		case 0xeb:	CMDPRINTF(("pr7820: %d Dump player status\n", state->parameter));
		    /* not implemented */
			break;

		case 0xef:	CMDPRINTF(("pr7820: %d End program\n", state->parameter));
		    /* exit programming mode */
		    state->mode = PR7820_MODE_MANUAL;
			break;

		case 0xf0:	CMDPRINTF(("pr7820: %d Decrement reg\n", state->parameter));
		    /* decrement register; if we hit 0, skip past the next branch statement */
		    if (state->mode == PR7820_MODE_AUTOMATIC)
		    {
			    UINT16 tempreg = read_16bits_from_ram_be(state, (state->parameter * 2) % 1024);
			    tempreg = (tempreg == 0) ? 0 : tempreg - 1;
				write_16bits_to_ram_be(state, (state->parameter * 2) % 1024, tempreg);
				if (tempreg == 0)
				    while (state->ram[state->curpc++ % 1024] != 0xcf) ;
			}
			break;

		case 0xf1:  CMDPRINTF(("pr7820: %d Display\n", state->parameter));
		    /* toggle or set the frame display */
			state->display = (state->parameter == -1) ? !state->display : (state->parameter & 1);
			break;

		case 0xf2:	CMDPRINTF(("pr7820: %d Slow forward\n", state->parameter));
		    /* play forward at slow speed (controlled by lever on the front of the player) */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    if (state->parameter != -1)
			    state->autostopfracframe = INT_TO_FRAC(state->parameter);
			else
			    state->autostopfracframe = 0;
			state->state = LASERDISC_SLOW_FORWARD;
			break;

		case 0xf3:  CMDPRINTF(("pr7820: %d Autostop\n", state->parameter));
		    /* play to a particular location and stop there */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    if (state->parameter == -1)
			    state->autostopfracframe = INT_TO_FRAC(read_16bits_from_ram_be(state, (state->activereg * 2) % 1024));
			else
			    state->autostopfracframe = INT_TO_FRAC(state->parameter);
			state->activereg++;

			if (state->autostopfracframe > state->curfracframe)
				state->state = LASERDISC_PLAYING;
			else
			{
			    state->state = LASERDISC_SEARCHING;
			    state->searchfracframe = state->autostopfracframe;
   			}
			break;

		case 0xf4:  CMDPRINTF(("pr7820: %d Audio track 1\n", state->parameter));
		    /* toggle or set the state of audio channel 1 */
		    state->audio &= ~0x01;
			state->audio |= (state->parameter == -1) ? (~state->audio & 0x01) : ((state->parameter & 1) << 0);
			break;

		case 0xf5:	CMDPRINTF(("pr7820: %d Store\n", state->parameter));
		    /* store either the current frame number or an explicit value into the active register */
		    if (state->parameter == -1)
			    write_16bits_to_ram_be(state, (state->activereg * 2) % 1024, FRAC_TO_INT(state->curfracframe));
			else
			    write_16bits_to_ram_be(state, (state->activereg * 2) % 1024, state->parameter);
			state->activereg++;
			break;

		case 0xf6:	CMDPRINTF(("pr7820: Step forward\n"));
		    /* step forward one frame */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    state->state = LASERDISC_STOPPED;
		    state->curfracframe += INT_TO_FRAC(1);
			break;

		case 0xf7:  CMDPRINTF(("pr7820: %d Search\n", state->parameter));
		    /* search to a particular frame number */
		    if (state->state == LASERDISC_PARKED)
		        break;
			state->state = LASERDISC_SEARCHING;
		    if (state->parameter == -1)
			    state->searchfracframe = INT_TO_FRAC(read_16bits_from_ram_be(state, (state->activereg * 2) % 1024));
			else
			    state->searchfracframe = INT_TO_FRAC(state->parameter);
			state->activereg++;
			break;

		case 0xf8:	CMDPRINTF(("pr7820: %d Input\n", state->parameter));
		    /* wait for user input -- not implemented */
			break;

		case 0xf9:	CMDPRINTF(("pr7820: Reject\n"));
		    /* eject the disc */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    state->state = LASERDISC_REJECTING;
			state->holdfinished = add_mame_times(mame_timer_get_time(), double_to_mame_time(10.0));
			state->postholdstate = LASERDISC_PARKED;
			break;

		case 0xfa:	CMDPRINTF(("pr7820: %d Slow reverse\n", state->parameter));
		    /* play backwards at slow speed (controlled by lever on the front of the player) */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    if (state->parameter != -1)
			    state->autostopfracframe = INT_TO_FRAC(state->parameter);
			else
			    state->autostopfracframe = 0;
			state->state = LASERDISC_SLOW_REVERSE;
			break;

		case 0xfb:	CMDPRINTF(("pr7820: %d Stop/Wait\n", state->parameter));
		    /* pause at the current location for a fixed amount of time (in 1/10ths of a second) */
		    if (state->state == LASERDISC_PARKED)
		        break;
			state->postholdstate = state->state;
		    state->state = LASERDISC_STOPPED;
		    if (state->parameter != -1)
				state->holdfinished = add_mame_times(mame_timer_get_time(), double_to_mame_time(state->parameter * 0.1));
			break;

		case 0xfc:  CMDPRINTF(("pr7820: %d Audio track 2\n", state->parameter));
		    /* toggle or set the state of audio channel 2 */
		    state->audio &= ~0x02;
			state->audio |= (state->parameter == -1) ? (~state->audio & 0x02) : ((state->parameter & 1) << 1);
			break;

		case 0xfd:  CMDPRINTF(("pr7820: Play\n"));
		    /* begin playing at regular speed, or load the disc if it is parked */
			if (state->state == LASERDISC_PARKED)
			{
				state->state = LASERDISC_LOADING;
				state->holdfinished = add_mame_times(mame_timer_get_time(), double_to_mame_time(10.0));
				state->postholdstate = LASERDISC_PLAYING;
				state->curfracframe = INT_TO_FRAC(1);
			}
			else
				state->state = LASERDISC_PLAYING;
			break;

		case 0xfe:	CMDPRINTF(("pr7820: Step reverse\n"));
		    /* step backwards one frame */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    state->state = LASERDISC_STOPPED;
		    state->curfracframe -= INT_TO_FRAC(1);
			break;

		default:	CMDPRINTF(("pr7820: %d Unknown command %02X\n", state->parameter, data));
		    /* unknown command */
			break;
	}

	/* reset the parameter after executing a command */
	state->parameter = -1;
}


/*-------------------------------------------------
    pr7820_ready_r - return state of the ready
    line
-------------------------------------------------*/

static UINT8 pr7820_ready_r(laserdisc_state *state)
{
	return (state->state != LASERDISC_SEARCHING) ? ASSERT_LINE : CLEAR_LINE;
}


/*-------------------------------------------------
    pr7820_status_r - return state of the ready
    line
-------------------------------------------------*/

static UINT8 pr7820_status_r(laserdisc_state *state)
{
	/* top 3 bits reflect audio and display states */
	UINT8 status = (state->audio << 6) | (state->display << 5);

	/* low 5 bits reflect player states */
	switch (state->state)
	{
	    case LASERDISC_PARKED:      		status |= 0x01;     break;
	    case LASERDISC_PLAYING:      		status |= 0x02;     break;
	    case LASERDISC_STOPPED:      		status |= 0x03;     break;
	    case LASERDISC_SEARCHING:      		status |= 0x06;     break;
	    case LASERDISC_SEARCH_FINISHED:     status |= 0x07;     break;
	    case LASERDISC_AUTOSTOPPED:      	status |= 0x09;     break;
	    case LASERDISC_SLOW_FORWARD:	    status |= 0x04;     break;
	    case LASERDISC_SLOW_REVERSE:	    status |= 0x05;     break;
	    case LASERDISC_PLAYING_ALTSPEED:    status |= 0x01;     break;
	    case LASERDISC_LOADING:      		status |= 0x06;     break;
	    case LASERDISC_LOAD_FINISHED:      	status |= 0x01;     break;
	    case LASERDISC_REJECTING:      		status |= 0x0d;     break;
	    default:							assert(FALSE);		break;
	}
	return status;
}


/*-------------------------------------------------
    pr7820_set_slow_speed - set the speed of
    "slow" playback, which is controlled by a
    slider on the device
-------------------------------------------------*/

void pr7820_set_slow_speed(laserdisc_state *state, double frame_rate_scaler)
{
	state->slowspeed = INT_TO_FRAC(1) * frame_rate_scaler;
}



/***************************************************************************
    PIONEER LD-V1000 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ldv1000_init - Pioneer LDV-1000-specific
    initialization
-------------------------------------------------*/

static void ldv1000_init(laserdisc_state *state)
{
	/* set up the write callbacks */
	state->writedata = ldv1000_data_w;

	/* set up the read callbacks */
	state->readdata = ldv1000_status_r;
	state->readline[LASERDISC_LINE_STATUS] = ldv1000_status_strobe_r;
	state->readline[LASERDISC_LINE_COMMAND] = ldv1000_command_strobe_r;

	/* configure the default speeds */
	state->slowspeed = INT_TO_FRAC(1) / 2;
	state->searchspeed = INT_TO_FRAC(1000);

	/* do a soft reset */
	ldv1000_soft_reset(state);
}


/*-------------------------------------------------
    ldv1000_soft_reset - Pioneer LDV-1000-specific
    soft reset
-------------------------------------------------*/

static void ldv1000_soft_reset(laserdisc_state *state)
{
	state->audio = 0x03;
	state->display = FALSE;
	state->mode = LDV1000_MODE_STATUS;
	state->activereg = 0;
	write_16bits_to_ram_be(state, 0, 1);
}


/*-------------------------------------------------
    ldv1000_data_w - write callback when the
    ENTER state is written
-------------------------------------------------*/

static void ldv1000_data_w(laserdisc_state *state, UINT8 prev, UINT8 data)
{
	static const UINT8 numbers[10] = { 0x3f, 0x0f, 0x8f, 0x4f, 0x2f, 0xaf, 0x6f, 0x1f, 0x9f, 0x5f };

	/* 0xFF bytes are used for synchronization and filler; just ignore */
	if (data == 0xff)
		return;

	/* look for and process numbers */
	if (process_number(state, data, numbers))
	    return;

	/* handle commands */
	switch (data)
	{
		case 0x7f:  CMDPRINTF(("ldv1000: %d Recall\n", state->parameter));
		    /* set the active register */
			state->activereg = state->parameter;
			/* should also display the register value */
			break;

		case 0xa0:  CMDPRINTF(("ldv1000: x0 forward (stop)\n"));
			/* play forward at 0 speed (stop) */
			if (state->state == LASERDISC_PARKED)
				break;
			state->state = LASERDISC_STOPPED;
			break;

		case 0xa1:  CMDPRINTF(("ldv1000: x1/4 forward\n"));
			/* play forward at 1/4 speed */
			if (state->state == LASERDISC_PARKED)
				break;
			state->state = LASERDISC_PLAYING_ALTSPEED;
			state->altspeed = INT_TO_FRAC(1) / 4;
			break;

		case 0xa2:  CMDPRINTF(("ldv1000: x1/2 forward\n"));
			/* play forward at 1/2 speed */
			if (state->state == LASERDISC_PARKED)
				break;
			state->state = LASERDISC_PLAYING_ALTSPEED;
			state->altspeed = INT_TO_FRAC(1) / 2;
			break;

		case 0xa3:
		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa7:  CMDPRINTF(("ldv1000: x%d forward\n", (data & 0x07) - 2));
			/* play forward at 1-5x speed */
			if (state->state == LASERDISC_PARKED)
				break;
			state->state = LASERDISC_PLAYING_ALTSPEED;
			state->altspeed = INT_TO_FRAC(1) * ((data & 0x07) - 2);
			break;

		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7:
		case 0xb8:
		case 0xb9:
		case 0xba:  CMDPRINTF(("ldv1000: Skip forward %d0\n", data & 0x0f));
		    /* skip forward */
		    if (state->state != LASERDISC_PLAYING && state->state != LASERDISC_PLAYING_ALTSPEED)
		    	break;
		    /* note that this skips tracks, not frames; the track->frame count is not 1:1 */
		    /* in the case of 3:2 pulldown or other effects; for now, we just ignore the diff */
		    add_to_current_frame(state, INT_TO_FRAC(10 * (data & 0x0f)));
			break;

		case 0xbf:	CMDPRINTF(("ldv1000: %d Clear\n", state->parameter));
		    /* clears register display and removes pending arguments */
			break;

		case 0xc2:	CMDPRINTF(("ldv1000: Get frame no.\n"));
			/* returns the current frame number */
			state->mode = LDV1000_MODE_GET_FRAME;
			state->readpos = 0;
			sprintf((char *)state->readbuf, "%05d", FRAC_TO_INT(state->curfracframe));
			break;

		case 0xc3:	CMDPRINTF(("ldv1000: Get 2nd display\n"));
			/* returns the data from the 2nd display line */
			state->mode = LDV1000_MODE_GET_2ND;
			state->readpos = 0;
			sprintf((char *)state->readbuf, "     \x1c\x1c\x1c");
			break;

		case 0xc4:	CMDPRINTF(("ldv1000: Get 1st display\n"));
			/* returns the data from the 1st display line */
			state->mode = LDV1000_MODE_GET_1ST;
			state->readpos = 0;
			sprintf((char *)state->readbuf, "     \x1c\x1c\x1c");
			break;

		case 0xc8:	CMDPRINTF(("ldv1000: Transfer memory\n"));
			/* returns the data from the 1st display line */
			state->mode = LDV1000_MODE_GET_RAM;
			state->readpos = 0;
			break;

		case 0xcc:	CMDPRINTF(("ldv1000: Load\n"));
		    /* load program from disc -- not implemented */
			break;

		case 0xcd:	CMDPRINTF(("ldv1000: Display disable\n"));
			/* disables the display of current command -- not implemented */
			break;

		case 0xce:	CMDPRINTF(("ldv1000: Display enable\n"));
			/* enables the display of current command -- not implemented */
			break;

		case 0xf0:	CMDPRINTF(("ldv1000: Scan forward\n"));
			/* scan forward */
		    if (state->state == LASERDISC_PARKED)
		        break;
			state->state = LASERDISC_SCANNING;
			state->scanspeed = INT_TO_FRAC(2000);
			break;

		case 0xf1:  CMDPRINTF(("ldv1000: %d Display\n", state->parameter));
		    /* toggle or set the frame display */
			state->display = (state->parameter == -1) ? !state->display : (state->parameter & 1);
			break;

		case 0xf3:  CMDPRINTF(("ldv1000: %d Autostop\n", state->parameter));
		    /* play to a particular location and stop there */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    if (state->parameter == -1)
			    state->autostopfracframe = INT_TO_FRAC(read_16bits_from_ram_be(state, (state->activereg++ * 2) % 1024));
			else
			    state->autostopfracframe = INT_TO_FRAC(state->parameter);

			if (state->autostopfracframe > state->curfracframe)
				state->state = LASERDISC_PLAYING;
			else
			{
			    state->state = LASERDISC_SEARCHING;
			    state->searchfracframe = state->autostopfracframe;
   			}
			break;

		case 0xf4:  CMDPRINTF(("ldv1000: %d Audio track 1\n", state->parameter));
		    /* toggle or set the state of audio channel 1 */
		    state->audio &= ~0x01;
			state->audio |= (state->parameter == -1) ? (~state->audio & 0x01) : ((state->parameter & 1) << 0);
			break;

		case 0xf5:	CMDPRINTF(("ldv1000: %d Store\n", state->parameter));
		    /* store either the current frame number or an explicit value into the active register */
		    if (state->parameter == -1)
			    write_16bits_to_ram_be(state, (state->activereg * 2) % 1024, FRAC_TO_INT(state->curfracframe));
			else
			    write_16bits_to_ram_be(state, (state->activereg * 2) % 1024, state->parameter);
			state->activereg++;
			break;

		case 0xf6:	CMDPRINTF(("ldv1000: Step forward\n"));
		    /* step forward one frame */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    state->state = LASERDISC_STOPPED;
		    state->curfracframe += INT_TO_FRAC(1);
			break;

		case 0xf7:  CMDPRINTF(("ldv1000: %d Search\n", state->parameter));
		    /* search to a particular frame number */
		    if (state->state == LASERDISC_PARKED)
		        break;
			state->state = LASERDISC_SEARCHING;
		    if (state->parameter == -1)
			    state->searchfracframe = INT_TO_FRAC(read_16bits_from_ram_be(state, (state->activereg++ * 2) % 1024));
			else
			    state->searchfracframe = INT_TO_FRAC(state->parameter);
			break;

		case 0xf8:	CMDPRINTF(("ldv1000: Scan reverse\n"));
			/* scan reverse */
		    if (state->state == LASERDISC_PARKED)
		        break;
			state->state = LASERDISC_SCANNING;
			state->scanspeed = -INT_TO_FRAC(2000);
			break;

		case 0xf9:	CMDPRINTF(("ldv1000: Reject\n"));
		    /* eject the disc */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    state->state = LASERDISC_REJECTING;
			state->holdfinished = add_mame_times(mame_timer_get_time(), double_to_mame_time(10.0));
			state->postholdstate = LASERDISC_PARKED;
			break;

		case 0xfb:	CMDPRINTF(("ldv1000: %d Stop/Wait\n", state->parameter));
		    /* pause at the current location for a fixed amount of time (in 1/10ths of a second) */
		    if (state->state == LASERDISC_PARKED)
		        break;
			state->postholdstate = state->state;
		    state->state = LASERDISC_STOPPED;
		    if (state->parameter != -1)
				state->holdfinished = add_mame_times(mame_timer_get_time(), double_to_mame_time(state->parameter * 0.1));
			break;

		case 0xfc:  CMDPRINTF(("ldv1000: %d Audio track 2\n", state->parameter));
		    /* toggle or set the state of audio channel 2 */
		    state->audio &= ~0x02;
			state->audio |= (state->parameter == -1) ? (~state->audio & 0x02) : ((state->parameter & 1) << 1);
			break;

		case 0xfd:  CMDPRINTF(("ldv1000: Play\n"));
		    /* begin playing at regular speed, or load the disc if it is parked */
			if (state->state == LASERDISC_PARKED)
			{
				state->state = LASERDISC_LOADING;
				state->holdfinished = add_mame_times(mame_timer_get_time(), double_to_mame_time(10.0));
				state->postholdstate = LASERDISC_PLAYING;
				state->curfracframe = INT_TO_FRAC(1);
			}
			else
				state->state = LASERDISC_PLAYING;
			break;

		case 0xfe:	CMDPRINTF(("ldv1000: Step reverse\n"));
		    /* step backwards one frame */
		    if (state->state == LASERDISC_PARKED)
		        break;
		    state->state = LASERDISC_STOPPED;
		    state->curfracframe -= INT_TO_FRAC(1);
			break;

		default:	CMDPRINTF(("ldv1000: %d Unknown command %02X\n", state->parameter, data));
		    /* unknown command */
			break;
	}

	/* reset the parameter after executing a command */
	state->parameter = -1;
}


/*-------------------------------------------------
    ldv1000_status_strobe_r - return state of the
    status strobe
-------------------------------------------------*/

static UINT8 ldv1000_status_strobe_r(laserdisc_state *state)
{
	/* the status strobe is asserted (active low) 500-650usec after VSYNC */
	/* for a duration of 26usec; we pick 600-626usec */
	mame_time delta = sub_mame_times(mame_timer_get_time(), state->lastvsynctime);
	if (delta.subseconds >= DOUBLE_TO_SUBSECONDS(TIME_IN_USEC(600)) &&
		delta.subseconds < DOUBLE_TO_SUBSECONDS(TIME_IN_USEC(626)))
		return ASSERT_LINE;

	return CLEAR_LINE;
}


/*-------------------------------------------------
    ldv1000_command_strobe_r - return state of the
    command strobe
-------------------------------------------------*/

static UINT8 ldv1000_command_strobe_r(laserdisc_state *state)
{
	/* the command strobe is asserted (active low) 54 or 84usec after the status */
	/* strobe for a duration of 25usec; we pick 600+84 = 684-709usec */
	/* for a duration of 26usec; we pick 600-626usec */
	mame_time delta = sub_mame_times(mame_timer_get_time(), state->lastvsynctime);
	if (delta.subseconds >= DOUBLE_TO_SUBSECONDS(TIME_IN_USEC(684)) &&
		delta.subseconds < DOUBLE_TO_SUBSECONDS(TIME_IN_USEC(709)))
		return ASSERT_LINE;

	return CLEAR_LINE;
}


/*-------------------------------------------------
    ldv1000_status_r - return state of the ready
    line
-------------------------------------------------*/

static UINT8 ldv1000_status_r(laserdisc_state *state)
{
	UINT8 status = 0xff;

	/* switch off the current mode */
	switch (state->mode)
	{
		/* reading frame number returns 5 characters */
		case LDV1000_MODE_GET_FRAME:
			assert(state->readpos < 5);
			status = state->readbuf[state->readpos];
			break;

		/* reading display lines returns 8 characters */
		case LDV1000_MODE_GET_1ST:
		case LDV1000_MODE_GET_2ND:
			assert(state->readpos < 8);
			status = state->readbuf[state->readpos];
			break;

		/* reading RAM returns 1024 bytes */
		case LDV1000_MODE_GET_RAM:
			assert(state->readpos < 1024);
			status = state->ram[1023 - state->readpos];
			break;

		/* otherwise, we just compute a status code */
		default:
		case LDV1000_MODE_STATUS:
			switch (state->state)
			{
			    case LASERDISC_PARKED:      		status = 0xfc;		break;
			    case LASERDISC_PLAYING:      		status = 0xe4;		break;
			    case LASERDISC_STOPPED:      		status = 0xe5;		break;
			    case LASERDISC_SEARCHING:      		status = 0x50;		break;
			    case LASERDISC_SEARCH_FINISHED:     status = 0xd0;		break;
			    case LASERDISC_AUTOSTOPPED:      	status = 0x54;		break;
			    case LASERDISC_SLOW_FORWARD:	    status = 0xae;		break;
			    case LASERDISC_SLOW_REVERSE:	    status = 0xae;		break;
			    case LASERDISC_PLAYING_ALTSPEED:    status = 0xae;		break;
			    case LASERDISC_LOADING:      		status = 0x48;		break;
			    case LASERDISC_LOAD_FINISHED:      	status = 0xc8;		break;
			    case LASERDISC_REJECTING:      		status = 0x60;		break;
			    default:							assert(FALSE);		break;
			}
			break;
	}

	return status;
}
