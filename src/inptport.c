/***************************************************************************

  inptport.c

  Input ports handling

***************************************************************************/

#include "driver.h"



static int input_port_value[MAX_INPUT_PORTS];
static int input_vblank[MAX_INPUT_PORTS];

/* Assuming a maxium of one analog input device per port BW 101297 */
static struct NewInputPort *input_analog[MAX_INPUT_PORTS];
static int input_analog_value[MAX_INPUT_PORTS];
static int input_analog_delta[MAX_INPUT_PORTS];

static int newgame = 1; /* LBO */

/***************************************************************************

  Obsolete functions which will eventually be removed

***************************************************************************/
static void old_load_input_port_settings(void)
{
	void *f;
	char buf[100];
	float temp;
	unsigned int i,j;
	unsigned int len,incount,keycount,trakcount;


	incount = 0;
	while (Machine->gamedrv->input_ports[incount].default_value != -1) incount++;

	keycount = 0;
	while (Machine->gamedrv->keysettings[keycount].num != -1) keycount++;

		trakcount = 0;
		if (Machine->gamedrv->trak_ports)
		{
			while (Machine->gamedrv->trak_ports[trakcount].axis != -1) trakcount++;
		}

		/* find the configuration file */
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_CONFIG,0)) != 0)
		{
			for (j=0; j < 4; j++)
			{
				if ((len = osd_fread(f,buf,4)) == 4)
				{
					len = (unsigned char)buf[3];
					buf[3] = '\0';
					if (stricmp(buf,"dsw") == 0) {
						if ((len == incount) && (osd_fread(f,buf,incount) == incount))
		{
			for (i = 0;i < incount;i++)
				Machine->gamedrv->input_ports[i].default_value = ((unsigned char)buf[i]);
		}
					} else if (stricmp(buf,"key") == 0) {
						if ((len == keycount) && (osd_fread(f,buf,keycount) == keycount))
		{
			for (i = 0;i < keycount;i++)
				Machine->gamedrv->input_ports[ Machine->gamedrv->keysettings[i].num ].keyboard[ Machine->gamedrv->keysettings[i].mask ] = ((unsigned char)buf[i]);
		}
					} else if (stricmp(buf,"joy") == 0) {
						if ((len == keycount) && (osd_fread(f,buf,keycount) == keycount))
		{
			for (i = 0;i < keycount;i++)
				Machine->gamedrv->input_ports[ Machine->gamedrv->keysettings[i].num ].joystick[ Machine->gamedrv->keysettings[i].mask ] = ((unsigned char)buf[i]);
		}
					} else if (stricmp(buf,"trk") == 0) {
						if ((len == trakcount) && (osd_fread(f,buf,sizeof(float)*trakcount) == sizeof(float)*trakcount))
		{
			for (i = 0;i < trakcount;i++) {
				memcpy(&temp, &buf[i*sizeof(float)], sizeof(float));
				Machine->gamedrv->trak_ports[i].scale = temp;
				}
		}
				}
			}
		}
		osd_fclose(f);
	}
}



