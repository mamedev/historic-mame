#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <sys/stat.h>
#include <sys/errno.h>

#define MAX_FILES 100
#define MAX_FILENAME_LEN 12	/* increase this if you are using a real OS... */


/* compare modes when one file is twice as long as the other */
/* A = All file */
/* 1 = 1st half */
/* 2 = 2nd half */
/* E = Even bytes */
/* O = Odd butes */
enum {	MODE_A_A, MODE_1_A, MODE_2_A, MODE_E_A, MODE_O_A,
		          MODE_A_1, MODE_A_2, MODE_A_E, MODE_A_O,
		TOTAL_MODES };
char *modenames[][2] =
{
	{ "          ", "          " },
	{ "[1st half]", "          " },
	{ "[2nd half]", "          " },
	{ "[even]    ", "          " },
	{ "[odd]     ", "          " },
	{ "          ", "[1st half]" },
	{ "          ", "[2nd half]" },
	{ "          ", "[even]    " },
	{ "          ", "[odd]     " }
};

struct fileinfo
{
	char name[MAX_FILENAME_LEN+1];
	int size;
	unsigned char *buf;	/* file is read in here */
	int listed;
};

struct fileinfo files[2][MAX_FILES];
float matchscore[MAX_FILES][MAX_FILES][TOTAL_MODES];


void checkintegrity(const struct fileinfo *file,int side)
{
	int i;
	int mask0,mask1;


	if (file->buf == 0) return;

	mask0 = 0x00;
	mask1 = 0xff;

	for (i = 0;i < file->size;i++)
	{
		mask0 |= file->buf[i];
		mask1 &= file->buf[i];
	}

	if (mask0 != 0xff || mask1 != 0x00)
	{
		int allfixed;

		allfixed = 1;
		printf("%-23s %-23s SOME FIXED BITS (",side ? "" : file->name,side ? file->name : "");
		for (i = 0;i < 8;i++)
		{
			if ((mask0 & 0x80) == 0) printf("0");
			else if (mask1 & 0x80) printf("1");
			else
			{
				printf("x");
				allfixed = 0;
			}
			mask0 <<= 1;
			mask1 <<= 1;
		}
		printf(")\n");

		/* if the file contains a fixed value, we don't need to do the other */
		/* validity checks */
		if (allfixed) return;
	}

	for (i = 0;i < file->size/2;i++)
	{
		if (file->buf[i] != file->buf[file->size/2 + i]) break;
	}

	if (i == file->size/2)
		printf("%-23s %-23s FIRST AND SECOND HALF IDENTICAL\n",side ? "" : file->name,side ? file->name : "",mask0);
	else
	{
		for (i = 0;i < file->size/4;i++)
		{
			if (file->buf[file->size/2 + 2*i+1] != 0xff) break;
			if (file->buf[2*i+1] != file->buf[file->size/2 + 2*i]) break;
		}

		if (i == file->size/4)
			printf("%-23s %-23s BAD NEOGEO DUMP - CUT 2ND HALF\n",side ? "" : file->name,side ? file->name : "",mask0);
	}
}


float filecompare(const struct fileinfo *file1,const struct fileinfo *file2,int mode)
{
	int i;
	int match = 0;
	float score = 0.0;


	if (file1->buf == 0 || file2->buf == 0) return 0.0;

	if (mode >= MODE_A_1 && mode <= MODE_A_O)
	{
		const struct fileinfo *temp;
		mode -= 4;

		temp = file1;
		file1 = file2;
		file2 = temp;
	}

	if (mode == MODE_A_A)
	{
		if (file1->size != file2->size) return 0.0;
	}
	else if (mode >= MODE_1_A && mode <= MODE_O_A)
	{
		if (file1->size != 2*file2->size) return 0.0;
	}

	switch (mode)
	{
		case MODE_A_A:
			for (i = 0;i < file1->size;i++)
			{
				if (file1->buf[i] == file2->buf[i]) match++;
			}
			score = (float)match/file1->size;
			break;

		case MODE_1_A:
			for (i = 0;i < file2->size;i++)
			{
				if (file1->buf[i] == file2->buf[i]) match++;
			}
			score = (float)match/file2->size;
			break;

		case MODE_2_A:
			for (i = 0;i < file2->size;i++)
			{
				if (file1->buf[i + file2->size] == file2->buf[i]) match++;
			}
			score = (float)match/file2->size;
			break;

		case MODE_E_A:
			for (i = 0;i < file2->size;i++)
			{
				if (file1->buf[2*i] == file2->buf[i]) match++;
			}
			score = (float)match/file2->size;
			break;

		case MODE_O_A:
			for (i = 0;i < file2->size;i++)
			{
				if (file1->buf[2*i+1] == file2->buf[i]) match++;
			}
			score = (float)match/file2->size;
			break;
	}


	return score;
}


void readfile(const char *path,struct fileinfo *file)
{
	char fullname[256];
	FILE *f = 0;


	if (path)
	{
		strcpy(fullname,path);
		strcat(fullname,"/");
	}
	else fullname[0] = 0;
	strcat(fullname,file->name);

	if ((file->buf = malloc(file->size)) == 0)
	{
		printf("%s: out of memory!\n",file->name);
		return;
	}

	if ((f = fopen(fullname,"rb")) == 0)
	{
		printf("%s: %s\n",fullname,strerror(errno));
		return;
	}

	if (fread(file->buf,1,file->size,f) != file->size)
	{
		printf("%s: %s\n",fullname,strerror(errno));
		fclose(f);
		return;
	}

	fclose(f);

	return;
}


void freefile(struct fileinfo *file)
{
	free(file->buf);
	file->buf = 0;
}


