/*********************************************************************

  unzip.h

  Public functions and globals in unzip.c

*********************************************************************/

/* public functions */
int /* error */ load_zipped_file (const char *zipfile, const char *filename,
	unsigned char **buf, int *length);
int /* error */ checksum_zipped_file (const char *zipfile, const char *filename, int *sum);


/* public globals */
extern int	gUnzipQuiet;	/* flag controls error messages */
