#include "driver.h"
#include <time.h>

int use_profiler;

void my_textout(char *buf,int x,int y);

struct profile_data
{
	uclock_t start;
	uclock_t count[OSD_TOTAL_PROFILES];
};

struct profile_data profile;

void osd_profiler_init(void)
{
	int i;


	profile.start = uclock();
	for (i = 0;i < OSD_TOTAL_PROFILES;i++)
		profile.count[i] = 0;
}

void osd_profiler(int type)
{
	static int FILO_type[10];
	static uclock_t FILO_start[10];
	static int FILO_length;


	if (!use_profiler) return;

	if (type != OSD_PROFILE_END)
	{
		if (FILO_length >= 10)
		{
if (errorlog) fprintf(errorlog,"Profiler error: FILO buffer overflow\n");
			return;
		}

		if (FILO_length > 0)
		{
			/* handle nested calls */
			profile.count[FILO_type[FILO_length-1]] += uclock() - FILO_start[FILO_length-1];
		}
		FILO_type[FILO_length] = type;
		FILO_start[FILO_length] = uclock();
		FILO_length++;
	}
	else
	{
		if (FILO_length <= 0)
		{
if (errorlog) fprintf(errorlog,"Profiler error: FILO buffer underflow\n");
			return;
		}

		profile.count[FILO_type[FILO_length-1]] += uclock() - FILO_start[FILO_length-1];
		FILO_length--;
		if (FILO_length > 0)
		{
			/* handle nested calls */
			FILO_start[FILO_length-1] = uclock();
		}
	}
}

void osd_profiler_display(void)
{
	int i;
	int total;
	int val;
	char buf[30];
	static char *names[] =
	{
		"CPU 1",
		"CPU 2",
		"CPU 3",
		"CPU 4",
		"Video",
		"Blit ",
		"Vsync",
		"Sound",
		"Mix  ",
		"Ssync",
	};


	if (!use_profiler) return;

	total = (uclock() - profile.start) / (UCLOCKS_PER_SEC / 1000);

	if (total == 0) return;	/* we have been just reset */

	for (i = 0;i < OSD_TOTAL_PROFILES;i++)
	{
		val = profile.count[i] / (UCLOCKS_PER_SEC / 1000);
		sprintf(buf,"%s %3d%%",names[i],val * 100 / total);
		my_textout(buf,0,i*Machine->uifont->height);
	}
}
