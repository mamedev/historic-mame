#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"
#include "datafile.h"

/* Output format indentation */
#define OUTPUT_XML		0

#define SELECT(a,b) (OUTPUT_XML ? (b) : (a))


/* Indentation */
#define INDENT "\t"

/* Output format configuration
	L1 first level
	L2 second level
	B begin a list of items
	E end a list of items
	P begin an item
	N end an item
*/

/* Output unformatted */
/*
#define L1B "("
#define L1P " "
#define L1N ""
#define L1E ")"
#define L2B "("
#define L2P " "
#define L2N ""
#define L2E ")"
*/

/* Output on one level */
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"
#define L2B " ("
#define L2P " "
#define L2N ""
#define L2E " )"

/* Output on two levels */
/*
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"
#define L2B " (\n"
#define L2P INDENT INDENT
#define L2N "\n"
#define L2E INDENT ")"
*/

/* Print a string in C format */
static void print_c_string(FILE* out, const char* s)
{
	if (!OUTPUT_XML)
	{
		fprintf(out, "\"");
		if (s)
		{
			while (*s)
			{
				switch (*s)
				{
					case '\a' : fprintf(out, "\\a"); break;
					case '\b' : fprintf(out, "\\b"); break;
					case '\f' : fprintf(out, "\\f"); break;
					case '\n' : fprintf(out, "\\n"); break;
					case '\r' : fprintf(out, "\\r"); break;
					case '\t' : fprintf(out, "\\t"); break;
					case '\v' : fprintf(out, "\\v"); break;
					case '\\' : fprintf(out, "\\\\"); break;
					case '\"' : fprintf(out, "\\\""); break;
					default:
						if (*s>=' ' && *s<='~')
							fprintf(out, "%c", *s);
						else
							fprintf(out, "\\x%02x", (unsigned)(unsigned char)*s);
				}
				++s;
			}
		}
		fprintf(out, "\"");
	}
	else
	{
		if (s)
		{
			while (*s)
			{
				switch (*s)
				{
					case '\"' : fprintf(out, "&quot;"); break;
					case '&'  : fprintf(out, "&amp;"); break;
					case '<'  : fprintf(out, "&lt;"); break;
					case '>'  : fprintf(out, "&gt;"); break;
					default:
						if (*s>=' ' && *s<='~')
							fprintf(out, "%c", *s);
						else
							fprintf(out, "&#%d;", (unsigned)(unsigned char)*s);
				}
				++s;
			}
		}
	}
}

/* Print a string in statement format (remove space, parentesis, ") */
static void print_statement_string(FILE* out, const char* s)
{
	if (OUTPUT_XML)
	{
		print_c_string(out, s);
		return;
	}
	if (s)
	{
		while (*s)
		{
			if (isspace(*s))
			{
				fprintf(out, "_");
			}
			else
			{
				switch (*s)
				{
					case '(' :
					case ')' :
					case '"' :
						fprintf(out, "_");
						break;
					default:
						fprintf(out, "%c", *s);
				}
			}
			++s;
		}
	}
	else
	{
		fprintf(out, "null");
	}
}

static void print_game_switch(FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		if ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_NAME)
		{
			int def = input->default_value;
			const char* def_name = 0;

			fprintf(out, SELECT(L1P "dipswitch" L2B, "\t\t<dipswitch>\n"));

			fprintf(out, SELECT(L2P "name ", "\t\t\t<name>"));
			print_c_string(out,input->name);
			fprintf(out, "%s", SELECT(L2N, "</name>\n"));
			++input;

			while ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_SETTING)
			{
				fprintf(out, SELECT(L2P "entry ", "\t\t\t<dipvalue"));
				if (def == input->default_value)
				{
					def_name = input->name;
					fprintf(out, "%s", SELECT("", " default=\"yes\""));
				}
				fprintf(out, "%s", SELECT("", ">\n"));
				fprintf(out, "%s", SELECT("", "\t\t\t\t<name>"));
				print_c_string(out,input->name);
				fprintf(out, "%s", SELECT(L2N, "</name>\n\t\t\t</dipvalue>\n"));
				++input;
			}

			if (def_name && !OUTPUT_XML)
			{
				fprintf(out, L2P "default ");
				print_c_string(out,def_name);
				fprintf(out, "%s", L2N);
			}

			fprintf(out, SELECT(L2E L1N, "\t\t</dipswitch>\n"));
		}
		else
			++input;
	}
}