static void old_save_input_port_settings(void)
{
	void *f;
	char buf[100];
	unsigned int i;
	unsigned int incount,keycount,trakcount;


incount = 0;
while (Machine->gamedrv->input_ports[incount].default_value != -1) incount++;

keycount = 0;
while (Machine->gamedrv->keysettings[keycount].num != -1) keycount++;

				trakcount = 0;
				if (Machine->gamedrv->trak_ports)
				{
					while (Machine->gamedrv->trak_ports[trakcount].axis != -1) trakcount++;
				}

				/* write the configuration file */
				if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_CONFIG,1)) != 0)
				{
						sprintf(buf, "dsw ");
						buf[3] = incount;
						osd_fwrite(f,buf,4);
	/* use buf as temporary buffer */
	for (i = 0;i < incount;i++)
		buf[i] = Machine->gamedrv->input_ports[i].default_value;
	osd_fwrite(f,buf,incount);

						sprintf(buf, "key ");
						buf[3] = keycount;
						osd_fwrite(f,buf,4);
	/* use buf as temporary buffer */
	for (i = 0;i < keycount;i++)
		buf[i] = Machine->gamedrv->input_ports[ Machine->gamedrv->keysettings[i].num ].keyboard[ Machine->gamedrv->keysettings[i].mask ];
	osd_fwrite(f,buf,keycount);

						sprintf(buf, "joy ");
						buf[3] = keycount;
						osd_fwrite(f,buf,4);
	/* use buf as temporary buffer */
	for (i = 0;i < keycount;i++)
		buf[i] = Machine->gamedrv->input_ports[ Machine->gamedrv->keysettings[i].num ].joystick[ Machine->gamedrv->keysettings[i].mask ];
	osd_fwrite(f,buf,keycount);

						sprintf(buf, "trk ");
						buf[3] = trakcount;
						osd_fwrite(f,buf,4);
	/* use buf as temporary buffer */
	for (i = 0;i < trakcount;i++)
							memcpy(&buf[i*sizeof(float)], &Machine->gamedrv->trak_ports[i].scale, sizeof(float));
	osd_fwrite(f,buf,sizeof(float)*trakcount);
						osd_fclose(f);
				}
}



static void old_update_input_ports(void)
{
	int port;


	/* scan all the input ports */
	port = 0;
	while (Machine->gamedrv->input_ports[port].default_value != -1)
	{
		int res, i;
		struct InputPort *in;


		in = &Machine->gamedrv->input_ports[port];

		res = in->default_value;
		input_vblank[port] = 0;

		for (i = 7;i >= 0;i--)
		{
			int c;


			c = in->keyboard[i];
			if (c == IPB_VBLANK)
			{
				res ^= (1 << i);
				input_vblank[port] ^= (1 << i);
			}
			else
			{
				if (c && osd_key_pressed(c))
					res ^= (1 << i);
				else
				{
					c = in->joystick[i];
					if (c && osd_joy_pressed(c))
						res ^= (1 << i);
				}
			}
		}

		input_port_value[port] = res;
		port++;
	}
}
int readtrakport(int port) {
  int axis;
  int read;
  struct TrakPort *in;

  in = &Machine->gamedrv->trak_ports[port];
  axis = in->axis;

  read = osd_trak_read(axis);

  if(in->conversion) {
    return((*in->conversion)(read*in->scale));
  }

  return(read*in->scale);
}

int input_trak_0_r(int offset) {
  return(readtrakport(0));
}

int input_trak_1_r(int offset) {
  return(readtrakport(1));
}

int input_trak_2_r(int offset) {
  return(readtrakport(2));
}

int input_trak_3_r(int offset) {
  return(readtrakport(3));
}
/***************************************************************************

  End of obsolete functions

***************************************************************************/








/***************************************************************************

  Configuration load/save

***************************************************************************/

static int readint(void *f,int *num)
{
	int i;


	*num = 0;
	for (i = 0;i < sizeof(int);i++)
	{
		unsigned char c;


		*num <<= 8;
		if (osd_fread(f,&c,1) != 1)
			return -1;
		*num |= c;
	}

	return 0;
}

static void writeint(void *f,int num)
{
	int i;


	for (i = 0;i < sizeof(int);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(int)-1)) & 0xff;
		osd_fwrite(f,&c,1);
		num <<= 8;
	}
}

static int readip(void *f,struct NewInputPort *in)
{
	if (osd_fread(f,&in->mask,1) != 1)
		return -1;
	if (osd_fread(f,&in->default_value,1) != 1)
		return -1;
	if (readint(f,&in->keyboard) != 0)
		return -1;
	if (readint(f,&in->joystick) != 0)
		return -1;
	if (readint(f,&in->arg) != 0)
		return -1;

	return 0;
}

