/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@dysfunction.demon.co.uk)             */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DST_ADDDER            - Multichannel adder                           */
/* DSS_GAIN              - Gain Factor                                  */
/*                                                                      */
/************************************************************************/



/************************************************************************/
/*                                                                      */
/* DSS_ADDER - This is a 4 channel input adder with enable function     */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Channel0 input value                                     */
/* input2    - Channel1 input value                                     */
/* input3    - Channel2 input value                                     */
/* input4    - Channel3 input value                                     */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dst_adder_step(struct node_description *node)
{
	if(node->input0)
	{
		node->output=node->input1 + node->input2 + node->input3 + node->input4;
	}
	else
	{
		node->output=0;
	}
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_GAIN - This is a programmable gain module with enable function   */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Channel0 input value                                     */
/* input2    - Gain value                                               */
/* input3    - NOT USED                                                 */
/* input4    - NOT USED                                                 */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dst_gain_step(struct node_description *node)
{
	if(node->input0)
	{
		node->output=node->input1 * node->input2;
	}
	else
	{
		node->output=0;
	}
	return 0;
}