static void print_game_input(FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;
	int nplayer = 0;
	const char* control = 0;
	int nbutton = 0;
	int ncoin = 0;
	const char* service = 0;
	const char* tilt = 0;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		if ((input->type & ~IPF_MASK) != IPT_EXTENSION)		/* skip analog extension fields */
		{
			switch (input->type & IPF_PLAYERMASK)
			{
				case IPF_PLAYER1:
					if (nplayer<1) nplayer = 1;
					break;
				case IPF_PLAYER2:
					if (nplayer<2) nplayer = 2;
					break;
				case IPF_PLAYER3:
					if (nplayer<3) nplayer = 3;
					break;
				case IPF_PLAYER4:
					if (nplayer<4) nplayer = 4;
					break;
			}
			switch (input->type & ~IPF_MASK)
			{
				case IPT_JOYSTICK_UP:
				case IPT_JOYSTICK_DOWN:
				case IPT_JOYSTICK_LEFT:
				case IPT_JOYSTICK_RIGHT:
					if (input->type & IPF_2WAY)
						control = "joy2way";
					else if (input->type & IPF_4WAY)
						control = "joy4way";
					else
						control = "joy8way";
					break;
				case IPT_JOYSTICKRIGHT_UP:
				case IPT_JOYSTICKRIGHT_DOWN:
				case IPT_JOYSTICKRIGHT_LEFT:
				case IPT_JOYSTICKRIGHT_RIGHT:
				case IPT_JOYSTICKLEFT_UP:
				case IPT_JOYSTICKLEFT_DOWN:
				case IPT_JOYSTICKLEFT_LEFT:
				case IPT_JOYSTICKLEFT_RIGHT:
					if (input->type & IPF_2WAY)
						control = "doublejoy2way";
					else if (input->type & IPF_4WAY)
						control = "doublejoy4way";
					else
						control = "doublejoy8way";
					break;
				case IPT_BUTTON1:
					if (nbutton<1) nbutton = 1;
					break;
				case IPT_BUTTON2:
					if (nbutton<2) nbutton = 2;
					break;
				case IPT_BUTTON3:
					if (nbutton<3) nbutton = 3;
					break;
				case IPT_BUTTON4:
					if (nbutton<4) nbutton = 4;
					break;
				case IPT_BUTTON5:
					if (nbutton<5) nbutton = 5;
					break;
				case IPT_BUTTON6:
					if (nbutton<6) nbutton = 6;
					break;
				case IPT_BUTTON7:
					if (nbutton<7) nbutton = 7;
					break;
				case IPT_BUTTON8:
					if (nbutton<8) nbutton = 8;
					break;
				case IPT_BUTTON9:
					if (nbutton<9) nbutton = 9;
					break;
				case IPT_BUTTON10:
					if (nbutton<10) nbutton = 10;
					break;
				case IPT_PADDLE:
					control = "paddle";
					break;
				case IPT_DIAL:
					control = "dial";
					break;
				case IPT_TRACKBALL_X:
				case IPT_TRACKBALL_Y:
					control = "trackball";
					break;
				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
					control = "stick";
					break;
				case IPT_LIGHTGUN_X:
				case IPT_LIGHTGUN_Y:
					control = "lightgun";
					break;
				case IPT_COIN1:
					if (ncoin < 1) ncoin = 1;
					break;
				case IPT_COIN2:
					if (ncoin < 2) ncoin = 2;
					break;
				case IPT_COIN3:
					if (ncoin < 3) ncoin = 3;
					break;
				case IPT_COIN4:
					if (ncoin < 4) ncoin = 4;
					break;
				case IPT_SERVICE :
					service = "yes";
					break;
				case IPT_TILT :
					tilt = "yes";
					break;
			}
		}
		++input;
	}

	fprintf(out, SELECT(L1P "input" L2B, "\t\t<input"));
	if (OUTPUT_XML)
	{
		if (service)
			fprintf(out, " service=\"%s\"", service );
		if (tilt)
			fprintf(out, " tilt=\"%s\"", tilt );
		fprintf(out, ">\n");
	}
	fprintf(out, SELECT(L2P "players %d" L2N, "\t\t\t<players>%d</players>\n"), nplayer );
	if (control)
		fprintf(out, SELECT(L2P "control %s" L2N, "\t\t\t<control>%s</control>\n"), control );
	if (nbutton)
		fprintf(out, SELECT(L2P "buttons %d" L2N, "\t\t\t<buttons>%d</buttons>\n"), nbutton );
	if (ncoin)
		fprintf(out, SELECT(L2P "coins %d" L2N, "\t\t\t<coins>%d</coins>\n"), ncoin );
	if (!OUTPUT_XML)
	{
		if (service)
			fprintf(out, L2P "service %s" L2N, service );
		if (tilt)
			fprintf(out, L2P "tilt %s" L2N, tilt );
	}
	fprintf(out, SELECT(L2E L1N, "\t\t</input>\n"));
}

