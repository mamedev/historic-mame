#include <stdio.h>

#ifdef MAME_DEBUG

#include <string.h>

#include "osdepend.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/d68000.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z8000/z8000.h"

int DasmZ80(char *dest,int PC);
int Dasm6502(char *buf,int pc);

#define LOOP_CHECK 32
#define MAX_CPU 10

static int lastPC[MAX_CPU][LOOP_CHECK];
static FILE *traceFile[MAX_CPU];
static int iters[MAX_CPU];
static int loops[MAX_CPU];
static int current=0,total=0;
int traceon=0;

#if 0
void asg_TraceInit(int count)
{
	char buf[100];
	int i;
	for (i=0;i<count;i++)
	{
		sprintf(buf,"temp:CPUTrace%d.log",i);
		traceFile[i]=fopen(buf,"w");
	}
	total=count;
	current=0;
	memset(iters,0,sizeof(iters));
	memset(loops,0,sizeof(loops));
	memset(lastPC,0xff,sizeof(lastPC));
}
#endif

/* JB 980214 */
void asg_TraceInit(int count, char *filename)
{
	char buf[100];
	int i;
	for (i=0;i<count;i++)
	{
		sprintf(buf,"%s.%d",filename,i);
		traceFile[i]=fopen(buf,"w");
	}
	total=count;
	current=0;
	memset(iters,0,sizeof(iters));
	memset(loops,0,sizeof(loops));
	memset(lastPC,0xff,sizeof(lastPC));
}

void asg_TraceKill(void)
{
	int i;
	for (i=0;i<total;i++)
	{
		if (traceFile[i])
			fclose(traceFile[i]);
		traceFile[i]=NULL;
	}
}

FILE *asg_TraceFile (void)
{
	if (traceon)
		return traceFile[current];
	else
		return NULL;
}

void asg_TraceSelect(int indx)
{
	if (traceon && traceFile[current])
	{
		if (loops[current])
			fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
		loops[current]=0;
		fprintf(traceFile[current],"\n=============== End of iteration #%d ===============\n\n",iters[current]++);
		fflush(traceFile[current]);
	}
	else if (!traceon && osd_key_pressed (OSD_KEY_5_PAD))
		traceon = 1;
	if (indx<total)
		current=indx;
}

void asg_Z80Trace(unsigned char *RAM, int PC)
{
	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			DasmZ80 (temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_6809Trace(unsigned char *RAM, int PC)
{
	extern int Dasm6809 (char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm6809 (temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}


void asg_6808Trace(unsigned char *RAM, int PC)
{
	extern int Dasm6808 (unsigned char *pBase, char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm6808 (&RAM[PC],temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}


void asg_6805Trace(unsigned char *RAM, int PC)
{
	extern int Dasm6805 (unsigned char *pBase, char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm6805 (&RAM[PC],temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}


void asg_6502Trace(unsigned char *RAM, int PC)
{
	extern int DAsm(char *S,unsigned short A);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm6502(temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}


void asg_T11Trace(unsigned char *RAM, int PC)
{
	extern int DasmT11 (unsigned char *pBase, char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			DasmT11(&RAM[PC],temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}


void asg_68000Trace(unsigned char *RAM, int PC)
{
	extern int m68000_disassemble(char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			m68000_disassemble(temp,PC);
			fprintf(traceFile[current],"%06X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_8085Trace(unsigned char *RAM, int PC)
{
        extern int Dasm8085 (char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
                                fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
                        Dasm8085(temp,PC);
                        fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_2650Trace(unsigned char *RAM, int PC)
{
extern int Dasm2650 (char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm2650(temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_8039Trace(unsigned char *RAM, int PC)
{
        extern int Dasm8039 (char *buffer, unsigned char* addr);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
                                fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm8039(temp,RAM+PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_34010Trace(unsigned char *RAM, int PC) /* AJP 980802 */
{
	extern int Dasm34010 (unsigned char *pBase, char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm34010 (&RAM[((unsigned int) PC)>>3],temp,PC);
//temp[0]=0;
			fprintf(traceFile[current],"%08X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_I86Trace(unsigned char *RAM, int PC) /* AM 980925 */
{
	extern int DasmI86(unsigned char *pBase, char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			DasmI86 (&RAM[PC],temp,PC);
			fprintf(traceFile[current],"%06X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_9900Trace(unsigned char *RAM, int PC)
{
extern int Dasm9900 (char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			if (loops[current])
				fprintf(traceFile[current],"\n   (loops for %d instructions)\n\n",loops[current]);
			loops[current]=0;
			Dasm9900(temp,PC);
			fprintf(traceFile[current],"%04X: %s\n",PC,temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

void asg_Z8000Trace(unsigned char *RAM, int PC)
{
	extern int DasmZ8000 (char *buffer, int pc);

	if (traceon && traceFile[current])
	{
		char temp[80];
		int count, i;

		// check for loops
		for (i=count=0;i<LOOP_CHECK;i++)
			if (lastPC[current][i]==PC)
				count++;
		if (count>1)
			loops[current]++;
		else
		{
			int n;
			Z8000_Regs r;
			if (loops[current])
				fprintf(traceFile[current],"   (loops for %d instructions)\n",loops[current]);
			loops[current]=0;
			n = DasmZ8000 (temp,PC);
			fprintf(traceFile[current],"%04X:",PC);
			for (i = 0; i < 6; i += 2) {
				if (i < n)
					fprintf(traceFile[current]," %02X%02X",RAM[PC+i+1],RAM[PC+i]);
				else
					fprintf(traceFile[current],"     ");
			}
			Z8000_GetRegs(&r);
			fprintf(traceFile[current]," %c%c%c%c%c%c",
				(r.fcw & 0x80) ? 'C':'.',
				(r.fcw & 0x40) ? 'Z':'.',
				(r.fcw & 0x20) ? 'S':'.',
				(r.fcw & 0x10) ? 'V':'.',
				(r.fcw & 0x08) ? 'D':'.',
				(r.fcw & 0x04) ? 'H':'.');
			fprintf(traceFile[current]," %s\n",temp);
			memmove(&lastPC[current][0],&lastPC[current][1],(LOOP_CHECK-1)*sizeof(int));
			lastPC[current][LOOP_CHECK-1]=PC;
		}
	}
}

#endif