static void writeip(void *f,struct NewInputPort *in)
{
	osd_fwrite(f,&in->mask,1);
	osd_fwrite(f,&in->default_value,1);
	writeint(f,in->keyboard);
	writeint(f,in->joystick);
	writeint(f,in->arg);
}



void load_input_port_settings(void)
{
	void *f;


if (Machine->input_ports == 0)
{
	old_load_input_port_settings();
	return;
}

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_CONFIG,0)) != 0)
	{
		struct NewInputPort *in;
		int total,savedtotal;
		char buf[8];


		in = Machine->gamedrv->new_input_ports;

		/* calculate the size of the array */
		total = 0;
		while (in->type != IPT_END)
		{
			total++;
			in++;
		}

		/* read header */
		if (osd_fread(f,buf,8) != 8)
			return;
		if (memcmp(buf,"MAMECFG\0",8) != 0)
			return;	/* header invalid */

		/* read array size */
		if (readint(f,&savedtotal) != 0)
			return;
		if (total != savedtotal)
			return;	/* different size */

		/* read the original settings and compare them with the ones defined in the driver */
		in = Machine->gamedrv->new_input_ports;
		while (in->type != IPT_END)
		{
			struct NewInputPort saved;


			if (readip(f,&saved) != 0)
				return;
			if (in->mask != saved.mask ||
					in->default_value != saved.default_value ||
					in->keyboard != saved.keyboard ||
					in->joystick != saved.joystick ||
					in->arg != saved.arg)
				return;	/* the default values are different */

			in++;
		}

		/* read the current settings */
		in = Machine->input_ports;
		newgame = 1; /* LBO */
		while (in->type != IPT_END)
		{
			if (readip(f,in) != 0)
				return;

			in++;
		}

		osd_fclose(f);
	}
}



void save_input_port_settings(void)
{
	void *f;


if (Machine->input_ports == 0)
{
	old_save_input_port_settings();
	return;
}


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_CONFIG,1)) != 0)
	{
		struct NewInputPort *in;
		int total;


		in = Machine->gamedrv->new_input_ports;

		/* calculate the size of the array */
		total = 0;
		while (in->type != IPT_END)
		{
			total++;
			in++;
		}

		/* write header */
		osd_fwrite(f,"MAMECFG\0",8);
		/* write array size */
		writeint(f,total);
		/* write the original settings as defined in the driver */
		in = Machine->gamedrv->new_input_ports;
		while (in->type != IPT_END)
		{
			writeip(f,in);
			in++;
		}
		/* write the current settings */
		in = Machine->input_ports;
		while (in->type != IPT_END)
		{
			writeip(f,in);
			in++;
		}

		osd_fclose(f);
	}
}





