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
/* DST_RCDISC            - Simple discharing RC                         */
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
                node->output=node->output+((node->input1-node->output)*node->input5);
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
        node->input5=1-exp(node->input5);
	/* Initialise the object */
	dst_rcfilter_reset(node);
	return 0;
}






/************************************************************************/
/*                                                                      */
/* DST_RCDISC -   Usage of node_description values for RC discharge     */
/*                (inverse slope of DST_RCFILTER)                       */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - input value                                              */
/* input2    - Resistor value (initialisation only)                     */
/* input3    - Capacitor Value (initialisation only)                    */
/* input4    - NOT USED                                                 */
/* input5    - Pre-calculated value for exponent                        */
/*                                                                      */
/************************************************************************/


struct dss_rcdisc_context
{
        int state;
        double t;           // time
        double step;
};

int dst_rcdisc_step(struct node_description *node)
{
        struct dss_rcdisc_context *context;
        context=(struct dss_rcdisc_context*)node->context;

        switch (context->state) {
            case 0:     /* waiting for trigger  */
                if(node->input0) {
                    context->state = 1;
                    context->t = 0;
                }
		node->output=0;
                break;

            case 1:
                if (node->input0) {

                    node->output=node->input1 * exp(context->t / node->input5);
                    context->t += context->step;

                } else {
                    context->state = 0;
                }
        }

	return 0;
}

int dst_rcdisc_reset(struct node_description *node)
{
        struct dss_rcdisc_context *context;
        context=(struct dss_rcdisc_context*)node->context;

	node->output=0;

        context->state = 0;
        context->t = 0;
        context->step = 1.0 / Machine->sample_rate;

	return 0;
}

int dst_rcdisc_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
        if((node->context=malloc(sizeof(struct dss_rcdisc_context)))==NULL)
	{
                discrete_log("dss_rcdisc_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
                memset(node->context,0,sizeof(struct dss_rcdisc_context));
	}
        node->input5=-1.0 * node->input2*node->input3;

	/* Initialise the object */
        dst_rcdisc_reset(node);
	return 0;
}

int dst_rcdisc_kill(struct node_description *node)
{
	free(node->context);
	node->context=NULL;
	return 0;
}
