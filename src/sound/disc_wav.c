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
/* DSS_SINEWAVE          - Sinewave generator source code               */
/* DSS_SQUAREWAVE        - Squarewave generator source code             */
/*                                                                      */
/************************************************************************/

struct dss_sinewave_context
{
	double phase;
};

struct dss_noise_context
{
	double phase;
};

struct dss_squarewave_context
{
	double phase;
	double trigger;
};


/************************************************************************/
/*                                                                      */
/* DSS_SINWAVE - Usage of node_description values for step function     */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitde input value                                     */
/* input3    - NOT USED                                                 */
/* input4    - NOT USED                                                 */
/* input5    - Starting phase                                           */
/*                                                                      */
/************************************************************************/
int dss_sinewave_step(struct node_description *node)
{
	struct dss_sinewave_context *context=(struct dss_sinewave_context*)node->context;
	double newphase;

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */

	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	newphase=context->phase+((2.0*PI*node->input1)/Machine->sample_rate);
	/* Keep the new phasor in thw 2Pi range.*/
	context->phase=fmod(newphase,2.0*PI);

	if(node->input0)
	{
		node->output=node->input2 * sin(newphase);
	}
	else
	{
		node->output=0;
	}
	return 0;
}

int dss_sinewave_reset(struct node_description *node)
{
	struct dss_sinewave_context *context;
	double start;
	context=(struct dss_sinewave_context*)node->context;
	/* Establish starting phase, convert from degrees to radians */
	start=(node->input5/360.0)*(2.0*PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(node->input5,2.0*PI);
	return 0;
}

int dss_sinewave_init(struct node_description *node)
{
	discrete_log("dss_sinewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_sinewave_context)))==NULL)
	{
		discrete_log("dss_sinewave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_sinewave_context));
	}

	/* Initialise the object */
	dss_sinewave_reset(node);

	return 0;
}

int dss_sinewave_kill(struct node_description *node)
{
	free(node->context);
	node->context=NULL;
	return 0;
}



/************************************************************************/
/*                                                                      */
/* DSS_SQUAREWAVE - Usage of node_description values for step function  */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitude input value                                    */
/* input3    - Duty Cycle                                               */
/* input4    - NOT USED                                                 */
/* input5    - Start Phase                                              */
/*                                                                      */
/************************************************************************/
int dss_squarewave_step(struct node_description *node)
{
	struct dss_squarewave_context *context=(struct dss_squarewave_context*)node->context;
	double newphase;

	/* Establish trigger phase from duty */
	context->trigger=((100-node->input3)/100)*(2.0*PI);

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */

	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	newphase=context->phase+((2.0*PI*node->input1)/Machine->sample_rate);
	/* Keep the new phasor in thw 2Pi range.*/
	context->phase=fmod(newphase,2.0*PI);

	if(node->input0)
	{
		if(context->phase>context->trigger)
			node->output=node->input2;
		else
			node->output=0;
	}
	else
	{
		node->output=0;
	}
	return 0;
}

int dss_squarewave_reset(struct node_description *node)
{
	struct dss_squarewave_context *context;
	double start;
	context=(struct dss_squarewave_context*)node->context;

	/* Establish starting phase, convert from degrees to radians */
	start=(node->input5/360.0)*(2.0*PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(node->input5,2.0*PI);

	return 0;
}

int dss_squarewave_init(struct node_description *node)
{
	discrete_log("dss_squarewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_squarewave_context)))==NULL)
	{
		discrete_log("dss_squarewave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_squarewave_context));
	}

	/* Initialise the object */
	dss_squarewave_reset(node);

	return 0;
}

int dss_squarewave_kill(struct node_description *node)
{
	free(node->context);
	node->context=NULL;
	return 0;
}



/************************************************************************/
/*                                                                      */
/* DSS_NOISE - Usage of node_description values for white nose generator*/
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Noise sample frequency                                   */
/* input2    - Amplitude input value                                    */
/* input3    - NOT USED                                                 */
/* input4    - NOT USED                                                 */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dss_noise_step(struct node_description *node)
{
	struct dss_noise_context *context;
	double newphase;
	context=(struct dss_noise_context*)node->context;

	newphase=context->phase+((2.0*PI*node->input1)/Machine->sample_rate);

	/* Keep the new phasor in thw 2Pi range.*/
	context->phase=fmod(newphase,2.0*PI);

	if(node->input0)
	{
		/* Only sample noise on rollover to next cycle */
		if(newphase>(2.0*PI))
		{
			int newval=rand() & 0x7fff;
			node->output=node->input2*(1-(newval/16384.0));
		}
	}
	else
	{
		node->output=0;
	}
	return 0;
}


int dss_noise_reset(struct node_description *node)
{
	struct dss_noise_context *context=(struct dss_noise_context*)node->context;
	context->phase=0;
	return 0;
}

int dss_noise_init(struct node_description *node)
{
	discrete_log("dss_noise_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_noise_context)))==NULL)
	{
		discrete_log("dss_noise_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_noise_context));
	}

	/* Initialise the object */
	dss_noise_reset(node);

	return 0;
}

int dss_noise_kill(struct node_description *node)
{
	free(node->context);
	node->context=NULL;
	return 0;
}