struct ipd
{
	int type;
	const char *name;
	int keyboard;
	int joystick;
};
struct ipd inputport_defaults[] =
{
	{ IPT_JOYSTICK_UP,        "Up",              OSD_KEY_UP,      OSD_JOY_UP },
	{ IPT_JOYSTICK_DOWN,      "Down",            OSD_KEY_DOWN,    OSD_JOY_DOWN },
	{ IPT_JOYSTICK_LEFT,      "Left",            OSD_KEY_LEFT,    OSD_JOY_LEFT },
	{ IPT_JOYSTICK_RIGHT,     "Right",           OSD_KEY_RIGHT,   OSD_JOY_RIGHT },
	{ IPT_JOYSTICK_UP    | IPF_PLAYER2, "2 Up",            OSD_KEY_R,       0 },
	{ IPT_JOYSTICK_DOWN  | IPF_PLAYER2, "2 Down",          OSD_KEY_F,       0 },
	{ IPT_JOYSTICK_LEFT  | IPF_PLAYER2, "2 Left",          OSD_KEY_D,       0 },
	{ IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "2 Right",         OSD_KEY_G,       0 },
	{ IPT_JOYSTICK_UP    | IPF_PLAYER3, "3 Up",            0,               0 },
	{ IPT_JOYSTICK_DOWN  | IPF_PLAYER3, "3 Down",          0,               0 },
	{ IPT_JOYSTICK_LEFT  | IPF_PLAYER3, "3 Left",          0,               0 },
	{ IPT_JOYSTICK_RIGHT | IPF_PLAYER3, "3 Right",         0,               0 },
	{ IPT_JOYSTICK_UP    | IPF_PLAYER4, "4 Up",            0,               0 },
	{ IPT_JOYSTICK_DOWN  | IPF_PLAYER4, "4 Down",          0,               0 },
	{ IPT_JOYSTICK_LEFT  | IPF_PLAYER4, "4 Left",          0,               0 },
	{ IPT_JOYSTICK_RIGHT | IPF_PLAYER4, "4 Right",         0,               0 },
	{ IPT_JOYSTICKRIGHT_UP,    "Right/Up",        OSD_KEY_I,       OSD_JOY_FIRE2 },
	{ IPT_JOYSTICKRIGHT_DOWN,  "Right/Down",      OSD_KEY_K,       OSD_JOY_FIRE3 },
	{ IPT_JOYSTICKRIGHT_LEFT,  "Right/Left",      OSD_KEY_J,       OSD_JOY_FIRE1 },
	{ IPT_JOYSTICKRIGHT_RIGHT, "Right/Right",     OSD_KEY_L,       OSD_JOY_FIRE4 },
	{ IPT_JOYSTICKLEFT_UP,     "Left/Up",         OSD_KEY_E,       OSD_JOY_UP },
	{ IPT_JOYSTICKLEFT_DOWN,   "Left/Down",       OSD_KEY_D,       OSD_JOY_DOWN },
	{ IPT_JOYSTICKLEFT_LEFT,   "Left/Left",       OSD_KEY_S,       OSD_JOY_LEFT },
	{ IPT_JOYSTICKLEFT_RIGHT,  "Left/Right",      OSD_KEY_F,       OSD_JOY_RIGHT },
	{ IPT_BUTTON1,             "Button 1",        OSD_KEY_LCONTROL,OSD_JOY_FIRE1 },
	{ IPT_BUTTON2,             "Button 2",        OSD_KEY_ALT,     OSD_JOY_FIRE2 },
	{ IPT_BUTTON3,             "Button 3",        OSD_KEY_SPACE,   OSD_JOY_FIRE3 },
	{ IPT_BUTTON4,             "Button 4",        OSD_KEY_LSHIFT,  OSD_JOY_FIRE4 },
	{ IPT_BUTTON5,             "Button 5",        OSD_KEY_Z,       0 },
	{ IPT_BUTTON1 | IPF_PLAYER2, "2 Button 1",        OSD_KEY_A,       0 },
	{ IPT_BUTTON2 | IPF_PLAYER2, "2 Button 2",        OSD_KEY_S,       0 },
	{ IPT_BUTTON3 | IPF_PLAYER2, "2 Button 3",        OSD_KEY_Q,       0 },
	{ IPT_BUTTON4 | IPF_PLAYER2, "2 Button 4",        OSD_KEY_W,       0 },
	{ IPT_BUTTON1 | IPF_PLAYER3, "3 Button 1",        0,               0 },
	{ IPT_BUTTON2 | IPF_PLAYER3, "3 Button 2",        0,               0 },
	{ IPT_BUTTON3 | IPF_PLAYER3, "3 Button 3",        0,               0 },
	{ IPT_BUTTON4 | IPF_PLAYER3, "3 Button 4",        0,               0 },
	{ IPT_BUTTON1 | IPF_PLAYER4, "4 Button 1",        0,               0 },
	{ IPT_BUTTON2 | IPF_PLAYER4, "4 Button 2",        0,               0 },
	{ IPT_BUTTON3 | IPF_PLAYER4, "4 Button 3",        0,               0 },
	{ IPT_BUTTON4 | IPF_PLAYER4, "4 Button 4",        0,               0 },
	{ IPT_COIN1,               "Coin A",          OSD_KEY_3,       IP_JOY_NONE },
	{ IPT_COIN2,               "Coin B",          OSD_KEY_4,       IP_JOY_NONE },
	{ IPT_COIN3,               "Coin C",          OSD_KEY_5,       IP_JOY_NONE },
	{ IPT_COIN4,               "Coin D",          OSD_KEY_6,       IP_JOY_NONE },
	{ IPT_TILT,                "Tilt",            OSD_KEY_T,       IP_JOY_NONE },
	{ IPT_START1,              "1 Player Start",  OSD_KEY_1,       IP_JOY_NONE },
	{ IPT_START2,              "2 Players Start", OSD_KEY_2,       IP_JOY_NONE },
	{ IPT_UNKNOWN,             "UNKNOWN",         IP_KEY_NONE,     IP_JOY_NONE },
	{ IPT_END,                 0,                 IP_KEY_NONE,     IP_JOY_NONE }
};

