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
/* DSS_INPUT             - Memory Mapped input device                   */
/* DSS_CONSTANT          - Node based constand - Do we need this ???    */
/*                                                                      */
/************************************************************************/

#define DSS_INPUT_SPACE	0x1000

static struct node_description **dss_input_map=NULL;

WRITE_HANDLER(discrete_sound_w)
{
	/* Bring the system upto now */
	discrete_sh_update();
	/* Mask the memory offset to stay in the space allowed */
	offset&=(DSS_INPUT_SPACE-1);
	/* Update the node input value if allowed */
	if(dss_input_map[offset])
	{
		dss_input_map[offset]->input0=data;
	}
}

READ_HANDLER(discrete_sound_r)
{
	int data=0;
	
	discrete_sh_update();
	/* Mask the memory offset to stay in the space allowed */
	offset&=(DSS_INPUT_SPACE-1);
	/* Update the node input value if allowed */
	if(dss_input_map[offset])
	{
		data=dss_input_map[offset]->input0;
	}
    return data;
}


/************************************************************************/
/*                                                                      */
/* DSS_INPUT    - This is a programmable gain module with enable funct  */
/*                                                                      */
/* input0    - Constant value                                           */
/* input1    - Address value                                            */
/* input2    - Address mask                                             */
/* input3    - Gain value                                               */
/* input4    - Offset value                                             */
/* input5    - Starting Position                                        */
/*                                                                      */
/************************************************************************/
int dss_input_step(struct node_description *node)
{
	node->output=(node->input0*node->input3)+node->input4;
	return 0;
}

int dss_input_reset(struct node_description *node)
{
	node->input0=node->input5;
	return 0;
}

int dss_input_init(struct node_description *node)
{
	int loop,addr,mask;
	/* We just allocate memory on the first call */
	if(dss_input_map==NULL)
	{
		if((dss_input_map=malloc(DSS_INPUT_SPACE*sizeof(struct node_description*)))==NULL) return 1;
		memset(dss_input_map,0,DSS_INPUT_SPACE*sizeof(struct node_description*));
	}
	
	/* Initialise the input mapping array for this particular node */
	addr=((int)node->input1)&(DSS_INPUT_SPACE-1);
	mask=((int)node->input2)&(DSS_INPUT_SPACE-1);
	for(loop=0;loop<DSS_INPUT_SPACE;loop++)
	{
		if((loop&mask)==addr) dss_input_map[loop]=node;
	}
	dss_input_reset(node);
	return 0;
}

int dss_input_kill(struct node_description *node)
{
	free(dss_input_map);
	dss_input_map=NULL;
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_CONSTANT - This is a programmable gain module with enable funct  */
/*                                                                      */
/* input0    - Constant value                                           */
/* input1    - NOT USED                                                 */
/* input2    - NOT USED                                                 */
/* input3    - NOT USED                                                 */
/* input4    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dss_constant_step(struct node_description *node)
{
	node->output=node->input0;
	return 0;
}
