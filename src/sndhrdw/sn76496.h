struct SN76496
{
	/* set this before calling SN76496Reset() */
	int Clock;	/* chip clock in Hz */

	/* read these after calling SN76496Update(), and produce the required sounds */
	int Volume[4];	/* volume of voice 0-2 and noise. Range is 0-255 */
	int Frequency[3];	/* tone frequency in Hz */
	int NoiseShiftRate;	/* sample rate for the noise sample */

	/* private */
	int Register[8];
	int LastRegister;
	/* buffer mode */
	int Counter[4];
	int NoiseGen;
};



void SN76496Reset(struct SN76496 *R);
void SN76496Write(struct SN76496 *R,int data);
void SN76496Update(struct SN76496 *R);
void SN76496UpdateB(struct SN76496 *R , int rate , char *buffer , int size);
