/***************************************************************************

  sn76496.c

  Routines to emulate the Texas Instruments SN76489 and SN76496 programmable
  tone /noise generator.

  This is a very simple chip with no envelope control, therefore unlike the
  AY8910 or Pokey emulators, this module does not create a sample in a
  buffer: it just keeps track of the frequency of the three voices, it's up
  to the caller to generate tone of the required frenquency.

  Noise emulation is not accurate due to lack of documentation. In the chip,
  the noise generator uses a shift register, which can shift at four
  different rates: three of them are fixed, while the fourth is connected
  to the tone generator #3 output, but it's not clear how. It is also
  unknown hoe exactly the shift register works.
  Since it couldn't be accurate, this module doesn't create a sample for the
  noise output. It will only return the shift rate, that is, the sample rate
  the caller should use. It's up to the caller to create a suitable sample;
  a 10K sample filled with rand() will do.

  update 1997.6.21 Tatsuyuki Satoh.
    added SN76496UpdateB() function.

***************************************************************************/

#include "sn76496.h"



void SN76496Reset(struct SN76496 *R)
{
	int i;


	for (i = 0;i < 4;i++) R->Volume[i] = 0;
	for (i = 0;i < 3;i++) R->Frequency[i] = 1000;
	R->NoiseShiftRate = R->Clock/64;

	R->LastRegister = 0;
	for (i = 0;i < 8;i+=2)
	{
		R->Register[i] = 0;
		R->Register[i + 1] = 0x0f;	/* volume = 0 */
	}
	R->NoiseGen = 1;
}



void SN76496Write(struct SN76496 *R,int data)
{
	if (data & 0x80)
	{
		R->LastRegister = (data >> 4) & 0x07;
		R->Register[R->LastRegister] = data & 0x0f;
	}
	else
	{
		R->Register[R->LastRegister] =
				(R->Register[R->LastRegister] & 0x0f) | ((data & 0x3f) << 4);
	}
}

void SN76496Update(struct SN76496 *R)
{
	int i,n;


	for (i = 0;i < 4;i++)
	{
		n = 0x0f - R->Register[2 * i + 1];

		R->Volume[i] = (n << 4) | n;
	}

	for (i = 0;i < 3;i++)
	{
		n = R->Register[2 * i];

		/* if period is 0, shut down the voice */
		if (n == 0) R->Volume[i] = 0;
		else R->Frequency[i] = R->Clock / (32 * n);
	}

        R->Noisemode = R->Register[6] & 0x04;

	n = R->Register[6] & 3;
        switch(n)
        {
        case 0: R->NoiseShiftRate = R->Clock  / 512;break;
        case 1: R->NoiseShiftRate = R->Clock  / 1024;break;
        case 2: R->NoiseShiftRate = R->Clock  / 2048;break;
        default: R->NoiseShiftRate = R->Frequency[2];
        }

}

void SN76496UpdateB(struct SN76496 *R , int rate , char *buffer , int size)
{
	int i,n;
	int incr[4];
	int vb0,vb1,vb2,vbn;
	int l0,l1,l2;
	int c0,c1;

	for (i = 0;i < 4;i++)
	{
		n = 0x0f - R->Register[2 * i + 1];

		R->Volume[i] = (n << 4) | n;
	}

	for (i = 0;i < 3;i++)
	{
		n = R->Register[2 * i];

		/* if period is 0, shut down the voice */
		if (n == 0){
			R->Volume[i] = 0;
			incr[i] = 0;
		}else{
			 R->Frequency[i] = R->Clock / (32 * n);
			incr[i] = 0x10000 * R->Frequency[i] / rate;
		}
	}

        R->Noisemode = R->Register[6] & 4;

	n = R->Register[6] & 3;
        switch (n)
        {
        case 0: R->NoiseShiftRate = R->Clock / 512;break;
        case 1: R->NoiseShiftRate = R->Clock / 1024;break;
        case 2: R->NoiseShiftRate = R->Clock / 2048;break ;
        default: R->NoiseShiftRate = R->Frequency[2];
        }

	incr[3] = (0x10000 * R->NoiseShiftRate) / rate;

	vb0 = (R->Counter[0]&0x8000) ? -R->Volume[0]:R->Volume[0];
	vb1 = (R->Counter[1]&0x8000) ? -R->Volume[1]:R->Volume[1];
	vb2 = (R->Counter[2]&0x8000) ? -R->Volume[2]:R->Volume[2];
	vbn = (R->NoiseGen & 1     ) ? -R->Volume[3]:R->Volume[3];

	/* make buffer */
	for( i = 0 ; i < size ; i++ ){
		if( (l0=vb0) ){
			c0 = R->Counter[0];
			c1 = R->Counter[0] + incr[0];
			if( (c0^c1)&0x8000 ) {
			    l0=l0*(0x8000-(c0&0x7FFF)-(c1&0x7FFF))/incr[0];
				vb0 = -vb0;
			}
			R->Counter[0] = c1 & 0xFFFF;
		}

		if( (l1=vb1) ){
			c0 = R->Counter[1];
			c1 = R->Counter[1] + incr[1];
			if( (c0^c1)&0x8000 ) {
			    l1=l1*(0x8000-(c0&0x7FFF)-(c1&0x7FFF))/incr[1];
				vb1 = -vb1;
			}
			R->Counter[1] = c1 & 0xFFFF;
		}
		if( (l2=vb2) ){
			c0 = R->Counter[2];
			c1 = R->Counter[2] + incr[2];
			if( (c0^c1)&0x8000 ) {
			    l2=l2*(0x8000-(c0&0x7FFF)-(c1&0x7FFF))/incr[2];
				vb2 = -vb2;
			}
			R->Counter[2] = c1 & 0xFFFF;
		}

		if( vbn ){
			R->Counter[3] &= 0xffff;
			if( ( R->Counter[3] += incr[3] ) & 0xffff0000 )
                          {
                                if(!R->Noisemode)
                                {
                                        if(!(i%18)  ) /*VL I guess this should be tied to the clock rate*/
                                                vbn = -vbn;
                                }
                                else
                                {
			    /* The following code is a random bit generator  */
                                        if( ( R->NoiseGen <<= 1 ) & 0x80000000 )
                                        {
                                                R->NoiseGen ^= 0x00040001;
                                                vbn = R->Volume[3];
                                        }
                                        else
                                            {
                                                vbn = -R->Volume[3];
                                            }
                                }  

                        }
		}
		buffer[i] = ( l0 + l1 + l2 + vbn ) / 8;
	}
}