const char *default_name(const struct NewInputPort *in)
{
	int i;


	if (in->name != IP_NAME_DEFAULT) return in->name;

	i = 0;
	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != (in->type & (~IPF_MASK | IPF_PLAYERMASK)))
		i++;

	return inputport_defaults[i].name;
}

int default_key(const struct NewInputPort *in)
{
	int i;


	if (in->keyboard == IP_KEY_PREVIOUS) in--;

	if (in->keyboard != IP_KEY_DEFAULT) return in->keyboard;

	i = 0;
	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != (in->type & (~IPF_MASK | IPF_PLAYERMASK)))
		i++;

	return inputport_defaults[i].keyboard;
}

int default_joy(const struct NewInputPort *in)
{
	int i;


	if (in->joystick == IP_JOY_PREVIOUS) in--;

	if (in->joystick != IP_JOY_DEFAULT) return in->joystick;

	i = 0;
	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != (in->type & (~IPF_MASK | IPF_PLAYERMASK)))
		i++;

	return inputport_defaults[i].joystick;
}

/* NS, LBO 100397, BW 101297 */
void update_analog_port(port)
{
	struct NewInputPort *in;
	int current, delta, scaled_delta, type, sensitivity, clip, min, max;
	int axis, inckey, deckey, is_stick;

	/* get input definition */
	in=input_analog[port];
	type=(in->type & ~IPF_MASK);

	switch (type) {
		case IPT_DIAL:
			axis = X_AXIS;
			deckey = OSD_KEY_Z;
			inckey = OSD_KEY_X;
			is_stick = 0;
			break;
		case IPT_TRACKBALL_X:
			axis = X_AXIS;
			deckey = OSD_KEY_LEFT;
			inckey = OSD_KEY_RIGHT;
			is_stick = 0;
			break;
		case IPT_TRACKBALL_Y:
			axis = Y_AXIS;
			deckey = OSD_KEY_UP;
			inckey = OSD_KEY_DOWN;
			is_stick = 0;
			break;
		case IPT_AD_STICK_X:
			axis = X_AXIS;
			deckey = OSD_KEY_LEFT;
			inckey = OSD_KEY_RIGHT;
			is_stick = 1;
			break;
		case IPT_AD_STICK_Y:
			axis = Y_AXIS;
			deckey = OSD_KEY_UP;
			inckey = OSD_KEY_DOWN;
			is_stick = 1;
			break;
		default:
			/* Use some defaults to prevent crash */
			axis = X_AXIS;
			deckey = OSD_KEY_Z;
			inckey = OSD_KEY_Y;
			is_stick = 0;
			if (errorlog)
				fprintf (errorlog,"Oops, polling non analog device in update_analog_port()????\n");
	}


	if (newgame || ((in->type & IPF_CENTER) && (!is_stick)))
		input_analog_value[port] = in->default_value;

	current = input_analog_value[port];

	delta = osd_trak_read(axis);

	if (osd_key_pressed(deckey))
		delta -= 4;
	if (osd_key_pressed(inckey))
		delta += 4;

	sensitivity = in->arg & 0x000000ff;
	clip = (in->arg & 0x0000ff00) >> 8;

	/* Internally, we use 16=2^4 as default sensivity. 32 is twice as */
	/* sensitive, 8 half, and so on. The macro in driver.h translates */
	/* this to the old "percent" semantics, so there's no need to     */
	/* change the drivers. 101297 BW */

	scaled_delta = (delta * sensitivity) / 16;

	if (clip != 0) {
		if (scaled_delta < -clip)
			scaled_delta = -clip;
		else if (scaled_delta > clip)
			scaled_delta = clip;
	}

	if (in->type & IPF_REVERSE) scaled_delta = -scaled_delta;

	if (is_stick)
	{
		if (axis == Y_AXIS) scaled_delta = -scaled_delta;
		if ((delta == 0) && (in->type & IPF_CENTER))
		{
			if ((current+scaled_delta)>in->default_value)
			scaled_delta-=1;
			if ((current+scaled_delta)<in->default_value)
			scaled_delta+=1;
		}

		min = (in->arg & 0x00ff0000) >> 16;
		max = (in->arg & 0xff000000) >> 24;
		if (current+scaled_delta <= min)
			scaled_delta=min-current;
		if (current+scaled_delta >= max)
			scaled_delta=max-current;
	}

	current += scaled_delta;
	current &= in->mask;

	input_analog_value[port]=current;

	input_port_value[port] &= ~in->mask;
	input_port_value[port] |= current;
}

