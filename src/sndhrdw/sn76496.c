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

	R->NoiseShiftRate = R->Clock/64;

	n = R->Register[6] & 3;
	if (n == 3)
	{
		static int count;


		/* kludge just to play something */
		count++;
		if (count > 2) count = 0;
		R->NoiseShiftRate = 64 << count;
	}
	else R->NoiseShiftRate = 64 << n;
}
