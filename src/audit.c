#include "driver.h"
#include "strings.h"
#include "audit.h"


static tAuditRecord *gAudits = NULL;
static tMissingSample *gMissingSamples = NULL;

/* returns nonzero on error */
static int CalcCheckSum (void *f, const struct RomModule *romp, tAuditRecord *aud)
{
	int sum;
	int xor;
	unsigned int i;
	int j;
	unsigned char *temp;

	sum = 0;
	xor = 0;

	do
	{
		int length = romp->length & ~ROMFLAG_MASK;

		if (romp->name != (char *)-1) /* ignore ROM_RELOAD */
		{
			temp = malloc (length);

			if (osd_fread (f, temp, length) != length)
			{
				aud->status = AUD_LENGTH_MISMATCH;
				free (temp);
				return 1;
			}

			for (i = 0;i < length;i+=2)
			{
				j = 256 * temp[i] + temp[i+1];
				sum += j;
				xor ^= j;
			}

			free (temp);
		}
		romp++;
	} while (romp->length && (romp->name == 0 || romp->name == (char *)-1));

	aud->checksum = ((sum & 0xffff) << 16) | (xor & 0xffff);

	return 0;
}


/* Fills in an audit record for each rom in the romset. Sets 'audit' to
   point to the list of audit records. Returns total number of roms
   in the romset (same as number of audit records), 0 if romset missing. */
int AuditRomSet (int game, tAuditRecord **audit)
{
	void *f;
	const struct RomModule *romp;
	const char *name;
	const struct GameDriver *gamedrv;

	int count = 0;
	tAuditRecord *aud;

	if (!gAudits)
		gAudits = (tAuditRecord *)malloc (AUD_MAX_ROMS * sizeof (tAuditRecord));

	if (gAudits)
		*audit = aud = gAudits;
	else
		return 0;


	gamedrv = drivers[game];
	romp = gamedrv->rom;

	if (!osd_faccess (gamedrv->name, OSD_FILETYPE_ROM))
	{
		/* if the game is a clone, try loading the ROM from the main version */
		if (gamedrv->clone_of == 0 ||
				!osd_faccess(gamedrv->clone_of->name,OSD_FILETYPE_ROM))
			return 0;
	}

	while (romp->name || romp->offset || romp->length)
	{
		if (romp->name || romp->length) return 0; /* expecting ROM_REGION */

		romp++;

		while (romp->length)
		{
			if (romp->name == 0)
				return 0;	/* ROM_CONTINUE not preceded by ROM_LOAD */
			else if (romp->name == (char *)-1)
				return 0;	/* ROM_RELOAD not preceded by ROM_LOAD */

			name = romp->name;
			strcpy (aud->rom, name);
			aud->length = romp->length & ~ROMFLAG_MASK;
			aud->expchecksum = romp->checksum;
			aud->checksum = 0;
			count++;

			/* look if we can open it */
			f = osd_fopen (gamedrv->name, name, OSD_FILETYPE_ROM, 0);
			if (f == 0 && gamedrv->clone_of)
			{
				/* if the game is a clone, try loading the ROM from the main version */
				f = osd_fopen (gamedrv->clone_of->name, name, OSD_FILETYPE_ROM, 0);
			}

			if (!f)
				aud->status = AUD_ROM_NOT_FOUND;
			else
			{
				aud->length = romp->length & ~ROMFLAG_MASK;
				if (!CalcCheckSum (f, romp, aud))
				{
					if (aud->checksum != aud->expchecksum)
						aud->status = AUD_BAD_CHECKSUM;
					else
						aud->status = AUD_ROM_GOOD;
				}

				osd_fclose (f);
			}

			do
			{
				romp++;
			}
			while (romp->length && (romp->name == 0 || romp->name == (char *)-1));

			aud++;
		}
	}

	return count;
}


/* Generic function for evaluating a romset. Some platforms may wish to
   call AuditRomSet() instead and implement their own reporting (like MacMAME). */