void update_analog_ports(void)
{
	int port;

	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		if (input_analog[port])
			update_analog_port(port);
	}
}

void update_input_ports(void)
{
	int i,port,ib;
	struct NewInputPort *in;
#define MAX_INPUT_BITS 1024
static int impulsecount[MAX_INPUT_BITS];
static int waspressed[MAX_INPUT_BITS];
#define MAX_JOYSTICKS 3
#define MAX_PLAYERS 4
int joystick[MAX_JOYSTICKS*MAX_PLAYERS][4];


if (Machine->input_ports == 0)
{
	old_update_input_ports();
	return;
}

	/* clear all the values before proceeding */
	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		input_port_value[port] = 0;
		input_vblank[port] = 0;
		input_analog[port] = 0;
		input_analog_delta[port] = 0;
	}

	for (i = 0;i < 4*MAX_JOYSTICKS*MAX_PLAYERS;i++)
		joystick[i/4][i%4] = 0;

	in = Machine->input_ports;

	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		if (errorlog) fprintf(errorlog,"Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else in++;

	/* scan all the input ports */
	port = 0;
	ib = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		struct NewInputPort *start;


		/* first of all, scan the whole input port definition and build the */
		/* default value. I must do it before checking for input because otherwise */
		/* multiple keys associated with the same input bit wouldn't work (the bit */
		/* would be reset to its default value by the second entry, regardless if */
		/* the key associated with the first entry was pressed) */
		start = in;
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING)	/* skip dipswitch definitions */
			{
				input_port_value[port] =
						(input_port_value[port] & ~in->mask) | (in->default_value & in->mask);
			}

			in++;
		}

		/* now get back to the beginning of the input port and check the input bits. */
		in = start;
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
					(in->type & IPF_UNUSED) == 0)	/* skip unused bits */
			{
				if ((in->type & ~IPF_MASK) == IPT_VBLANK)
				{
					input_vblank[port] ^= in->mask;
					input_port_value[port] ^= in->mask;
				}
				else if (((in->type & ~IPF_MASK) == IPT_DIAL)
					|| ((in->type & ~IPF_MASK) == IPT_TRACKBALL_X)
					|| ((in->type & ~IPF_MASK) == IPT_TRACKBALL_Y)
					|| ((in->type & ~IPF_MASK) == IPT_AD_STICK_X)
					|| ((in->type & ~IPF_MASK) == IPT_AD_STICK_Y))
					{
						input_analog[port]=in;
						update_analog_port(port);
					}
				else
				{
					int key,joy;


					key = default_key(in);
					joy = default_joy(in);

					if ((key != 0 && key != IP_KEY_NONE && osd_key_pressed(key)) ||
							(joy != 0 && joy != IP_JOY_NONE && osd_joy_pressed(joy)))
					{
						if (in->type & IPF_IMPULSE)
						{
if (errorlog && in->arg == 0)
	fprintf(errorlog,"error in input port definition: IPF_IMPULSE with length = 0\n");
							if (waspressed[ib] == 0)
								impulsecount[ib] = in->arg;
								/* the input bit will be toggled later */
						}
						else if (in->type & IPF_TOGGLE)
						{
							if (waspressed[ib] == 0)
							{
								in->default_value ^= in->mask;
								input_port_value[port] ^= in->mask;
							}
						}
						else if ((in->type & ~IPF_MASK) >= IPT_JOYSTICK_UP &&
								(in->type & ~IPF_MASK) <= IPT_JOYSTICKLEFT_RIGHT)
						{
							int joynum,joydir,mask,player;


							player = 0;
							if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER2) player = 1;
							else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER3) player = 2;
							else if ((in->type & IPF_PLAYERMASK) == IPF_PLAYER4) player = 3;

							joynum = player * MAX_JOYSTICKS +
									((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) / 4;
							joydir = ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) % 4;

							mask = in->mask;

							/* avoid movement in two opposite directions */
							if (joystick[joynum][joydir ^ 1] != 0)
								mask = 0;

							if (in->type & IPF_4WAY)
							{
								int dir;


								/* avoid diagonal movements */
								for (dir = 0;dir < 4;dir++)
								{
									if (joystick[joynum][dir] != 0)
										mask = 0;
								}
							}

							joystick[joynum][joydir] = 1;

							input_port_value[port] ^= mask;
						}
						else
							input_port_value[port] ^= in->mask;

						waspressed[ib] = 1;
					}
					else
						waspressed[ib] = 0;

					if ((in->type & IPF_IMPULSE) && impulsecount[ib] > 0)
					{
						impulsecount[ib]--;
						waspressed[ib] = 1;
						input_port_value[port] ^= in->mask;
					}
				}
			}

			in++;
			ib++;
		}

		port++;
		if (in->type == IPT_PORT) in++;
	}
	newgame = 0; /* LBO */
}