static void print_game_rom(FILE* out, const struct GameDriver* game)
{
	const struct RomModule *region, *rom, *chunk;
	const struct RomModule *pregion, *prom, *fprom=NULL;
	extern struct GameDriver driver_0;

	if (!game->rom)
		return;

	if (game->clone_of && game->clone_of != &driver_0)
		fprintf(out, SELECT(L1P "romof %s" L1N, "\t\t<romof>%s</romof>\n"), game->clone_of->name);

	for (region = rom_first_region(game); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			int offset, length, in_parent, is_disk, i;
			char name[100];

			strcpy(name,ROM_GETNAME(rom));
			offset = ROM_GETOFFSET(rom);
			is_disk = ROMREGION_ISDISKDATA(region);

			in_parent = 0;
			length = 0;
			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);

			if (!ROM_NOGOODDUMP(rom) && game->clone_of)
			{
				fprom=NULL;
				for (pregion = rom_first_region(game->clone_of); pregion; pregion = rom_next_region(pregion))
					for (prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
						if (hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(prom), 0))
						{
							if (!fprom || !strcmp(ROM_GETNAME(prom), name))
								fprom=prom;
							in_parent = 1;
						}
			}

			if (!is_disk)
				fprintf(out, SELECT(L1P "rom" L2B, "\t\t<rom"));
			else
				fprintf(out, SELECT(L1P "disk" L2B, "\t\t<disk>\n"));

			if (OUTPUT_XML && !is_disk)
			{
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
					fprintf(out, " nodump=\"yes\"");	
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
					fprintf(out, " baddump=\"yes\"");
				if (ROMREGION_GETFLAGS(region) & ROMREGION_DISPOSE)
					fprintf(out, " dispose=\"yes\"");
				if (ROMREGION_GETFLAGS(region) & ROMREGION_SOUNDONLY)
					fprintf(out, " soundonly=\"yes\"");
				if (ROMREGION_GETFLAGS(region) & ~(ROMREGION_DISPOSE | ROMREGION_SOUNDONLY))
					fprintf(out, " flags=\"0x%x\"", ROMREGION_GETFLAGS(region) & ~(ROMREGION_DISPOSE | ROMREGION_SOUNDONLY));
				fprintf(out, ">\n");
			}

			if (*name)
				fprintf(out, SELECT(L2P "name %s" L2N, "\t\t\t<name>%s</name>\n"), name);
			if (!is_disk && in_parent)
				fprintf(out, SELECT(L2P "merge %s" L2N, "\t\t\t<merge>%s</merge>\n"), ROM_GETNAME(fprom));
			if (!is_disk)
				fprintf(out, SELECT(L2P "size %d" L2N, "\t\t\t<size>%d</size>\n"), length);
			
			if (!OUTPUT_XML)
			{
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
					fprintf(out, L2P "flags nodump" L2N);
				
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
					fprintf(out, L2P "flags baddump" L2N);
			}

			// Dump checksum informatoins only if there is a known dump
			if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
				for (i=0;i<HASH_NUM_FUNCTIONS;i++)
				{
					int func = 1<<i;
					const char* func_name = hash_function_name(func);
					char checksum[1000];

					if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), func, checksum))
					{
						/* I can't use the SELECT() macro here because the number of parameters
						   are different, and GCC whines about it. */
						if (OUTPUT_XML)
							fprintf(out, "\t\t\t<%s>%s</%s>\n", func_name, checksum, func_name);
						else
							fprintf(out, L2P "%s %s" L2N, func_name, checksum);
					}

				}

			switch (ROMREGION_GETTYPE(region))
			{
				case REGION_CPU1: fprintf(out, SELECT(L2P "region cpu1" L2N, "\t\t\t<region>cpu1</region>\n")); break;
				case REGION_CPU2: fprintf(out, SELECT(L2P "region cpu2" L2N, "\t\t\t<region>cpu2</region>\n")); break;
				case REGION_CPU3: fprintf(out, SELECT(L2P "region cpu3" L2N, "\t\t\t<region>cpu3</region>\n")); break;
				case REGION_CPU4: fprintf(out, SELECT(L2P "region cpu4" L2N, "\t\t\t<region>cpu4</region>\n")); break;
				case REGION_CPU5: fprintf(out, SELECT(L2P "region cpu5" L2N, "\t\t\t<region>cpu5</region>\n")); break;
				case REGION_CPU6: fprintf(out, SELECT(L2P "region cpu6" L2N, "\t\t\t<region>cpu6</region>\n")); break;
				case REGION_CPU7: fprintf(out, SELECT(L2P "region cpu7" L2N, "\t\t\t<region>cpu7</region>\n")); break;
				case REGION_CPU8: fprintf(out, SELECT(L2P "region cpu8" L2N, "\t\t\t<region>cpu8</region>\n")); break;
				case REGION_GFX1: fprintf(out, SELECT(L2P "region gfx1" L2N, "\t\t\t<region>gfx1</region>\n")); break;
				case REGION_GFX2: fprintf(out, SELECT(L2P "region gfx2" L2N, "\t\t\t<region>gfx2</region>\n")); break;
				case REGION_GFX3: fprintf(out, SELECT(L2P "region gfx3" L2N, "\t\t\t<region>gfx3</region>\n")); break;
				case REGION_GFX4: fprintf(out, SELECT(L2P "region gfx4" L2N, "\t\t\t<region>gfx4</region>\n")); break;
				case REGION_GFX5: fprintf(out, SELECT(L2P "region gfx5" L2N, "\t\t\t<region>gfx5</region>\n")); break;
				case REGION_GFX6: fprintf(out, SELECT(L2P "region gfx6" L2N, "\t\t\t<region>gfx6</region>\n")); break;
				case REGION_GFX7: fprintf(out, SELECT(L2P "region gfx7" L2N, "\t\t\t<region>gfx7</region>\n")); break;
				case REGION_GFX8: fprintf(out, SELECT(L2P "region gfx8" L2N, "\t\t\t<region>gfx8</region>\n")); break;
				case REGION_PROMS: fprintf(out, SELECT(L2P "region proms" L2N, "\t\t\t<region>proms</region>\n")); break;
				case REGION_SOUND1: fprintf(out, SELECT(L2P "region sound1" L2N, "\t\t\t<region>sound1</region>\n")); break;
				case REGION_SOUND2: fprintf(out, SELECT(L2P "region sound2" L2N, "\t\t\t<region>sound2</region>\n")); break;
				case REGION_SOUND3: fprintf(out, SELECT(L2P "region sound3" L2N, "\t\t\t<region>sound3</region>\n")); break;
				case REGION_SOUND4: fprintf(out, SELECT(L2P "region sound4" L2N, "\t\t\t<region>sound4</region>\n")); break;
				case REGION_SOUND5: fprintf(out, SELECT(L2P "region sound5" L2N, "\t\t\t<region>sound5</region>\n")); break;
				case REGION_SOUND6: fprintf(out, SELECT(L2P "region sound6" L2N, "\t\t\t<region>sound6</region>\n")); break;
				case REGION_SOUND7: fprintf(out, SELECT(L2P "region sound7" L2N, "\t\t\t<region>sound7</region>\n")); break;
				case REGION_SOUND8: fprintf(out, SELECT(L2P "region sound8" L2N, "\t\t\t<region>sound8</region>\n")); break;
				case REGION_USER1: fprintf(out, SELECT(L2P "region user1" L2N, "\t\t\t<region>user1</region>\n")); break;
				case REGION_USER2: fprintf(out, SELECT(L2P "region user2" L2N, "\t\t\t<region>user2</region>\n")); break;
				case REGION_USER3: fprintf(out, SELECT(L2P "region user3" L2N, "\t\t\t<region>user3</region>\n")); break;
				case REGION_USER4: fprintf(out, SELECT(L2P "region user4" L2N, "\t\t\t<region>user4</region>\n")); break;
				case REGION_USER5: fprintf(out, SELECT(L2P "region user5" L2N, "\t\t\t<region>user5</region>\n")); break;
				case REGION_USER6: fprintf(out, SELECT(L2P "region user6" L2N, "\t\t\t<region>user6</region>\n")); break;
				case REGION_USER7: fprintf(out, SELECT(L2P "region user7" L2N, "\t\t\t<region>user7</region>\n")); break;
				case REGION_USER8: fprintf(out, SELECT(L2P "region user8" L2N, "\t\t\t<region>user8</region>\n")); break;
				case REGION_DISKS: fprintf(out, SELECT(L2P "region disks" L2N, "\t\t\t<region>disks</region>\n")); break;
				default: fprintf(out, SELECT(L2P "region 0x%x" L2N, "\t\t\t<region>0x%x</region>\n"), ROMREGION_GETTYPE(region));
		}

		if (!is_disk && !OUTPUT_XML)
		{
			switch (ROMREGION_GETFLAGS(region))
			{
				case 0:
					break;
				case ROMREGION_SOUNDONLY:
					fprintf(out, L2P "flags soundonly" L2N);
					break;
				case ROMREGION_DISPOSE:
					fprintf(out, L2P "flags dispose" L2N);
					break;
				default:
					fprintf(out, L2P "flags 0x%x" L2N, ROMREGION_GETFLAGS(region));
			}
		}
		if (!is_disk)
		{
			fprintf(out, SELECT(L2P "offs %x", "\t\t\t<offset>%x</offset>\n"), offset);
			fprintf(out, SELECT(L2E L1N, "\t\t</rom>\n"));
		}
		else
		{
			fprintf(out, SELECT(L2P "index %x", "\t\t\t<index>%x</index>\n"), DISK_GETINDEX(rom));
			fprintf(out, SELECT(L2E L1N, "\t\t</disk>\n"));
		}
	}
}

