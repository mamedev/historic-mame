#include "driver.h"
#include "strings.h"
#include "audit.h"


static tAuditRecord *gAudits = NULL;
static tMissingSample *gMissingSamples = NULL;



/* returns 1 if rom is defined in this set */
int RomInSet (const struct GameDriver *gamedrv, unsigned int crc)
{
	const struct RomModule *romp = gamedrv->rom;

	while (romp->name || romp->offset || romp->length)
	{
		romp++;	/* skip ROM_REGION */

		while (romp->length)
		{
			if (romp->crc == crc) return 1;
			do
			{
				romp++;
				/* skip ROM_CONTINUEs and ROM_RELOADs */
			}
			while (romp->length && (romp->name == 0 || romp->name == (char *)-1));
		}
	}
	return 0;
}


/* returns nonzero if romset is missing */
int RomsetMissing (int game)
{
	const struct GameDriver *gamedrv = drivers[game];

	if (gamedrv->clone_of)
	{
		tAuditRecord	*aud;
		int				count;
		int 			i;
		int 			cloneRomsFound = 0;

		if ((count = AuditRomSet (game, &aud)) == 0)
			return 1;

		/* count number of roms found that are unique to clone */
		for (i = 0; i < count; i++)
			if (aud[i].status != AUD_ROM_NOT_FOUND)
				if (!RomInSet (gamedrv->clone_of, aud[i].expchecksum))
					cloneRomsFound++;

		return !cloneRomsFound;
	}
	else
		return !osd_faccess (gamedrv->name, OSD_FILETYPE_ROM);
}


/* Fills in an audit record for each rom in the romset. Sets 'audit' to
   point to the list of audit records. Returns total number of roms
   in the romset (same as number of audit records), 0 if romset missing. */
int AuditRomSet (int game, tAuditRecord **audit)
{
	const struct RomModule *romp;
	const char *name;
	const struct GameDriver *gamedrv;

	int count = 0;
	tAuditRecord *aud;
	int	err;

	if (!gAudits)
		gAudits = (tAuditRecord *)malloc (AUD_MAX_ROMS * sizeof (tAuditRecord));

	if (gAudits)
		*audit = aud = gAudits;
	else
		return 0;


	gamedrv = drivers[game];
	romp = gamedrv->rom;

	/* check for existence of romset */
	if (!osd_faccess (gamedrv->name, OSD_FILETYPE_ROM))
	{
		/* if the game is a clone, check for parent */
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
			aud->explength = 0;
			aud->length = 0;
			aud->expchecksum = romp->crc;
			/* NS981003: support for "load by CRC" */
			aud->checksum = romp->crc;
			count++;

			/* obtain CRC-32 and length of ROM file */
			err = osd_fchecksum (gamedrv->name, name, &aud->length, &aud->checksum);
			if (err && gamedrv->clone_of)
			{
				/* if the game is a clone, try the parent */
				err = osd_fchecksum (gamedrv->clone_of->name, name, &aud->length, &aud->checksum);
				if (err && gamedrv->clone_of->clone_of)
				{
					/* clone of a clone (for NeoGeo clones) */
					err = osd_fchecksum (gamedrv->clone_of->clone_of->name, name, &aud->length, &aud->checksum);
				}
			}

			/* spin through ROM_CONTINUEs and ROM_RELOADs, totaling length */
			do
			{
				if (romp->name != (char *)-1) /* ROM_RELOAD */
					aud->explength += romp->length & ~ROMFLAG_MASK;
				romp++;
			}
			while (romp->length && (romp->name == 0 || romp->name == (char *)-1));

			if (err)
				aud->status = AUD_ROM_NOT_FOUND;
			else if (aud->explength != aud->length)
				aud->status = AUD_LENGTH_MISMATCH;
			else if (aud->checksum != aud->expchecksum)
				aud->status = AUD_BAD_CHECKSUM;
			else
				aud->status = AUD_ROM_GOOD;

			aud++;
		}
	}

	return count;
}


/* Generic function for evaluating a romset. Some platforms may wish to
   call AuditRomSet() instead and implement their own reporting (like MacMAME). */
int VerifyRomSet (int game, verify_printf_proc verify_printf)
{
	tAuditRecord			*aud;
	int						count;
	int						badarchive = 0;
	const struct GameDriver *gamedrv = drivers[game];

	if ((count = AuditRomSet (game, &aud)) == 0)
		return NOTFOUND;

	if (gamedrv->clone_of)
	{
		int i;
		int cloneRomsFound = 0;

		/* count number of roms found that are unique to clone */
		for (i = 0; i < count; i++)
			if (aud[i].status != AUD_ROM_NOT_FOUND)
				if (!RomInSet (gamedrv->clone_of, aud[i].expchecksum))
					cloneRomsFound++;

		if (cloneRomsFound == 0)
			return CLONE_NOTFOUND;
	}

	while (count--)
	{
		switch (aud->status)
		{
			case AUD_ROM_NOT_FOUND:
				if (aud->expchecksum)
					verify_printf ("%-8s: %-12s %7d bytes %08x NOT FOUND\n",
						drivers[game]->name, aud->rom, aud->explength, aud->expchecksum);
				else
					verify_printf ("%-8s: %-12s %7d bytes NOT FOUND (NO GOOD DUMP KNOWN)\n",
						drivers[game]->name, aud->rom, aud->explength, aud->expchecksum);
				badarchive = 1;
				break;
			case AUD_BAD_CHECKSUM:
				if (aud->expchecksum && aud->expchecksum != BADCRC(aud->checksum))
					verify_printf ("%-8s: %-12s %7d bytes %08x INCORRECT CHECKSUM: %08x\n",
						drivers[game]->name, aud->rom, aud->explength, aud->expchecksum, aud->checksum);
				else
					verify_printf ("%-8s: %-12s %7d bytes NO GOOD DUMP KNOWN\n",
						drivers[game]->name, aud->rom, aud->explength);
				badarchive = 1;
				break;
			case AUD_MEM_ERROR:
				verify_printf ("Out of memory reading ROM %s\n", aud->rom);
				badarchive = 1;
				break;
			case AUD_LENGTH_MISMATCH:
				if (aud->expchecksum)
					verify_printf ("%-8s: %-12s %7d bytes %08x INCORRECT LENGTH: %8d\n",
						drivers[game]->name, aud->rom, aud->explength, aud->expchecksum, aud->length);
				else
					verify_printf ("%-8s: %-12s %7d bytes NO GOOD DUMP KNOWN\n",
						drivers[game]->name, aud->rom, aud->explength);
				badarchive = 1;
				break;
			case AUD_ROM_GOOD:
				/* put something here if you want a full accounting of roms */
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
		verify_printf ("%-8s: %s NOT FOUND\n", drivers[game]->name, aud->name);
		aud++;
	}

	return INCORRECT;
}
