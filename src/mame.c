#include <stdio.h>
#include <string.h>
#include "machine.h"
#include "osdepend.h"

#define DEFAULT_NAME "pacman"


FILE *errorlog;


int main(int argc,char **argv)
{
	int i,log;


	log = 0;
	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-log") == 0)
			log = 1;
	}

	if (log) errorlog = fopen("error.log","wa");

	if (init_machine(argc > 1 && argv[1][0] != '-' ? argv[1] : DEFAULT_NAME) == 0)
	{
		printf("\nPLEASE DO NOT DISTRIBUTE THE SOURCE FILES OR THE EXECUTABLE WITH ROM IMAGES.\n"
			   "DOING SO WILL HARM FURTHER EMULATOR DEVELOPMENT AND WILL CONSIDERABLY ANNOY\n"
			   "THE RIGHTFUL COPYRIGHT HOLDERS OF THOSE ROM IMAGES AND CAN RESULT IN LEGAL\n"
			   "ACTION UNDERTAKEN BY EARLIER MENTIONED COPYRIGHT HOLDERS.\n"
			   "\n\n"
			   "Press <ENTER> to continue.\n");

		getchar();

		if (osd_init(argc,argv) == 0)
		{
			if (run_machine(argc > 1 && argv[1][0] != '-' ? argv[1] : DEFAULT_NAME) != 0)
				printf("Unable to start emulation\n");

			osd_exit();
		}
		else printf("Unable to initialize system\n");
	}
	else printf("Unable to initialize machine emulation\n");

	if (errorlog) fclose(errorlog);

	exit(0);
}