static void print_game_sample(FILE* out, const struct GameDriver* game)
{
#if (HAS_SAMPLES)
	struct InternalMachineDriver drv;
	int i;

	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
	{
		const char **samplenames = NULL;
#if (HAS_SAMPLES)
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
#endif
		if (samplenames != 0 && samplenames[0] != 0) {
			int k = 0;
			if (samplenames[k][0]=='*')
			{
				/* output sampleof only if different from game name */
				if (strcmp(samplenames[k] + 1, game->name)!=0)
					fprintf(out, SELECT(L1P "sampleof %s" L1N, "\t\t<sampleof>%s</sampleof>\n"), samplenames[k] + 1);
				++k;
			}
			while (samplenames[k] != 0) {
				/* Check if is not empty */
				if (*samplenames[k]) {
					/* Check if sample is duplicate */
					int l = 0;
					while (l<k && strcmp(samplenames[k],samplenames[l])!=0)
						++l;
					if (l==k)
						fprintf(out, SELECT(L1P "sample %s" L1N, "\t\t<sample name=\"%s\" />\n"), samplenames[k]);
				}
				++k;
			}
		}
	}
#endif
}

static void print_game_micro(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;
	const struct MachineCPU* cpu;
	const struct MachineSound* sound;
	int j;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	for(j=0;j<MAX_CPU;++j)
	{
		if (cpu[j].cpu_type!=0)
		{
			fprintf(out, SELECT(L1P "chip" L2B, "\t\t<chip "));
			if (cpu[j].cpu_flags & CPU_AUDIO_CPU)
				fprintf(out, SELECT(L2P "type cpu flags audio" L2N, "type=\"cpu\" audio=\"yes\">\n"));
			else
				fprintf(out, SELECT(L2P "type cpu" L2N, "type=\"cpu\">\n"));

			fprintf(out, SELECT(L2P "name ", "\t\t\t<name>"));
			print_statement_string(out, cputype_name(cpu[j].cpu_type));
			fprintf(out, "%s", SELECT(L2N, "</name>\n"));

			fprintf(out, SELECT(L2P "clock %d" L2N, "\t\t\t<clock>%d</clock>\n"), cpu[j].cpu_clock);
			fprintf(out, SELECT(L2E L1N, "\t\t</chip>\n"));
		}
	}

	for(j=0;j<MAX_SOUND;++j) if (sound[j].sound_type)
	{
		if (sound[j].sound_type)
		{
			int num = sound_num(&sound[j]);
			int l;

			if (num == 0) num = 1;

			for(l=0;l<num;++l)
			{
				fprintf(out, SELECT(L1P "chip" L2B, "\t\t<chip "));
				fprintf(out, SELECT(L2P "type audio" L2N, "type=\"audio\">\n"));
				fprintf(out, SELECT(L2P "name ", "\t\t\t<name>"));
				print_statement_string(out, sound_name(&sound[j]));
				fprintf(out, "%s", SELECT(L2N, "</name>\n"));
				if (sound_clock(&sound[j]))
					fprintf(out, SELECT(L2P "clock %d" L2N, "\t\t\t<clock>%d</clock>\n"), sound_clock(&sound[j]));
				fprintf(out, SELECT(L2E L1N, "\t\t</chip>\n"));
			}
		}
	}
}

