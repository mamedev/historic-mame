/*
          FM.C External interface port for MAME
*/

/* For YM2203 / YM2608 */
/* ----- SSG(YM2149/AY-3-8910) emurator interface port  ----- */
/* void SSGClk(int n,int clk,int rate) */
/* void SSGWrite(int n,int a,int v)    */
/* unsigned char SSGRead(int n,int a)  */
/* void SSGReset(int n)                */

/* SSGClk   : Set Clock          */
/* int n    = chip number        */
/* int clk  = MasterClock(Hz)    */
/* int rate = Emuration rate(Hz) */
#define SSGClk(n,clk,rate) AYSetClock(n,clk,rate)

/* SSGWrite : Write SSG port     */
/* int n    = chip number        */
/* int a    = address            */
/* int v    = data               */
#define SSGWrite(n,a,v) AY8910Write(n,a,v)

/* SSGRead  : Read SSG port */
/* int n    = chip number   */
/* int a    = address       */
/* return   = Read data     */
#define SSGRead(n,a) AY8910Read(n,a)

/* SSGReset : Reset SSG chip */
/* int n    = chip number   */
#define SSGReset(n) AYResetChip(n)

/* -------------------- Timer Interface ---------------------*/
#ifndef INTERNAL_TIMER
#define SELF_UPDATE
/* Return :  the position of buffer present      */
/* note : The return value should be FMBufSize or less. */
#define FMUpdatePos() cpu_scalebyfcount(FMBufSize)

#endif

#ifdef BUILD_YM2608
/*******************************************************************************/
/*		YM2608 local section                                                   */
/*******************************************************************************/
/* --------------- External Rhythm(ADPCM) emurator interface port  ---------------*/
/* int n = chip number */
/* int c = rhythm number */
void RhythmStop(int n,int r)
{
	return;
}
void RhythmStart(int n,int r)
{
	return;
}
#endif