void printname(const struct fileinfo *file1,const struct fileinfo *file2,float score,int mode)
{
	printf("%-12s %s %-12s %s ",file1 ? file1->name : "",modenames[mode][0],file2 ? file2->name : "",modenames[mode][1]);
	if (score == 0.0) printf("NO MATCH\n");
	else if (score == 1.0) printf("IDENTICAL\n");
	else printf("%3.3f%%\n",score*100);
}


int main(int argc,char **argv)
{
	struct stat s1,s2;


	if (argc < 2)
	{
		printf("usage: romcmp dir1 [dir2]\n");
		return 0;
	}

	if (stat(argv[1],&s1) != 0)
	{
		printf("%s: %s\n",argv[1],strerror(errno));
		return 10;
	}

	if (!S_ISDIR(s1.st_mode))
	{
		printf("%s not a directory\n",argv[1]);
		return 10;
	}

	if (argc >= 3)
	{
		if (stat(argv[2],&s2) != 0)
		{
			printf("%s: %s\n",argv[2],strerror(errno));
			return 10;
		}

		if (!S_ISDIR(s2.st_mode))
		{
			printf("%s not a directory\n",argv[2]);
			return 10;
		}
	}

	{
		char buf[100];
		struct find_t f;
		int found[2];
		int i,j,mode;
		int besti,bestj;


		found[0] = found[1] = 0;
		for (i = 0;i < 2;i++)
		{
			if (argc > i+1)
			{
				strcpy(buf,argv[i+1]);
				strcat(buf,"/*.*");
				if (_dos_findfirst(buf,_A_NORMAL | _A_RDONLY,&f) == 0)
				{
					do
					{
						int size;

						size = f.size;
						while (size && (size & 1) == 0) size >>= 1;
						if (size & ~1)
							printf("%-23s %-23s ignored (not a ROM)\n",i ? "" : f.name,i ? f.name : "");
						else
						{
							strcpy(files[i][found[i]].name,f.name);
							files[i][found[i]].size = f.size;
							readfile(argv[i+1],&files[i][found[i]]);
							files[i][found[i]].listed = 0;
							if (found[i] >= MAX_FILES)
							{
								printf("%s: max of %d files exceeded\n",argv[i+1],MAX_FILES);
								break;
							}
							found[i]++;
						}
					} while (_dos_findnext(&f) == 0);
				}
			}
		}

		if (argc >= 3)
			printf("%d and %d files\n",found[0],found[1]);
		else
			printf("%d files\n",found[0]);

		for (i = 0;i < found[0];i++)
		{
			checkintegrity(&files[0][i],0);
		}

		for (j = 0;j < found[1];j++)
		{
			checkintegrity(&files[1][j],1);
		}

		if (argc < 3)
		{
			/* find duplicates in one dir */
			for (i = 0;i < found[0];i++)
			{
				for (j = i+1;j < found[0];j++)
				{
					for (mode = 0;mode < TOTAL_MODES;mode++)
					{
						if (filecompare(&files[0][i],&files[0][j],mode) == 1.0)
							printname(&files[0][i],&files[0][j],1.0,mode);
					}
				}
			}
		}
		else
		{
			/* compare two dirs */
			for (i = 0;i < found[0];i++)
			{
				for (j = 0;j < found[1];j++)
				{
					for (mode = 0;mode < TOTAL_MODES;mode++)
					{
						matchscore[i][j][mode] = filecompare(&files[0][i],&files[1][j],mode);
					}
				}
			}

			do
			{
				float bestscore;
				int bestmode;

				besti = -1;
				bestj = -1;
				bestscore = 0.0;
				bestmode = -1;

				for (mode = 0;mode < TOTAL_MODES;mode++)
				{
					for (i = 0;i < found[0];i++)
					{
						for (j = 0;j < found[1];j++)
						{
							if (matchscore[i][j][mode] > bestscore)
							{
								bestscore = matchscore[i][j][mode];
								besti = i;
								bestj = j;
								bestmode = mode;
							}
						}
					}
				}

				if (besti != -1)
				{
					printname(&files[0][besti],&files[1][bestj],bestscore,bestmode);
					files[0][besti].listed = 1;
					files[1][bestj].listed = 1;

					matchscore[besti][bestj][bestmode] = 0.0;
					for (mode = 0;mode < TOTAL_MODES;mode++)
					{
						if (bestmode == MODE_A_A || mode == bestmode ||
								((mode-1)&~1) != ((bestmode-1)&~1))
						{
							for (i = 0;i < found[0];i++)
							{
								if (matchscore[i][bestj][mode] < bestscore)
									matchscore[i][bestj][mode] = 0.0;
							}
							for (j = 0;j < found[1];j++)
							{
								if (matchscore[besti][j][mode] < bestscore)
									matchscore[besti][j][mode] = 0.0;
							}
						}
						if (files[0][besti].size > files[1][bestj].size)
						{
							for (i = 0;i < found[0];i++)
							{
								if (matchscore[i][bestj][mode] < bestscore)
									matchscore[i][bestj][mode] = 0.0;
							}
						}
						else
						{
							for (j = 0;j < found[1];j++)
							{
								if (matchscore[besti][j][mode] < bestscore)
									matchscore[besti][j][mode] = 0.0;
							}
						}
					}
				}
			} while (besti != -1);


			for (i = 0;i < found[0];i++)
			{
				if (files[0][i].listed == 0) printname(&files[0][i],0,0.0,0);
				freefile(&files[0][i]);
			}
			for (i = 0;i < found[1];i++)
			{
				if (files[1][i].listed == 0) printname(0,&files[1][i],0.0,0);
				freefile(&files[1][i]);
			}
		}
	}

	return 0;
}