static void print_game_video(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	int dx;
	int dy;
	int ax;
	int ay;
	int showxy;
	int orientation;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, SELECT(L1P "video" L2B, "\t\t<video "));
	if (driver.video_attributes & VIDEO_TYPE_VECTOR)
	{
		fprintf(out, SELECT(L2P "screen vector" L2N, "screen=\"vector\" "));
		showxy = 0;
	}
	else
	{
		fprintf(out, SELECT(L2P "screen raster" L2N, "screen=\"raster\" "));
		showxy = 1;
	}

	if (game->flags & ORIENTATION_SWAP_XY)
	{
		ax = driver.aspect_y;
		ay = driver.aspect_x;
		if (ax == 0 && ay == 0) {
			ax = 3;
			ay = 4;
		}
		dx = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		dy = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		orientation = 1;
	}
	else
	{
		ax = driver.aspect_x;
		ay = driver.aspect_y;
		if (ax == 0 && ay == 0) {
			ax = 4;
			ay = 3;
		}
		dx = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		dy = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		orientation = 0;
	}

	fprintf(out, SELECT(L2P "orientation %s" L2N, "orientation=\"%s\">\n"), orientation ? "vertical" : "horizontal" );
	if (showxy)
	{
		fprintf(out, SELECT(L2P "x %d" L2N, "\t\t\t<width>%d</width>\n"), dx);
		fprintf(out, SELECT(L2P "y %d" L2N, "\t\t\t<height>%d</height>\n"), dy);
	}

	fprintf(out, SELECT(L2P "aspectx %d" L2N, "\t\t\t<aspectx>%d</aspectx>\n"), ax);
	fprintf(out, SELECT(L2P "aspecty %d" L2N, "\t\t\t<aspecty>%d</aspecty>\n"), ay);

	fprintf(out, SELECT(L2P "freq %f" L2N, "\t\t\t<refresh>%f</refresh>\n"), driver.frames_per_second);
	fprintf(out, SELECT(L2E L1N, "\t\t</video>\n"));
}