int VerifyRomSet (int game, verify_printf_proc verify_printf)
{
	tAuditRecord	*aud;
	int				count;
	int				badarchive = 0;

	if ((count = AuditRomSet (game, &aud)) == 0)
		return NOTFOUND;

	while (count--)
	{
		switch (aud->status)
		{
			case AUD_ROM_NOT_FOUND:
				verify_printf ("%-10s: %-12s %5d bytes   %08x NOT FOUND\n",
					drivers[game]->name, aud->rom, aud->length, aud->expchecksum);
				badarchive = 1;
				break;
			case AUD_BAD_CHECKSUM:
				verify_printf ("%-10s: %-12s %5d bytes   %08x INCORRECT CHECKSUM %08x\n",
					drivers[game]->name, aud->rom, aud->length, aud->expchecksum, aud->checksum);
				badarchive = 1;
				break;
			case AUD_MEM_ERROR:
				verify_printf ("Out of memory reading ROM %s\n", aud->rom);
				badarchive = 1;
				break;
			case AUD_LENGTH_MISMATCH:
				verify_printf ("Error reading ROM %s (length mismatch)\n", aud->rom);
				badarchive = 1;
				break;
			case AUD_ROM_GOOD:
				break;
		}
		aud++;
	}

	if (badarchive)
		return INCORRECT;
	else
		return CORRECT;
}


/* Builds a list of every missing sample. Returns total number of missing
   samples, or -1 if no samples were found. Sets audit to point to the
   list of missing samples. */
int AuditSampleSet (int game, tMissingSample **audit)
{
	int skipfirst;
	void *f;
	const char *sharedname;
	int exist;
	static const struct GameDriver *gamedrv;
	int j;
	int count = 0;
	tMissingSample *aud;

	gamedrv = drivers[game];

	/* does the game use samples at all? */
	if (gamedrv->samplenames == 0 || gamedrv->samplenames[0] == 0)
		return 0;

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
	exist = osd_faccess (gamedrv->name, OSD_FILETYPE_SAMPLE);

	/* try shared samples */
	if (!exist && skipfirst)
		exist = osd_faccess (sharedname, OSD_FILETYPE_SAMPLE);

	/* if still not found, we're done */
	if (!exist)
		return -1;

	/* allocate missing samples list (if necessary) */
	if (!gMissingSamples)
		gMissingSamples = (tMissingSample *)malloc (AUD_MAX_SAMPLES * sizeof (tMissingSample));

	if (gMissingSamples)
		*audit = aud = gMissingSamples;
	else
		return 0;

	for (j = skipfirst; gamedrv->samplenames[j] != 0; j++)
	{
		/* skip empty definitions */
		if (strlen (gamedrv->samplenames[j]) == 0)
			continue;
		f = osd_fopen (gamedrv->name, gamedrv->samplenames[j], OSD_FILETYPE_SAMPLE, 0);
		if (f == NULL && skipfirst)
			f = osd_fopen (sharedname, gamedrv->samplenames[j], OSD_FILETYPE_SAMPLE, 0);

		if (f)
			osd_fclose(f);
		else
		{
			strcpy (aud->name, gamedrv->samplenames[j]);
			count++;
			aud++;
		}
	}
	return count;
}


/* Generic function for evaluating a sampleset. Some platforms may wish to
   call AuditSampleSet() instead and implement their own reporting (like MacMAME). */
int VerifySampleSet (int game, verify_printf_proc verify_printf)
{
	tMissingSample	*aud;
	int				count;

	count = AuditSampleSet (game, &aud);
	if (count==-1)
		return NOTFOUND;
	else if (count==0)
		return CORRECT;

	/* list missing samples */
	while (count--)
	{
		verify_printf ("%-10s: %s NOT FOUND\n", drivers[game]->name, aud->name);
		aud++;
	}

	return INCORRECT;
}
