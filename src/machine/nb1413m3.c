/***************************************************************************

	Machine Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi 1999/11/05 -

***************************************************************************/

#include "driver.h"
#include "machine/nb1413m3.h"


int nb1413m3_type;
int nb1413m3_int_count;
int nb1413m3_sndrombank1;
int nb1413m3_sndrombank2;
int nb1413m3_busyctr;
int nb1413m3_busyflag;
int nb1413m3_inputport;
unsigned char *nb1413m3_nvram;
size_t nb1413m3_nvram_size;

static int nb1413m3_nmi_clock;
static int nb1413m3_nmi_enable;
static int nb1413m3_counter;
static int nb1413m3_gfxradr_l;
static int nb1413m3_gfxradr_h;
static int nb1413m3_gfxrombank;
static int nb1413m3_outcoin_flag;


void nb1413m3_init_machine(void)
{
	nb1413m3_nmi_clock = 0;
	nb1413m3_nmi_enable = 0;
	nb1413m3_counter = 0;
	nb1413m3_sndrombank1 = 0;
	nb1413m3_sndrombank2 = 0;
	nb1413m3_busyctr = 0;
	nb1413m3_busyflag = 1;
	nb1413m3_gfxradr_l = 0;
	nb1413m3_gfxradr_h = 0;
	nb1413m3_gfxrombank = 0;
	nb1413m3_inputport = 0xff;
	nb1413m3_outcoin_flag = 1;
}

void nb1413m3_nmi_clock_w(int data)
{
	nb1413m3_nmi_clock = ((data & 0xf0) >> 4);
}

int nb1413m3_interrupt(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_TRIPLEW1:
		case	NB1413M3_NTOPSTAR:
		case	NB1413M3_PSTADIUM:
		case	NB1413M3_TRIPLEW2:
		case	NB1413M3_VANILLA:
		case	NB1413M3_MJLSTORY:
		case	NB1413M3_QMHAYAKU:
		case	NB1413M3_AV2MJ1:
			nb1413m3_busyflag = 1;
			nb1413m3_busyctr = 0;
			return interrupt();
			break;
		default:
			if (nb1413m3_nmi_enable)
			{
				if (nb1413m3_counter++ % nb1413m3_int_count)
				{
					return nmi_interrupt();
				}
				else
				{
					nb1413m3_busyflag = 1;
					nb1413m3_busyctr = 0;
					return interrupt();
				}
			}
			else
			{
				if (nb1413m3_counter++ % nb1413m3_int_count)
				{
					return ignore_interrupt();
				}
				else
				{
					nb1413m3_busyflag = 1;
					nb1413m3_busyctr = 0;
					return interrupt();
				}
			}
			break;
	}
}

void nb1413m3_nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, nb1413m3_nvram, nb1413m3_nvram_size);
	else
	{
		if (file)
			osd_fread(file, nb1413m3_nvram, nb1413m3_nvram_size);
		else
			memset(nb1413m3_nvram, 0, nb1413m3_nvram_size);
	}
}

int nb1413m3_sndrom_r(int offset)
{
	unsigned char *SNDROM = memory_region(REGION_SOUND1);
	int rombank;
	int ret;

	switch (nb1413m3_type)
	{
		case	NB1413M3_IEMOTO:
		case	NB1413M3_SEIHA:
		case	NB1413M3_SEIHAM:
		case	NB1413M3_OJOUSAN:
		case	NB1413M3_MJSIKAKU:
			rombank = ((nb1413m3_sndrombank2 << 1) + nb1413m3_sndrombank1);
			break;
		case	NB1413M3_HYHOO:
		case	NB1413M3_HYHOO2:
			rombank = (nb1413m3_sndrombank1 & 0x01);
			break;
		case	NB1413M3_APPAREL:
		case	NB1413M3_SECOLOVE:
		case	NB1413M3_CITYLOVE:
		case	NB1413M3_HOUSEMNQ:
		case	NB1413M3_HOUSEMN2:
		case	NB1413M3_BIJOKKOY:
		case	NB1413M3_BIJOKKOG:
		case	NB1413M3_MJCAMERA:
		case	NB1413M3_OTONANO:
		case	NB1413M3_KAGUYA:
			rombank = nb1413m3_sndrombank1;
			break;
		case	NB1413M3_SCANDAL:
		case	NB1413M3_SCANDALM:
		case	NB1413M3_MJFOCUSM:
		case	NB1413M3_BANANADR:
			offset = (((offset & 0x7f00) >> 8) | ((offset & 0x0080) >> 0) | ((offset & 0x007f) << 8));
			rombank = (nb1413m3_sndrombank1 >> 1);
			break;
		case	NB1413M3_MSJIKEN:
		case	NB1413M3_HANAMOMO:
		case	NB1413M3_TELMAHJN:
		case	NB1413M3_GIONBANA:
		case	NB1413M3_MGMEN89:
		case	NB1413M3_MJFOCUS:
		case	NB1413M3_PEEPSHOW:
		case	NB1413M3_GALKOKU:
		case	NB1413M3_MJNANPAS:
		case	NB1413M3_MLADYHTR:
		case	NB1413M3_CLUB90S:
		case	NB1413M3_CHINMOKU:
		case	NB1413M3_GALKAIKA:
		case	NB1413M3_MCONTEST:
		case	NB1413M3_TOKIMBSJ:
		case	NB1413M3_TOKYOGAL:
		case	NB1413M3_MAIKO:
		case	NB1413M3_HANAOJI:
		default:
			rombank = (nb1413m3_sndrombank1 >> 1);
			break;
	}

	ret = SNDROM[((0x08000 * rombank) + offset)];

	return ret;
}

