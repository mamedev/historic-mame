/*
    c352.c - Namco C352 custom PCM chip emulation
    v0.1
    By R. Belmont

    Thanks to Cap of VivaNonno for info and The_Author for preliminary reverse-engineering

    Chip specs:
    32 voices
    Supports 8-bit linear and 8-bit muLaw samples
    Output: digital, 16 bit, 4 channels
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "driver.h"
#include "cpuintrf.h"
#include "state.h"

#define VERBOSE (0)

// flags

#define C352_FLGEX_LOOPFIXUP	0x10000	// have we done a "loop fixup" for this voice?
#define	C352_FLG_ACTIVE		0x4000	// channel is active
#define C352_FLG_PHASE		0x0080	// invert phase 180 degrees (e.g. flip sign of sample)
#define C352_FLG_LONG		0x0020	// "long-format" sample (can't loop, not sure what else it means)
#define C352_FLG_NOISE		0x0010	// play noise instead of sample
#define C352_FLG_MULAW		0x0008	// sample is mulaw instead of linear 8-bit PCM
#define C352_FLG_FILTER		0x0004	// apply filter
#define C352_FLG_REVLOOP   	0x0003	// loop backwards
#define C352_FLG_LOOP		0x0002	// loop forward
#define C352_FLG_REVERSE	0x0001	// play sample backwards

typedef struct
{
	UINT8	vol_l;
	UINT8	vol_r;
	UINT8	vol_l2;
	UINT8	vol_r2;
	UINT8	unk9;
	UINT8	bank;
	INT16	noise;
	UINT16	pitch;
	UINT16	start_addr;
	UINT16	end_addr;
	UINT32	flag;
	UINT32	current_addr;
	UINT32	stop_addr;
	UINT32	loop_addr;
	UINT32	pos;
} c352_ch_t;

static c352_ch_t c352_ch[32];
static unsigned char *c352_rom_samples;
static int c352_region;

static INT16 level_table[256];

static long	channel_l[2048];
static long	channel_r[2048];
static long	channel_l2[2048];
static long	channel_r2[2048];

static short	mulaw_table[256];
static unsigned int mseq_reg;

#define SAMPLE_RATE_BASE    	(88200)

// noise generator
static int get_mseq_bit(void)
{
	unsigned int mask = (1 << (7 - 1));
	unsigned int reg = mseq_reg;
	unsigned int bit = reg & (1 << (17 - 1));

	if (bit)
	{
       		reg = ((reg ^ mask) << 1) | 1;
	}
	else
	{
		reg = reg << 1;
	}

	mseq_reg = reg;

	return (reg & 1);
}

static void c352_mix_one_channel(unsigned long ch, long sample_count)
						  
{
														  
	int i;
													  
	signed short sample;
	float pbase = (float)SAMPLE_RATE_BASE / (float)Machine->sample_rate;
	INT32 frequency, delta, offset, cnt, flag;
	UINT32 pos;
	
	frequency = c352_ch[ch].pitch;
	delta=(long)((float)frequency * pbase);

	pos = c352_ch[ch].current_addr;	// sample pointer
	offset = c352_ch[ch].pos;	// 16.16 fixed-point offset into the sample
	flag = c352_ch[ch].flag;

	if ((flag & C352_FLG_ACTIVE) == 0) 
	{
		return;
	}

	for(i = 0 ; i < sample_count ; i++)
	{
		offset += delta;
		cnt = (offset>>16)&0x7fff;	
		if (cnt)			// if there is a whole sample part, chop it off now that it's been applied
		{
			offset &= 0xffff;		
		}

		// apply the whole sample part of the fraction to the current pointer and check if we're at the end
		if (flag & C352_FLG_REVERSE)
		{
			pos -= cnt;

			if (pos <= c352_ch[ch].loop_addr) 
			{
				if (flag & C352_FLG_LOOP)
				{
					pos = c352_ch[ch].stop_addr;
				}
				else
				{
					c352_ch[ch].flag &= ~C352_FLG_ACTIVE;
					return;
				}
			}
		}
		else
		{
			pos += cnt;

			if (pos >= c352_ch[ch].stop_addr) 
			{
				if (flag & C352_FLG_LOOP)
				{
					pos = c352_ch[ch].loop_addr;
				}
				else
				{
					c352_ch[ch].flag &= ~C352_FLG_ACTIVE;
					return;
				}
			}
		}

		if ((int)pos > memory_region_length(c352_region)) pos %= memory_region_length(c352_region);
		sample = (char)c352_rom_samples[pos];

		// sample is muLaw, not 8-bit linear (Fighting Layer uses this extensively)
		if (flag & C352_FLG_MULAW)
		{
			sample = mulaw_table[(unsigned char)sample];
		}
		else
		{
			sample <<= 8;
		}

		// invert phase 180 degrees for surround effects
		if (flag & C352_FLG_PHASE)
		{
			sample = -sample;
		}

		// play noise instead of sample data
		if (flag & C352_FLG_NOISE)
		{
			int noise_level = 0x8000;

			sample = c352_ch[ch].noise = (c352_ch[ch].noise << 1) | get_mseq_bit();
			sample = (sample & (noise_level - 1)) - (noise_level >> 1);
			if (sample > 0xff)
			{
				sample = 0xff;
			}
			else if (sample < 0)
			{
				sample = 0;
			}

			sample = mulaw_table[(unsigned char)sample];
		}

		channel_l[i] += ((sample * level_table[c352_ch[ch].vol_l])>>8);  
		channel_r[i] += ((sample * level_table[c352_ch[ch].vol_r])>>8);  
		channel_l2[i] += ((sample * level_table[c352_ch[ch].vol_l2])>>8);  
		channel_r2[i] += ((sample * level_table[c352_ch[ch].vol_r2])>>8);  
	}

	c352_ch[ch].pos = offset;
	c352_ch[ch].current_addr = pos;
}


static void c352_update(int num, short **buf, int sample_count)
{
	int i, j;
	short *bufferl = buf[0];
	short *bufferr = buf[1];
	short *bufferl2 = buf[2];
	short *bufferr2 = buf[3];

	for(i = 0 ; i < sample_count ; i++) 
	{
	       channel_l[i] = channel_r[i] = channel_l2[i] = channel_r2[i] = 0;
	}

	for (j = 0 ; j < 32 ; j++)
	{
		c352_mix_one_channel(j, sample_count);
	}

	for(i = 0 ; i < sample_count ; i++) 
	{
		*bufferl++ = (short) (channel_l[i] >>3);
		*bufferr++ = (short) (channel_r[i] >>3);
		*bufferl2++ = (short) (channel_l2[i] >>3);
		*bufferr2++ = (short) (channel_r2[i] >>3);
	}
}

static unsigned short c352_read_reg16(unsigned long address)
{
	unsigned long	chan;
	unsigned short	val;

	chan = (address >> 4) & 0xfff;
	if (chan > 31)
	{
		val = 0;
	}
	else
	{
		val = c352_ch[chan].flag;
	}
	return val;
}

static void c352_write_reg16(unsigned long address, unsigned short val)
{
	unsigned long	chan;
	unsigned long	temp;
	chan = (address >> 4) & 0xfff;

	if (chan > 31)
	{
		#if VERBOSE
		logerror("C352 CTRL %08x %04x\n", address, val);
		#endif
		return;
	}
	switch(address & 0xf)
	{
	case 0x0:
		// volumes (output 1)
		#if VERBOSE
		logerror("CH %02d LVOL %02x RVOL %02x\n", chan, val & 0xff, val >> 8);
		#endif
		c352_ch[chan].vol_l = val & 0xff;
		c352_ch[chan].vol_r = val >> 8;
		break;

	case 0x2:
		// volumes (output 2)
		#if VERBOSE
		logerror("CH %02d RLVOL %02x RRVOL %02x\n", chan, val & 0xff, val >> 8);
		#endif
		c352_ch[chan].vol_l2 = val & 0xff;
		c352_ch[chan].vol_r2 = val >> 8;
		break;

	case 0x4:
		// pitch
		#if VERBOSE
		logerror("CH %02d PITCH %04x\n", chan, val);
		#endif
		c352_ch[chan].pitch = val;
		break;

	case 0x6:
		// flags
		#if VERBOSE
		logerror("CH %02d FLAG %02x\n", chan, val);
		#endif
		// clear all flags except our internal one
		c352_ch[chan].flag &= C352_FLGEX_LOOPFIXUP;
		// and OR in the values
		c352_ch[chan].flag |= val;

		if (!(val & C352_FLG_ACTIVE))
		{
			return;
		}

		temp = c352_ch[chan].bank<<16;
		temp += c352_ch[chan].start_addr;
		c352_ch[chan].current_addr = temp;
		temp = c352_ch[chan].bank<<16;
		temp += c352_ch[chan].end_addr;
		c352_ch[chan].stop_addr = temp;

		if (!(c352_ch[chan].flag & C352_FLGEX_LOOPFIXUP))
		{
			c352_ch[chan].flag |= C352_FLGEX_LOOPFIXUP;

			if (val & C352_FLG_REVERSE)
			{
				if (c352_ch[chan].current_addr < c352_ch[chan].stop_addr)
				{
					if (c352_ch[chan].stop_addr >= 0x10000)
						c352_ch[chan].stop_addr -= 0x10000;
					if (c352_ch[chan].loop_addr >= 0x10000)
						c352_ch[chan].loop_addr -= 0x10000;
				}

				c352_ch[chan].loop_addr += (c352_ch[chan].bank<<16);

				// don't loop if loop start is out of range.
				// don't loop if the LONG flag is set (from vivanonno HLE code)
				// (supported by golgo13 song 2 - the percussion loops when it shouldn't and sounds bad if we don't do this)
				if ((c352_ch[chan].flag & C352_FLG_LONG))
				{     
					c352_ch[chan].flag &= ~C352_FLG_LOOP;
				}

			}
			else
			{
				if (c352_ch[chan].current_addr > c352_ch[chan].stop_addr)
				{
					c352_ch[chan].stop_addr += 0x10000;
					c352_ch[chan].loop_addr += 0x10000;
				}

				c352_ch[chan].loop_addr += (c352_ch[chan].bank<<16);

				if ((c352_ch[chan].loop_addr > c352_ch[chan].stop_addr) || (c352_ch[chan].flag & C352_FLG_LONG))
				{
					c352_ch[chan].flag &= ~C352_FLG_LOOP;
				}
			}
		}

		c352_ch[chan].pos = 0;
		break;

	case 0x8:
		// lower part is bank (ie bits 23-16 of address)
		// upper part is unknown
		c352_ch[chan].bank = val & 0xff;
		c352_ch[chan].unk9 = val >> 8;
		#if VERBOSE
		logerror("CH %02d UNK8 %02x", chan, c352_ch[chan].unk9);
		logerror("CH %02d BANK %02x", chan, c352_ch[chan].bank);
		#endif
		break;

	case 0xa:
		// start address
		#if VERBOSE
		logerror("CH %02d SADDR %04x\n", chan, val);
		#endif
		c352_ch[chan].flag &= ~C352_FLGEX_LOOPFIXUP;
		c352_ch[chan].start_addr = val;
		break;

	case 0xc:
		// end address
		#if VERBOSE
		logerror("CH %02d EADDR %04x\n", chan, val);
		#endif
		c352_ch[chan].flag &= ~C352_FLGEX_LOOPFIXUP;
		c352_ch[chan].end_addr = val;
		break;

	case 0xe:
		// loop address
		#if VERBOSE
		logerror("CH %02d LADDR %04x\n", chan, val);
		#endif
		c352_ch[chan].flag &= ~C352_FLGEX_LOOPFIXUP;
		c352_ch[chan].loop_addr = val;
		break;

	default:
		#if VERBOSE
		logerror("CH %02d UNKN %01x %04x", chan, address & 0xf, val);
		#endif
		break;
	}
}

static void c352_init(void)
{
	int i, sign;
	double x_max = 25000.0;
	double y_max = 127.0;
	double u = 8.0;
	double y, x;

	// clear all channels states
	memset(c352_ch, 0, sizeof(c352_ch_t)*32);

	// generate mulaw table for mulaw format samples
	for (i = 0; i < 256; i++)
	{
		sign = i & 0x80;
		y = (double)(i & 0x7f);
		x = (exp (y / y_max * log (1.0 + u)) - 1.0) * x_max / u;

		if (sign) 
		{
			x = -x;
		}
		mulaw_table[i] = (short)x;
	}

	// init noise generator
	mseq_reg = 0x12345678;

	// register save state info
	for (i = 0; i < 32; i++)
	{
		char cname[32];

		sprintf(cname, "C352 v %02d", i);

		state_save_register_UINT8(cname, 0, "voll1", &c352_ch[i].vol_l, 1);
		state_save_register_UINT8(cname, 0, "volr1", &c352_ch[i].vol_r, 1);
		state_save_register_UINT8(cname, 0, "voll2", &c352_ch[i].vol_l2, 1);
		state_save_register_UINT8(cname, 0, "volr2", &c352_ch[i].vol_r2, 1);
		state_save_register_UINT8(cname, 0, "unk9", &c352_ch[i].unk9, 1);
		state_save_register_UINT8(cname, 0, "bank", &c352_ch[i].bank, 1);
		state_save_register_INT16(cname, 0, "noise", &c352_ch[i].noise, 1);
		state_save_register_UINT16(cname, 0, "pitch", &c352_ch[i].pitch, 1);
		state_save_register_UINT16(cname, 0, "startaddr", &c352_ch[i].start_addr, 1);
		state_save_register_UINT16(cname, 0, "endaddr", &c352_ch[i].end_addr, 1);
		state_save_register_UINT32(cname, 0, "flag", &c352_ch[i].flag, 1);
		state_save_register_UINT32(cname, 0, "curaddr", &c352_ch[i].current_addr, 1);
		state_save_register_UINT32(cname, 0, "stopaddr", &c352_ch[i].stop_addr, 1);
		state_save_register_UINT32(cname, 0, "loopaddr", &c352_ch[i].loop_addr, 1);
		state_save_register_UINT32(cname, 0, "pos", &c352_ch[i].pos, 1);
	}

	for (i = 0; i < 256; i++)
	{
	      double max_level = 255.0;

	      level_table[255-i] = (int) (pow (10.0, (double) i / 256.0 * -20.0 / 20.0) * max_level);
	}
}

int c352_sh_start(const struct MachineSound *msound)
{
	char buf[4][40];
	const char *name[4];
	int vol[4];
	struct C352interface *intf;

	intf = msound->sound_interface;

	c352_rom_samples = memory_region(intf->region);
	c352_region = intf->region;

	sprintf(buf[0], "C352 L (1)"); 
	sprintf(buf[1], "C352 R (1)"); 
	sprintf(buf[2], "C352 L (2)"); 
	sprintf(buf[3], "C352 R (2)"); 
	name[0] = buf[0];
	name[1] = buf[1];
	name[2] = buf[2];
	name[3] = buf[3];
	vol[0]=intf->mixing_level >> 16;
	vol[1]=intf->mixing_level & 0xffff;
	vol[2]=intf->mixing_level2 >> 16;
	vol[3]=intf->mixing_level2 & 0xffff;
	stream_init_multi(4, name, vol, Machine->sample_rate, 0, c352_update);

	c352_init();

	return 0;
}

void c352_sh_stop(void)
{
}

READ16_HANDLER( c352_0_r )
{
	return(c352_read_reg16(offset*2));
}

WRITE16_HANDLER( c352_0_w )
{
	if (mem_mask == 0)
	{
		c352_write_reg16(offset*2, data);
	}
	else
	{
		logerror("C352: byte-wide write unsupported at this time!\n");
	}
}

