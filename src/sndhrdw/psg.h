#ifndef PSG_H
#define PSG_H


#define MAX_PSG 5

/*
** Initialize AY8910 emulator(s).
**
** 'num' is the number of virtual AY8910's to allocate
** 'clock' is master clock rate (Hz)
** 'rate' is sampling rate and 'bufsiz' is the size of the
** buffer that should be updated at each interval
*/
int AYInit(int num, int clk, int rate, int bitsize, int bufsiz, void **buffer );

/*
** shutdown the AY8910 emulators .. make sure that no sound system stuff
** is touching our audio buffers ...
*/
void AYShutdown(void);

/*
** reset all chip registers for AY8910 number 'num'
*/
void AYResetChip(int num);

/*
** called to update all chips; should be called about 50 - 70 times per
** second ... (depends on sample rate and buffer size)
*/
void AYUpdate(void);

void AYUpdateOne(int chip , int endp );

/*
** write 'v' to register 'r' on AY8910 chip number 'n'
*/
void AYWriteReg(int n, int r, int v);

/*
** read register 'r' on AY8910 chip number 'n'
*/
unsigned char AYReadReg(int n, int r);

void AY8910Write(int n, int a, int v);
int AY8910Read(int n, int a);

/*
** set clockrate for one
*/
void AYSetClock(int n,int clk,int rate);

/*
** set output gain
**
** The gain is expressed in 0.2dB increments, e.g. a gain of 10 is an increase
** of 2dB. Note that the gain only affects sounds not playing at full volume,
** since the ones at full volume are already played at the maximum intensity
** allowed by the sound card.
** 0x00 is the default.
** 0xff is the maximum allowed value.
*/
void AYSetGain(int n,int gain);

/*
** You have to provide this function
*/
unsigned char AYPortHandler(int num,int port, int iswrite, unsigned char val);

#endif