void nb1413m3_sndrombank1_w(int data)
{
	nb1413m3_nmi_enable = ((data & 0x20) >> 5);
	nb1413m3_sndrombank1 = (((data & 0xc0) >> 5) | ((data & 0x10) >> 4));
}

void nb1413m3_sndrombank2_w(int data)
{
	nb1413m3_sndrombank2 = (data & 0x03);
}

int nb1413m3_gfxrom_r(int offset)
{
	unsigned char *GFXROM = memory_region(REGION_GFX1);

	return GFXROM[(0x20000 * (nb1413m3_gfxrombank | ((nb1413m3_sndrombank1 & 0x02) << 3))) + ((0x0200 * nb1413m3_gfxradr_h) + (0x0002 * nb1413m3_gfxradr_l)) + (offset & 0x01)];
}

void nb1413m3_gfxrombank_w(int data)
{
	nb1413m3_gfxrombank = (((data & 0xc0) >> 4) + (data & 0x03));
}

void nb1413m3_gfxradr_l_w(int data)
{
	nb1413m3_gfxradr_l = data;
}

void nb1413m3_gfxradr_h_w(int data)
{
	nb1413m3_gfxradr_h = data;
}

void nb1413m3_inputportsel_w(int data)
{
	nb1413m3_inputport = data;
}

int nb1413m3_inputport0_r(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_NIGHTGL:
		case	NB1413M3_PASTELGL:
			return ((input_port_3_r(0) & 0xfe) | (nb1413m3_busyflag & 0x01));
			break;
		default:
			return ((input_port_2_r(0) & 0xfc) | ((nb1413m3_outcoin_flag & 0x01) << 1) | (nb1413m3_busyflag & 0x01));
			break;
	}
}

int nb1413m3_inputport1_r(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_PASTELGL:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case	0x01:	return readinputport(4);
				case	0x02:	return readinputport(5);
				case	0x04:	return readinputport(6);
				case	0x08:	return readinputport(7);
				case	0x10:	return readinputport(8);
				default:	return 0xff;
			}
			break;
		case	NB1413M3_HYHOO:
		case	NB1413M3_HYHOO2:
			switch ((nb1413m3_inputport ^ 0xff) & 0x07)
			{
				case	0x01:	return readinputport(3);
				case	0x02:	return readinputport(4);
				case	0x04:	return 0xff;
				default:	return 0xff;
			}
			break;
		case	NB1413M3_MSJIKEN:
		case	NB1413M3_TELMAHJN:
			if (readinputport(0) & 0x80)
			{
				switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
				{
					case	0x01:	return readinputport(3);
					case	0x02:	return readinputport(4);
					case	0x04:	return readinputport(5);
					case	0x08:	return readinputport(6);
					case	0x10:	return readinputport(7);
					default:	return 0xff;
				}
			}
			else return readinputport(9);
			break;
		default:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case	0x01:	return readinputport(3);
				case	0x02:	return readinputport(4);
				case	0x04:	return readinputport(5);
				case	0x08:	return readinputport(6);
				case	0x10:	return readinputport(7);
				default:	return 0xff;
			}
			break;
	}
}

int nb1413m3_inputport2_r(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_PASTELGL:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case	0x01:	return 0xff;
				case	0x02:	return 0xff;
				case	0x04:	return 0xff;
				case	0x08:	return 0xff;
				case	0x10:	return 0xff;
				default:	return 0xff;
			}
			break;
		case	NB1413M3_HYHOO:
		case	NB1413M3_HYHOO2:
			switch ((nb1413m3_inputport ^ 0xff) & 0x07)
			{
				case	0x01:	return 0xff;
				case	0x02:	return 0xff;
				case	0x04:	return readinputport(5);
				default:	return 0xff;
			}
			break;
		case	NB1413M3_MSJIKEN:
		case	NB1413M3_TELMAHJN:
			if (readinputport(0) & 0x80)
			{
				switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
				{
					case	0x01:	return 0xff;
					case	0x02:	return 0xff;
					case	0x04:	return 0xff;
					case	0x08:	return 0xff;
					case	0x10:	return 0xff;
					default:	return 0xff;
				}
			}
			else return readinputport(8);
			break;
		default:
			switch ((nb1413m3_inputport ^ 0xff) & 0x1f)
			{
				case	0x01:	return 0xff;
				case	0x02:	return 0xff;
				case	0x04:	return 0xff;
				case	0x08:	return 0xff;
				case	0x10:	return 0xff;
				default:	return 0xff;
			}
			break;
	}
}

