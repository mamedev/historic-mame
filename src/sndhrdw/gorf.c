/**************************************************************************

	Gorf/Votrax SC-01 Emulator

 	Mike@Dissfulfils.co.uk

        Modified to match phonemes to words

        Ajudd@quantime.co.uk

**************************************************************************

gorf_sh_start  - Start emulation, load samples from Votrax subdirectory
gorf_sh_stop   - End emulation, free memory used for samples
gorf_sh_w      - Write data to votrax port
gorf_sh_status - Return busy status (-1 = busy)
gorf_port_2_r  - Returns status of voice port
gorf_sh_ update- Null

If you need to alter the base frequency (i.e. Qbert) then just alter
the variable GorfBaseFrequency, this is defaulted to 8000

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"

int	GorfBaseFrequency;		/* Some games (Qbert) change this */
int 	GorfBaseVolume;
int 	GorfChannel = 0;
struct  GameSamples *GorfSamples;

/****************************************************************************
 * 64 Phonemes - currently 1 sample per phoneme, will be combined sometime!
 ****************************************************************************/

static const char *PhonemeTable[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

static const char *GorfWordTable[] =
{
 "A2AYY1","A2E1","UH1GEH1I3N","AE1EH2M","AEM","and.sam","UH1NAH2I1YLA2SHUH2N","AH2NUHTHER","AH1NUHTHVRR",
 "AH1R","UHR","UH1TAEEH3K","avenger.sam","BAEEH3D","BAEEH1D","be.sam",
 "been.sam","BAH2I3Y1T","but.sam","BUH1DTTEH2NN","KUHDEH2T",
 "KAE1NUH1T","captain.sam","chronicles.sam","KO1UH3I3E1N","KO1UH3I3E1NS","colonel.sam",
 "KAH1NKER","KAH1NCHEHSNEHS","DE1FEH1NDER","destroy.sam","DE1STRO1I1Y1D","DYVAH1U1ER",
 "DU1UM","DRAW1S","DUHST","empire.sam","EHND",
 "EH1NEH1MY","EH1SKA1E1P","flagship.sam","FO1R","GUH1LAEKTI1K",
 "GAE1LUH1KSY","general.sam","GDTO1O1RRFF","GDTO1RFYA2N","GDTO1RFE1EH2N","GDTO1RFYA2NS",
 "GAH1EH3T","hahahahu.sam","hahaher.sam","harder.sam","have.sam",
 "hitting.sam","AH1I1Y", "AH1I1Y1","I1MPAH1SI1BL","in.sam","insert.sam",
 "I1S","LI1V","LAWNG","MEE1T","MUU1V",
 "MAH2I1Y","NIR","next.sam","nice.sam","NO",
 "now.sam","PA1","PAH1I1R","PLA1AYER","PRE1PAE1ER","PRI1SI3NEH3RS",
 "promoted.sam","POO1IUSH","RO1U1BAH1T","RO1U1BAH1TS","seek.sam",
 "SHIP","shot.sam","SUHM","SPA2I3YS","PA0",
 "SERVAH2I1Y1VUH3L","TAK","THVUH","THVUH1","time.sam",
 "TU","TRAH2I1Y","UH2NBE1AYTUH3BUH3L","warrior.sam","warriors.sam","WI1L",
 "Y1I3U1","YIUU1U1","YI1U1U1","Y1IUU1U1","Y1I1U1U1","YOR","YU1O1RSEH1LF",
 0
};

#define num_samples (sizeof(GorfWordTable)/sizeof(char *))


/* Total word to join the phonemes together - Global to make it easier to use */
char totalword[256], *totalword_ptr;
char oldword[256];
int plural = 0;

int gorf_sh_start(void)
{
    GorfBaseFrequency = 11025;
    GorfBaseVolume = 230;
    GorfChannel = 0;
    return 0;
}

void gorf_sh_stop(void)
{
}

int gorf_speech_r(int offset)
{
    int Phoneme,Intonation;
    int i = 0;

    Z80_Regs regs;
    int data;

    totalword_ptr = totalword;

    Z80_GetRegs(&regs);
    data = regs.BC.B.h;

    Phoneme = data & 0x3F;
    Intonation = data >> 6;

    if(errorlog) fprintf(errorlog,"Date : %d Speech : %s at intonation %d\n",Phoneme, PhonemeTable[Phoneme],Intonation);

    if(Phoneme==63) {
   		sample_stop(GorfChannel);
                if (errorlog) fprintf(errorlog,"Clearing sample %s\n",totalword);
                totalword[0] = 0;				   /* Clear the total word stack */
                return data;
    }

/* Phoneme to word translation */

    if (strlen(totalword) == 0) {
       strcpy(totalword,PhonemeTable[Phoneme]);	                   /* Copy over the first phoneme */
       if (plural != 0) {
          if (errorlog) fprintf(errorlog,"found a possible plural at %d\n",plural-1);
          if (!strcmp("S",totalword)) {		   /* Plural check */
             sample_start(GorfChannel, num_samples-2, 0);	   /* play the sample at position of word */
             sample_adjust(GorfChannel, GorfBaseFrequency, -1);    /* play at correct rate */
             totalword[0] = 0;				   /* Clear the total word stack */
             oldword[0] = 0;				   /* Clear the total word stack */
             return data;
          } else {
             plural=0;
          }
       }
    } else
       strcat(totalword,PhonemeTable[Phoneme]);	                   /* Copy over the first phoneme */

    if(errorlog) fprintf(errorlog,"Total word = %s\n",totalword);

    for (i=0; GorfWordTable[i]; i++) {
       if (!strcmp(GorfWordTable[i],totalword)) {		   /* Scan the word (sample) table for the complete word */
          if ((!strcmp("GDTO1RFYA2N",totalword)) || (!strcmp("RO1U1BAH1T",totalword)) || (!strcmp("KO1UH3I3E1N",totalword))) {		   /* May be plural */
             plural=i+1;
             strcpy(oldword,totalword);
	     if (errorlog) fprintf(errorlog,"Storing sample position %d and copying string %s\n",plural,oldword);
          } else {
             plural=0;
          }
          sample_start(GorfChannel, i, 0);	                   /* play the sample at position of word */
          sample_adjust(GorfChannel, GorfBaseFrequency, -1);       /* play at correct rate */
          if (errorlog) fprintf(errorlog,"Playing sample %d",i);
          totalword[0] = 0;				   /* Clear the total word stack */
          return data;
       }
    }

    /* Note : We should really also use volume in this as well as frequency */
    return data;				                   /* Return nicely */
}

int gorf_status_r(void)
{
    return !sample_playing(GorfChannel);
}

/* Read from port 2 (0x12) returns speech status as 0x80 */

int gorf_port_2_r(int offset)
{
    int Ans;

    Ans = (input_port_2_r(0) & 0x7F);
    if (gorf_status_r() != 0) Ans += 128;
    return Ans;
}

void gorf_sh_update(void)
{
}
