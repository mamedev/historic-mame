#include "driver.h"
#include "strings.h"
#include "audit.h"

/* 0 error else checksum */
static int CalcCheckSum(void *f, const struct RomModule *romp, int length,
			verify_printf_proc verify_printf)
{
	int sum;
	int xor;
	unsigned int i;
	int j;
	unsigned char *temp;

	length &= ~ROMFLAG_MASK;

	sum = 0;
	xor = 0;

	temp = malloc (length);

	if (!temp)
	{
		verify_printf("Out of memory reading ROM %s\n",romp->name);
		osd_fclose(f);
		exit(1);
	}

	if (osd_fread(f,temp,length) != length)
	{
		verify_printf("Error reading ROM %s (length mismatch)\n", romp->name);
		free(temp);
		return 0;
	}

	for (i = 0;i < length;i+=2)
	{
		j = 256 * temp[i] + temp[i+1];
		sum += j;
		xor ^= j;
	}

	free(temp);

	sum = ((sum & 0xffff) << 16) | (xor & 0xffff);

	return sum;
}


int VerifyRomSet(int game,verify_printf_proc verify_printf)
{
	void *f;
	const struct RomModule *romp;
	const char *name;
	static const struct GameDriver *gamedrv;

	int length;
	int badarchive;
	int expchecksum;
	int checksum;

	gamedrv = drivers[game];
	romp = gamedrv->rom;

	if (!osd_faccess (gamedrv->name,OSD_FILETYPE_ROM))
		return NOTFOUND;

	badarchive = 0;

	while (romp->name || romp->offset || romp->length || romp->checksum )
	{
		/* we're only interested in checksums */
		if (!romp->checksum)
		{
			romp++;
			continue;
		}

		name=romp->name;
		expchecksum = romp->checksum;
		length = 0;

		/* fetch the next entries to see if we're reloading */
		do
		{
			if (romp->name == (char *)-1)
				length = 0;
			length += romp->length & ~ROMFLAG_MASK;
			romp++;
		} while (romp->length && (romp->name == 0 || romp->name == (char *) -1));

		/* look if we can open it */
		f = osd_fopen(gamedrv->name,name,OSD_FILETYPE_ROM,0);

		if (!f)
		{
			verify_printf("%-10s: %-12s %5d bytes   %08x NOT FOUND\n",
					      gamedrv->name, name, length, expchecksum);
			badarchive=1;
		}
		else
		{
			checksum = CalcCheckSum(f, romp, length, verify_printf);
			if (checksum != expchecksum)
			{
				verify_printf("%-10s: %-12s %5d bytes   %08x INCORRECT CHECKSUM\n",
						      gamedrv->name, name, length, expchecksum);
				badarchive = 1;
			}
			osd_fclose(f);
			f = NULL;
		}
	}

	if (!badarchive)
		return CORRECT;
	else
		return INCORRECT;
}


int VerifySampleSet(int game,verify_printf_proc verify_printf)
{
	int j;
	int skipfirst;
	void *f;
	const char *sharedname;
	int exist;
	int badsamples;
	static const struct GameDriver *gamedrv;

	gamedrv = drivers[game];

	/* does the game use samples at all? */
	if (gamedrv->samplenames == 0 || gamedrv->samplenames[0] == 0)
		return CORRECT;

	badsamples = 0;

	/* take care of shared samples */
	if (gamedrv->samplenames[0][0] == '*')
	{
		sharedname=gamedrv->samplenames[0]+1;
		skipfirst = 1;
	}
	else
	{
		sharedname = NULL;
		skipfirst = 0;
	}

	/* do we have samples for this game? */
	exist = osd_faccess (gamedrv->name,OSD_FILETYPE_SAMPLE);

	/* try shared samples */
	if (!exist && skipfirst)
		exist = osd_faccess (sharedname, OSD_FILETYPE_SAMPLE);

	/* if still not found, we're done */
	if (!exist)
		return NOTFOUND;

	for (j = skipfirst; gamedrv->samplenames[j] != 0; j++)
	{
		/* skip empty definitions */
		if (strlen(gamedrv->samplenames[j]) == 0)
			continue;
		f = osd_fopen(gamedrv->name,gamedrv->samplenames[j], OSD_FILETYPE_SAMPLE,0);
		if (f == NULL && skipfirst)
			f = osd_fopen (sharedname, gamedrv->samplenames[j], OSD_FILETYPE_SAMPLE,0);

		if (f)
			osd_fclose(f);
		else
		{
			verify_printf("%-10s: %s NOT FOUND\n",gamedrv->name, gamedrv->samplenames[j]);
			badsamples = 1;
		}
	}
	if (badsamples)
		return INCORRECT;
	else
		return CORRECT;
}