static void print_game_sound(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;
	const struct MachineCPU* cpu;
	const struct MachineSound* sound;

	/* check if the game have sound emulation */
	int has_sound = 0;
	int i;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	i = 0;
	while (i < MAX_SOUND && !has_sound)
	{
		if (sound[i].sound_type)
			has_sound = 1;
		++i;
	}
	i = 0;
	while (i < MAX_CPU && !has_sound)
	{
		if  ((cpu[i].cpu_flags & CPU_AUDIO_CPU)!=0)
			has_sound = 1;
		++i;
	}

	fprintf(out, SELECT(L1P "sound" L2B, "\t\t<sound>\n"));

	/* sound channel */
	if (has_sound)
	{
		if (driver.sound_attributes & SOUND_SUPPORTS_STEREO)
			fprintf(out, SELECT(L2P "channels 2" L2N, "\t\t\t<channels>2</channels>\n"));
		else
			fprintf(out, SELECT(L2P "channels 1" L2N, "\t\t\t<channels>1</channels>\n"));
	}
	else
		fprintf(out, SELECT(L2P "channels 0" L2N, "\t\t\t<channels>0</channels>\n"));

	fprintf(out, SELECT(L2E L1N, "\t\t</sound>\n"));
}

#define HISTORY_BUFFER_MAX 16384

static void print_game_history(FILE* out, const struct GameDriver* game)
{
	char buffer[HISTORY_BUFFER_MAX];

	if (load_driver_history(game,buffer,HISTORY_BUFFER_MAX)==0)
	{
		fprintf(out, SELECT(L1P "history ", "\t\t<history>\n\t\t"));
		print_c_string(out, buffer);
		fprintf(out, SELECT(L1N, "\n\t\t</history>\n"));
	}
}

static void print_game_driver(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, SELECT(L1P "driver" L2B, "\t\t<driver "));
	if (game->flags & GAME_NOT_WORKING)
		fprintf(out, SELECT(L2P "status preliminary" L2N, "status=\"preliminary\" "));
	else
		fprintf(out, SELECT(L2P "status good" L2N, "status=\"good\" "));

	if (game->flags & GAME_WRONG_COLORS)
		fprintf(out, SELECT(L2P "color preliminary" L2N, "color=\"preliminary\" "));
	else if (game->flags & GAME_IMPERFECT_COLORS)
		fprintf(out, SELECT(L2P "color imperfect" L2N, "color=\"imperfect\" "));
	else
		fprintf(out, SELECT(L2P "color good" L2N, "color=\"good\" "));

	if (game->flags & GAME_NO_SOUND)
		fprintf(out, SELECT(L2P "sound preliminary" L2N, "sound=\"preliminary\""));
	else if (game->flags & GAME_IMPERFECT_SOUND)
		fprintf(out, SELECT(L2P "sound imperfect" L2N, "sound=\"imperfect\""));
	else
		fprintf(out, SELECT(L2P "sound good" L2N, "sound=\"good\""));

	fprintf(out, "%s", SELECT("", ">\n"));

	fprintf(out, SELECT(L2P "palettesize %d" L2N, "\t\t\t<palettesize>%d</palettesize>\n"), driver.total_colors);

	fprintf(out, SELECT(L2E L1N, "\t\t</driver>\n"));
}

