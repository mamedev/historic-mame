/************************************************************************
 *
 *  MAME - Discrete sound system emulation library
 *
 *  Written by Keith Wilkins (mame@esplexo.co.uk)
 *
 *  (c) K.Wilkins 2000
 *
 ***********************************************************************
 *
 * DSS_ADJUSTMENT        - UI Mapped adjustable input
 * DSS_CONSTANT          - Node based constant - Do we need this ???
 * DSS_INPUT_x           - Memory Mapped input device
 *
 ************************************************************************/

/* NOTE: We will cheat and store the value written by discrete_sound_w
 *       in an unused input.  context->data
 */

#define DSS_INPUT__GAIN		(*(node->input[0]))
#define DSS_INPUT__OFFSET	(*(node->input[1]))
#define DSS_INPUT__INIT		(*(node->input[2]))


struct dss_adjustment_context
{
	INT32		port;
	INT32		lastpval;
	INT32		pmin;
	double		pscale;
	double		min;
	double		scale;
	double		lastval;
};


struct dss_input_context
{
	data8_t data;
};


READ8_HANDLER(discrete_sound_r)
{
	struct node_description *node = discrete_find_node(offset);
	data8_t data = 0;

	if (!Machine->sample_rate) return 0;

	discrete_sh_update();

	/* Read the node input value if allowed */
	if (node)
	{
		struct dss_input_context *context = node->context;

		if ((node->module.type >= DSS_INPUT_DATA) && (node->module.type <= DSS_INPUT_PULSE))
		{
			data = context->data;
		}
	}
	else
		discrete_log("discrete_sound_r read from non-existent NODE_%02d\n",offset-NODE_00);

    return data;
}

WRITE8_HANDLER(discrete_sound_w)
{
	struct node_description *node = discrete_find_node(offset);

	if (!Machine->sample_rate) return;

	/* Bring the system up to now */
	discrete_sh_update();

	/* Update the node input value if it's a proper input node */
	if (node)
	{
		struct dss_input_context *context = node->context;

		switch (node->module.type)
		{
			case DSS_INPUT_DATA:
				context->data = data;
				break;
			case DSS_INPUT_LOGIC:
			case DSS_INPUT_PULSE:
				context->data = data ? 1 : 0;
				break;
			case DSS_INPUT_NOT:
				context->data = data ? 0 : 1;
				break;
		}
	}
	else
		discrete_log("discrete_sound_w write to non-existent NODE_%02d\n",offset-NODE_00);
}


/************************************************************************
 *
 * DSS_ADJUSTMENT - UI Adjustable constant node to emulate trimmers
 *
 * input[0]    - Enable
 * input[1]    - Minimum value
 * input[2]    - Maximum value
 * input[3]    - Log/Linear 0=Linear !0=Log
 * input[4]    - Input Port number
 * input[5]    -
 * input[6]    -
 *
 ************************************************************************/
#define DSS_ADJUSTMENT__ENABLE	(*(node->input[0]))
#define DSS_ADJUSTMENT__MIN		(*(node->input[1]))
#define DSS_ADJUSTMENT__MAX		(*(node->input[2]))
#define DSS_ADJUSTMENT__LOG		(*(node->input[3]))
#define DSS_ADJUSTMENT__PORT	(*(node->input[4]))
#define DSS_ADJUSTMENT__PMIN	(*(node->input[5]))
#define DSS_ADJUSTMENT__PMAX	(*(node->input[6]))

void dss_adjustment_step(struct node_description *node)
{
	if (DSS_ADJUSTMENT__ENABLE)
	{
		struct dss_adjustment_context *context = node->context;
		INT32 rawportval = readinputport(context->port);

		/* only recompute if the value changed from last time */
		if (rawportval != context->lastpval)
		{
			double portval = (double)(rawportval - context->pmin) * context->pscale;
			double scaledval = portval * context->scale + context->min;

			context->lastpval = rawportval;
			if (DSS_ADJUSTMENT__LOG == 0)
				context->lastval = scaledval;
			else
				context->lastval = pow(10, scaledval);
		}
		node->output = context->lastval;
	}
	else
	{
		node->output = 0;
	}
}

void dss_adjustment_reset(struct node_description *node)
{
	struct dss_adjustment_context *context = node->context;

	context->port = DSS_ADJUSTMENT__PORT;
	context->lastpval = 0x7fffffff;
	context->pmin = DSS_ADJUSTMENT__PMIN;
	context->pscale = 1.0 / (double)(DSS_ADJUSTMENT__PMAX - DSS_ADJUSTMENT__PMIN);

	/* linear scale */
	if (DSS_ADJUSTMENT__LOG == 0)
	{
		context->min = DSS_ADJUSTMENT__MIN;
		context->scale = DSS_ADJUSTMENT__MAX - DSS_ADJUSTMENT__MIN;
	}

	/* logarithmic scale */
	else
	{
		context->min = log10(DSS_ADJUSTMENT__MIN);
		context->scale = log10(DSS_ADJUSTMENT__MAX) - log10(DSS_ADJUSTMENT__MIN);
	}
	context->lastval = 0;

	dss_adjustment_step(node);
}


/************************************************************************
 *
 * DSS_CONSTANT - This is a constant.
 *
 * input[0]    - Constant value
 *
 ************************************************************************/
#define DSS_CONSTANT__INIT	(*(node->input[0]))

void dss_constant_step(struct node_description *node)
{
	node->output= DSS_CONSTANT__INIT;
}


/************************************************************************
 *
 * DSS_INPUT_x    - Receives input from discrete_sound_w
 *
 * input[0]    - Gain value
 * input[1]    - Offset value
 * input[2]    - Starting Position
 * input[3]    - Current data value
 *
 ************************************************************************/
void dss_input_step(struct node_description *node)
{
	struct dss_input_context *context = node->context;

	node->output = context->data * DSS_INPUT__GAIN + DSS_INPUT__OFFSET;
}

void dss_input_reset(struct node_description *node)
{
	struct dss_input_context *context = node->context;

	switch (node->module.type)
	{
		case DSS_INPUT_DATA:
			context->data = DSS_INPUT__INIT;
			break;
		case DSS_INPUT_LOGIC:
		case DSS_INPUT_PULSE:
			context->data = (DSS_INPUT__INIT == 0) ? 0 : 1;
			break;
		case DSS_INPUT_NOT:
			context->data = (DSS_INPUT__INIT == 0) ? 1 : 0;
			break;
	}
	dss_input_step(node);
}

void dss_input_pulse_step(struct node_description *node)
{
	struct dss_input_context *context = node->context;

	/* Set a valid output */
	node->output = context->data;
	/* Reset the input to default for the next cycle */
	/* node order is now important */
	context->data = DSS_INPUT__INIT;
}
