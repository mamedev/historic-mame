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
/* DST_RCFILTER          - Simple RC filter & also lowpass filter       */
/*                                                                      */
/************************************************************************/


/************************************************************************/
/*                                                                      */
/* DST_RCFILTER - Usage of node_description values for RC filter        */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - input value                                              */
/* input2    - Resistor value (initialisation only)                     */
/* input3    - Capacitor Value (initialisation only)                    */
/* input4    - NOT USED                                                 */
/* input5    - Pre-calculated value for exponent                        */
/*                                                                      */
/************************************************************************/
int dst_rcfilter_step(struct node_description *node)
{
	/************************************************************************/
	/* Next Value = PREV + (INPUT_VALUE - PREV)*(1-(EXP(-TIMEDELTA/RC)))    */
	/************************************************************************/

	if(node->input0)
	{
		node->output=node->output+((node->input1-node->output)*(1-exp(node->input5)));
	}
	else
	{
		node->output=0;
	}
	return 0;
}

int dst_rcfilter_reset(struct node_description *node)
{
	node->output=0;
	return 0;
}

int dst_rcfilter_init(struct node_description *node)
{
	node->input5=-1.0/(node->input2*node->input3*Machine->sample_rate);
	/* Initialise the object */
	dst_rcfilter_reset(node);
	return 0;	
}