/* Print the MAME info record for a game */
static void print_game_info(FILE* out, const struct GameDriver* game)
{

#ifndef MESS
	fprintf(out, SELECT("game" L1B, "\t<game>\n"));
#else
	fprintf(out, SELECT("machine" L1B, "\t<machine>\n"));
#endif

	fprintf(out, SELECT(L1P "name %s" L1N, "\t\t<name>%s</name>\n"), game->name );

	if (game->description)
	{
		fprintf(out, SELECT(L1P "description ", "\t\t<description>"));
		print_c_string(out, game->description );
		fprintf(out, SELECT(L1N, "</description>\n"));
	}

	/* print the year only if is a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, SELECT(L1P "year %s" L1N, "\t\t<year>%s</year>\n"), game->year );

	if (game->manufacturer)
	{
		fprintf(out, SELECT(L1P "manufacturer ", "\t\t<manufacturer>"));
		print_c_string(out, game->manufacturer );
		fprintf(out, SELECT(L1N, "</manufacturer>\n"));
	}

	print_game_history(out,game);

	if (game->clone_of && !(game->clone_of->flags & NOT_A_DRIVER))
		fprintf(out, SELECT(L1P "cloneof %s" L1N, "\t\t<cloneof>%s</cloneof>\n"), game->clone_of->name);

	print_game_rom(out,game);
	print_game_sample(out,game);
	print_game_micro(out,game);
	print_game_video(out,game);
	print_game_sound(out,game);
	print_game_input(out,game);
	print_game_switch(out,game);
	print_game_driver(out,game);

	fprintf(out, SELECT(L1E, "\t</game>\n"));
}

#if !defined(MESS) && !defined(TINY_COMPILE) && !defined(CPSMAME) && !defined(MMSND)
/* Print the resource info */
static void print_resource_info(FILE* out, const struct GameDriver* game)
{
	fprintf(out, SELECT("resource" L1B, "\t<game resource=\"yes\">\n") );

	fprintf(out, SELECT(L1P "name %s" L1N, "\t\t<name>%s</name>\n"), game->name );

	if (game->description)
	{
		fprintf(out, SELECT(L1P "description ", "\t\t<description>"));
		print_c_string(out, game->description );
		fprintf(out, SELECT(L1N, "</description>\n"));
	}

	/* print the year only if it's a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, SELECT(L1P "year %s" L1N, "\t\t<year>%s</year>\n"), game->year );

	if (game->manufacturer)
	{
		fprintf(out, SELECT(L1P "manufacturer ", "\t\t<manufacturer>"));
		print_c_string(out, game->manufacturer );
		fprintf(out, SELECT(L1N, "</manufacturer>\n"));
	}

	print_game_rom(out,game);
	print_game_sample(out,game);

	fprintf(out, SELECT(L1E, "\t</game>\n"));
}

/* Import the driver object and print it as a resource */
#define PRINT_RESOURCE(s) \
	{ \
		extern struct GameDriver driver_##s; \
		print_resource_info( out, &driver_##s ); \
	}

#endif

/* Print all the MAME info database */
void print_mame_info(FILE* out, const struct GameDriver* games[])
{
	int j;

	if (OUTPUT_XML)
	{
		fprintf(out,
			"<?xml version=\"1.0\"?>\n"
#ifndef MESS
			"<!DOCTYPE gamelist [\n"
			"<!ELEMENT gamelist (game+)>\n"
			"\t<!ELEMENT game (name, description, year, manufacturer, history, cloneof, driver, chip*, video, sound, input, dipswitch*, romof, rom*, disk*)>\n"
			"\t\t<!ATTLIST game resource (yes|no) \"no\">\n"
#else
			"<!DOCTYPE machinelist [\n"
			"<!ELEMENT machinelist (machine+)>\n"
			"\t<!ELEMENT machine (name, description, year, manufacturer, history, cloneof, driver, chip*, video, sound, input, dipswitch*, romof, rom*, disk*)>\n"
#endif
			"\t\t<!ELEMENT name (#PCDATA)>\n"
			"\t\t<!ELEMENT description (#PCDATA)>\n"
			"\t\t<!ELEMENT year (#PCDATA)>\n"
			"\t\t<!ELEMENT manufacturer (#PCDATA)>\n"
			"\t\t<!ELEMENT history (#PCDATA)>\n"
			"\t\t<!ELEMENT cloneof (#PCDATA)>\n"
			"\t\t<!ELEMENT driver (palettesize)>\n"
			"\t\t\t<!ATTLIST driver status (good|preliminary) #REQUIRED>\n"
			"\t\t\t<!ATTLIST driver color (good|imperfect|preliminary) #REQUIRED>\n"
			"\t\t\t<!ATTLIST driver sound (good|imperfect|preliminary) #REQUIRED>\n"
			"\t\t\t<!ELEMENT palettesize (#PCDATA)>\n"
			"\t\t<!ELEMENT chip (name, clock)>\n"
			"\t\t\t<!ATTLIST chip type (cpu|audio) #REQUIRED>\n"
			"\t\t\t<!ATTLIST chip audio (yes|no) \"no\">\n"
			"\t\t\t<!ELEMENT name (#PCDATA)>\n"
			"\t\t\t<!ELEMENT clock (#PCDATA)>\n"
			"\t\t<!ELEMENT video (width, height, aspectx, aspecty, refresh)>\n"
			"\t\t\t<!ATTLIST video screen (raster|vector) \"raster\">\n"
			"\t\t\t<!ATTLIST video orientation (raster|vector) #REQUIRED>\n"
			"\t\t\t<!ELEMENT width (#PCDATA)>\n"
			"\t\t\t<!ELEMENT height (#PCDATA)>\n"
			"\t\t\t<!ELEMENT aspectx (#PCDATA)>\n"
			"\t\t\t<!ELEMENT aspecty (#PCDATA)>\n"
			"\t\t\t<!ELEMENT refresh (#PCDATA)>\n"
			"\t\t<!ELEMENT sound (channels)>\n"
			"\t\t\t<!ELEMENT channels (#PCDATA)>\n"
			"\t\t<!ELEMENT input (players, control?, buttons?, coins?)>\n"
			"\t\t\t<!ATTLIST input service (yes|no) \"no\">\n"
			"\t\t\t<!ATTLIST input tilt (yes|no) \"no\">\n"
			"\t\t\t<!ELEMENT players (#PCDATA)>\n"
			"\t\t\t<!ELEMENT control (#PCDATA)>\n"
			"\t\t\t<!ELEMENT buttons (#PCDATA)>\n"
			"\t\t\t<!ELEMENT coins (#PCDATA)>\n"
			"\t\t<!ELEMENT dipswitch (name, dipvalue*)>\n"
			"\t\t\t<!ELEMENT name (#PCDATA)>\n"
			"\t\t\t<!ELEMENT dipvalue (name)>\n"
			"\t\t\t\t<!ATTLIST dipvalue default (yes|no) \"no\">\n"
			"\t\t\t\t<!ELEMENT name (#PCDATA)>\n"
			"\t\t<!ELEMENT romof (#PCDATA)>\n"
			"\t\t<!ELEMENT rom (name?, merge?, size, crc?, md5?, sha1?, region, offset)>\n"
			"\t\t\t<!ATTLIST rom baddump (yes|no) \"no\">\n"
			"\t\t\t<!ATTLIST rom nodump (yes|no) \"no\">\n"
			"\t\t\t<!ATTLIST rom dispose (yes|no) \"no\">\n"
			"\t\t\t<!ATTLIST rom soundonly (yes|no) \"no\">\n"
			"\t\t\t<!ATTLIST rom flags CDATA #IMPLIED>\n"
			"\t\t\t<!ELEMENT name (#PCDATA)>\n"
			"\t\t\t<!ELEMENT merge (#PCDATA)>\n"
			"\t\t\t<!ELEMENT size (#PCDATA)>\n"
			"\t\t\t<!ELEMENT crc (#PCDATA)>\n"
			"\t\t\t<!ELEMENT md5 (#PCDATA)>\n"
			"\t\t\t<!ELEMENT sha1 (#PCDATA)>\n"
			"\t\t\t<!ELEMENT region (#PCDATA)>\n"
			"\t\t\t<!ELEMENT offset (#PCDATA)>\n"
			"\t\t<!ELEMENT disk (name?, md5?, sha1?, region, index)>\n"
			"\t\t\t<!ELEMENT name (#PCDATA)>\n"
			"\t\t\t<!ELEMENT md5 (#PCDATA)>\n"
			"\t\t\t<!ELEMENT sha1 (#PCDATA)>\n"
			"\t\t\t<!ELEMENT region (#PCDATA)>\n"
			"\t\t\t<!ELEMENT index (#PCDATA)>\n"
			"]>\n\n"
#ifndef MESS
			"<gamelist>\n"
#else
			"<machinelist>\n"
#endif
			);
	}

	/* print games */
	for(j=0;games[j];++j)
		print_game_info( out, games[j] );

	/* print the resources (only if linked) */
#if !defined(MESS) && !defined(TINY_COMPILE) && !defined(CPSMAME) && !defined(MMSND)
	PRINT_RESOURCE(neogeo);
#if !defined(NEOMAME)
	PRINT_RESOURCE(cvs);
	PRINT_RESOURCE(decocass);
	PRINT_RESOURCE(playch10);
	PRINT_RESOURCE(pgm);
	PRINT_RESOURCE(skns);
	PRINT_RESOURCE(stvbios);
	PRINT_RESOURCE(konamigx);
#endif
#endif

	if (OUTPUT_XML)
	{
#ifndef MESS
		fprintf(out, "</gamelist>\n");
#else
		fprintf(out, "</machinelist>\n");
#endif
	}
}