int nb1413m3_inputport3_r(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_SEIHAM:
		case	NB1413M3_TOKIMBSJ:
		case	NB1413M3_MJFOCUSM:
		case	NB1413M3_SCANDALM:
		case	NB1413M3_BANANADR:
			return ((nb1413m3_outcoin_flag & 0x01) << 1);
			break;
		case	NB1413M3_MAIKO:
		case	NB1413M3_HANAOJI:
			return ((input_port_8_r(0) & 0xfd) | ((nb1413m3_outcoin_flag & 0x01) << 1));
			break;
		default:
			return 0xff;
			break;
	}
}

int nb1413m3_dipsw1_r(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_OTONANO:
		case	NB1413M3_MJCAMERA:
		case	NB1413M3_KAGUYA:
			return (((readinputport(0) & 0x0f) << 4) | (readinputport(1) & 0x0f));
			break;
		case	NB1413M3_MJFOCUSM:
		case	NB1413M3_GALKOKU:
		case	NB1413M3_GALKAIKA:
		case	NB1413M3_MCONTEST:
		case	NB1413M3_TOKIMBSJ:
		case	NB1413M3_TOKYOGAL:
			return ((readinputport(0) & 0x0f) | ((readinputport(1) & 0x0f) << 4));
			break;
		case	NB1413M3_TRIPLEW1:
		case	NB1413M3_NTOPSTAR:
		case	NB1413M3_PSTADIUM:
		case	NB1413M3_TRIPLEW2:
		case	NB1413M3_VANILLA:
		case	NB1413M3_MJLSTORY:
		case	NB1413M3_QMHAYAKU:
			return (((readinputport(1) & 0x01) >> 0) | ((readinputport(1) & 0x04) >> 1) |
			        ((readinputport(1) & 0x10) >> 2) | ((readinputport(1) & 0x40) >> 3) |
			        ((readinputport(0) & 0x01) << 4) | ((readinputport(0) & 0x04) << 3) |
			        ((readinputport(0) & 0x10) << 2) | ((readinputport(0) & 0x40) << 1));
			break;
		default:
			return readinputport(0);
			break;
	}
}

int nb1413m3_dipsw2_r(void)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_OTONANO:
		case	NB1413M3_MJCAMERA:
		case	NB1413M3_KAGUYA:
			return ((readinputport(0) & 0xf0) | ((readinputport(1) & 0xf0) >> 4));
			break;
		case	NB1413M3_MJFOCUSM:
		case	NB1413M3_GALKOKU:
		case	NB1413M3_GALKAIKA:
		case	NB1413M3_MCONTEST:
		case	NB1413M3_TOKIMBSJ:
		case	NB1413M3_TOKYOGAL:
			return (((readinputport(0) & 0xf0) >> 4) | (readinputport(1) & 0xf0));
			break;
		case	NB1413M3_TRIPLEW1:
		case	NB1413M3_NTOPSTAR:
		case	NB1413M3_PSTADIUM:
		case	NB1413M3_TRIPLEW2:
		case	NB1413M3_VANILLA:
		case	NB1413M3_MJLSTORY:
		case	NB1413M3_QMHAYAKU:
			return (((readinputport(1) & 0x02) >> 1) | ((readinputport(1) & 0x08) >> 2) |
			        ((readinputport(1) & 0x20) >> 3) | ((readinputport(1) & 0x80) >> 4) |
			        ((readinputport(0) & 0x02) << 3) | ((readinputport(0) & 0x08) << 2) |
			        ((readinputport(0) & 0x20) << 1) | ((readinputport(0) & 0x80) << 0));
			break;
		default:
			return readinputport(1);
			break;
	}
}

void nb1413m3_outcoin_w(int data)
{
	switch (nb1413m3_type)
	{
		case	NB1413M3_SEIHAM:
		case	NB1413M3_TOKIMBSJ:
		case	NB1413M3_MJFOCUSM:
		case	NB1413M3_SCANDALM:
		case	NB1413M3_BANANADR:
		case	NB1413M3_HANAOJI:
			if (data & 0x04) nb1413m3_outcoin_flag ^= 1;
			else nb1413m3_outcoin_flag = 1;
			break;
		default:
			break;
	}
}

void nb1413m3_vcrctrl_w(int data)
{
	if (data & 0x08) usrintf_showmessage(" ** VCR CONTROL ** ");
}
