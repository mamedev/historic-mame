/*
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 */


#include "adpcm.h"

/* step size index shift table */
int indsft[8]={-1, -1, -1, -1, 2, 4, 6, 8};

/* step size table:  calculated as   stpsz[i]=floor[16*(11/10)^i] */
int stpsz[49]= {16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,
                73,80,88,97,107,118,130,143,157,173,190,209,230,
                253,279,307,337,371,408,449,494,544,598,658,724,
                796,876,963,1060,1166,1282,1411,1552};

/* nibble to bit map */
int nbl2bit[16][4]={{1,0,0,0},{1,0,0,1},{1,0,1,0},{1,0,1,1},
                    {1,1,0,0},{1,1,0,1},{1,1,1,0},{1,1,1,1},
                    {-1,0,0,0},{-1,0,0,1},{-1,0,1,0},{-1,0,1,1},
                    {-1,1,0,0},{-1,1,0,1},{-1,1,1,0},{-1,1,1,1}};

/* step size index */
int ssindex=0;

/* the current adpcm signalf */
int signalf=-2;



void InitDecoder(void)
{
    ssindex = 0;
    signalf = -2;
}



/*
 *   This function perform ADPCM decode of a given Input block
 *   It reads in input an ADPCM nibble and return decoded difference
 */

int DecodeAdpcm(unsigned char encoded)
{
    int diff,step;

    step = stpsz[ssindex];
    diff = nbl2bit[encoded][0]*(step*nbl2bit[encoded][1]+
                               (step/2)*nbl2bit[encoded][2]+
	                       (step/4)*nbl2bit[encoded][3]+
	                       (step/8));

    ssindex += indsft[(encoded%8)];
    if(ssindex<0) ssindex=0;      /* clip step index */
    if(ssindex>48) ssindex=48;

    return(diff);
}