int readinputport(int port)
{
	if (input_vblank[port])
	{
		/* I'm not yet sure about how long the vertical blanking should last, */
		/* I think it should be about 1/12th of the frame. */
		if (cpu_getfcount() < cpu_getfperiod() * 11 / 12)
		{
			input_port_value[port] ^= input_vblank[port];
			input_vblank[port] = 0;
		}
	}

	return input_port_value[port];
}

int input_port_0_r(int offset) { return readinputport(0); }
int input_port_1_r(int offset) { return readinputport(1); }
int input_port_2_r(int offset) { return readinputport(2); }
int input_port_3_r(int offset) { return readinputport(3); }
int input_port_4_r(int offset) { return readinputport(4); }
int input_port_5_r(int offset) { return readinputport(5); }
int input_port_6_r(int offset) { return readinputport(6); }
int input_port_7_r(int offset) { return readinputport(7); }
int input_port_8_r(int offset) { return readinputport(8); }
int input_port_9_r(int offset) { return readinputport(9); }
int input_port_10_r(int offset) { return readinputport(10); }
int input_port_11_r(int offset) { return readinputport(11); }
int input_port_12_r(int offset) { return readinputport(12); }
int input_port_13_r(int offset) { return readinputport(13); }
int input_port_14_r(int offset) { return readinputport(14); }
int input_port_15_r(int offset) { return readinputport(15); }
