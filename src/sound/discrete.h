#ifndef _discrete_h_
#define _discrete_h_

/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@dysfunction.demon.co.uk)             */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/*  Coding started in November 2000                                     */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* Unused/Unconnected input nodes should be set to NODE_NC (No Connect) */
/*                                                                      */
/* Each node can have upto 6 inputs from either constants or other      */
/* nodes within the system.                                             */
/*                                                                      */
/* It should be remembered that the discrete sound system emulation     */
/* does not do individual device emulation, but instead does a function */
/* emulation. So you will need to convert the schematic design into     */
/* a logic block representation.                                        */
/*                                                                      */
/* One node point may feed a number of inputs, for example you could    */
/* connect the output of a DISCRETE_SINEWAVE to the AMPLITUDE input     */
/* of another DISCRETE_SINEWAVE to amplitude modulate its output and    */
/* also connect it to the frequecy input of another to frequency        */
/* modulate its output, the combinations are endless....                */
/*                                                                      */
/* Consider the circuit below:                                          */
/*                                                                      */
/*   --------               ----------                   -------        */
/*  |        |             |          |                 |       |       */
/*  | SQUARE |       Enable| SINEWAVE |                 |       |       */
/*  | WAVE   |------------>|  2000Hz  |---------------->|       |       */
/*  |        | |           |          |                 | ADDER |--}OUT */
/*  | NODE01 | |           |  NODE02  |                 |       |       */
/*   --------  |            ----------                ->|       |       */
/*             |                                     |  |NODE06 |       */
/*             |   ------     ------     ---------   |   -------        */
/*             |  |      |   |      |   |         |  |       ^          */
/*             |  | INV  |Ena| ADD  |Ena| SINEWVE |  |       |          */
/*              ->| ERT  |-->| ER2  |-->| 4000Hz  |--    -------        */
/*                |      |ble|      |ble|         |     |       |       */
/*                |NODE03|   |NODE04|   | NODE05  |     | INPUT |       */
/*                 ------     ------     ---------      |       |       */
/*                                                      |NODE07 |       */
/*                                                       -------        */
/*                                                                      */
/* This should give you an alternating two tone sound switching         */
/* between the 2000Hz and 4000Hz sine waves at the frequency of the     */
/* square wave, with the memory mapped enable signal mapped onto NODE07 */
/* so discrete_sound_w(NODE_06,1) will enable the sound, and            */
/* discrete_sound_w(NODE_06,0) will disable the sound.                  */
/*                                                                      */
/*  DISCRETE_SOUND_START(test_interface)                                */
/*      DISCRETE_SQUAREWAVE(NODE_01,1,0.5,1,50,0)                       */
/*      DISCRETE_SINEWAVE(NODE_02,NODE_01,2000,10000,0)                 */
/*      DISCRETE_INVERT(NODE_03,NODE_01)                                */
/*      DISCRETE_ADDER2(NODE_04,1,NODE_03,1)                            */
/*      DISCRETE_SINEWAVE(NODE_05,NODE_04,4000,10000,0)                 */
/*      DISCRETE_ADDER2(NODE_06,NODE_07,NODE_02,NODE_05)                */
/*      DISCRETE_INPUT(NODE_07,1)                                       */
/*      DISCRETE_OUTPUT(NODE_06)                                        */
/*  DISCRETE_SOUND_END                                                  */
/*                                                                      */
/* To aid simulation speed it is preferable to use the enable/disable   */
/* inputs to a block rather than setting the output amplitude to zero   */
/*                                                                      */
/* Feedback loops are allowed BUT they will always feeback one time     */
/* step later, the loop over the netlist is only performed once per     */
/* deltaT so feedback occurs in the next deltaT step. This is not       */
/* the perfect solution but saves repeatedly traversing the netlist     */
/* until all nodes have settled.                                        */
/*                                                                      */
/* It will often be necessary to add gain and level shifting blocks to  */
/* correct the signal amplitude and offset to the required level        */
/* as all sine waves have 0 offset and swing +ve/-ve so if you want     */
/* to use this as a frequency modulation you need to offset it so that  */
/* it always stays positive, use the DISCRETE_ADDER & DISCRETE_GAIN     */
/* modules to do this. You will also need to do this to scale and off-  */
/* set your memory mapped device inputs if they are being used to set   */
/* amplitude and frequency values.                                      */
/*                                                                      */
/* The best way to work out your system is generally to use a pen and   */
/* paper to draw a logical block diagram like the one above, it helps   */
/* to understand the system ,map the inputs and outputs and to work     */
/* out your node numbering scheme.                                      */
/*                                                                      */
/* Node numbers NODE_01 to NODE_99 are defined at present.              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* LIST OF CURRENTLY IMPLEMENTED DISCRETE BLOCKS                        */
/* ---------------------------------------------                        */
/*                                                                      */
/* DISCRETE_SOUND_START(STRUCTURENAME)                                  */
/* DISCRETE_SOUND_END                                                   */
/*                                                                      */
/* DISCRETE_CONSTANT(NODE,CONST0)                                       */
/* DISCRETE_INPUT(NODE,INIT0,ADDR,MASK)                                 */
/* DISCRETE_SQUAREWAVE(NODE,IN0,IN1,IN2,IN3,PHASE)                      */
/* DISCRETE_SINEWAVE(NODE,IN0,IN1,IN2,PHASE)                            */
/*                                                                      */
/* DISCRETE_INVERT(NODE,IN0)                                            */
/* DISCRETE_GAIN(NODE,IN0,IN1)                                          */
/* DISCRETE_ONOFF(NODE,IN0,IN1)                                         */
/* DISCRETE_ADDER2(NODE,IN0,IN1,IN2)                                    */
/* DISCRETE_ADDER3(NODE,IN0,IN1,IN2,IN3)                                */
/*                                                                      */
/* DISCRETE_RCFILTER(NODE,ENAB,IN0,RVAL,CVAL)                           */
/*                                                                      */
/* DISCRETE_NE555(NODE,RESET,TRIGR,THRSH,CTRLV,VCC)                     */
/*                                                                      */
/* DISCRETE_OUTPUT(OPNODE)                                              */
/* DISCRETE_OUTPUT_STEREO(OPNODEL,OPNODER)                              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_CONSTANT - Single output, fixed at compile time             */
/*                                                                      */
/*                         ----------                                   */
/*                        |          |                                  */
/*                        | CONSTANT |--------}   Netlist node          */
/*                        |          |                                  */
/*                         ----------                                   */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_CONSTANT(name of node, constant value)                  */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_CONSTANT(NODE_01, 100)                                  */
/*                                                                      */
/*  Define a node that has a constant value of 100                      */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE INPUT - Single output node, initialised at compile time and */
/*                variable via memory based interface                   */
/*                                                                      */
/*                             ----------                               */
/*                      -----\|          |                              */
/*        Memory Mapped ===== | INPUT(A) |----}   Netlist node          */
/*            Write     -----/|          |                              */
/*                             ----------                               */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_INPUT(name of node, initial value, addr, addrmask)      */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_INPUT(NODE_02, 100,0x0000,0x000f)                       */
/*                                                                      */
/*  Define a memory mapped input node called NODE_02 that has an        */
/*  initial value of 100. It is memory mapped into location 0x0000,     */
/*  0x0010, 0x0020 etc all the way to 0x0ff0. The exact size of memory  */
/*  space is defined by DSS_INPUT_SPACE in file disc_inp.c              */
/*                                                                      */
/*  The incoming address is first logicalled AND with the the ADDRMASK  */
/*  and then compared to the address, if there is a match on the write  */
/*  then the node is written to.                                        */
/*                                                                      */
/*  The memory space for discrete sound is 4096 locations (0x1000)      */
/*  the addr/addrmask values are used to setup a lookup table.          */
/*                                                                      */
/*  Can be written to with:    discrete_sound_w(0x0000, data);          */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_SQUAREWAVE - Squarewave waveform generator node, has four   */
/*                       input nodes FREQUENCY, AMPLITUDE, ENABLE and   */
/*                       DUTY if node is not connected it will default  */
/*                       to the initialised value.                      */
/*                                                                      */
/*                         ------------                                 */
/*                        |            |                                */
/*    ENABLE     -0------}|            |                                */
/*                        |            |                                */
/*    FREQUENCY  -1------}|            |                                */
/*                        |            |                                */
/*    AMPLITUDE  -2------}| SQUAREWAVE |----}   Netlist node            */
/*                        |            |                                */
/*    DUTY CYCLE -3------}|            |                                */
/*                        |            |                                */
/*    PHASE      -4------}|            |                                */
/*                        |            |                                */
/*                         ------------                                 */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_SQUAREWAVE(name of node,                                */
/*                         enable node or static value                  */
/*                         frequency node or static value               */
/*                         amplitude node or static value               */
/*                         duty cycle node or static value              */
/*                         starting phase value in degrees)             */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_SQUAREWAVE(NODE_03,NODE_01,NODE_02,0,100,90)            */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_SINEWAVE   - Sinewave waveform generator node, has four     */
/*                       input nodes FREQUENCY, AMPLITUDE, ENABLE and   */
/*                       PHASE, if a node is not connected it will      */
/*                       default to the initialised value in the macro  */
/*                                                                      */
/*                         ------------                                 */
/*                        |            |                                */
/*    ENABLE     -0------}|            |                                */
/*                        |            |                                */
/*    FREQUENCY  -1------}|            |                                */
/*                        | SINEWAVE   |----}   Netlist node            */
/*    AMPLITUDE  -2------}|            |                                */
/*                        |            |                                */
/*    PHASE      -3------}|            |                                */
/*                        |            |                                */
/*                         ------------                                 */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_SINEWAVE  (name of node,                                */
/*                         enable node or static value                  */
/*                         frequency node or static value               */
/*                         amplitude node or static value               */
/*                         starting phase value in degrees)             */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_SINEWAVE(NODE_03,NODE_01,NODE_02,10000,90)              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_ADDER      - Node addition function, available in three     */
/*                       lovelly flavours, ADDER2,ADDER3,ADDER4         */
/*                       that perform a summation of incoming nodes     */
/*                                                                      */
/*                         ------------                                 */
/*                        |            |                                */
/*    INPUT0     -0------}|            |                                */
/*                        |            |                                */
/*    INPUT1     -1------}|     |      |                                */
/*                        |    -+-     |----}   Netlist node            */
/*    INPUT2     -2------}|     |      |                                */
/*                        |            |                                */
/*    INPUT3     -3------}|            |                                */
/*                        |            |                                */
/*                         ------------                                 */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_ADDERx    (name of node,                                */
/*        (x=2/3/4)        enable node or static value                  */
/*                         input0 node or static value                  */
/*                         input1 node or static value                  */
/*                         input2 node or static value                  */
/*                         input3 node or static value)                 */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_ADDER2(NODE_03,1,NODE_12,-2000)                         */
/*                                                                      */
/*  Always enabled, subtracts 2000 from the output of NODE_12           */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_RCFILTER - Simple single pole RC filter network             */
/*                                                                      */
/*                         ------------                                 */
/*                        |            |                                */
/*    ENAB       -0------}| RC FILTER  |                                */
/*                        |            |                                */
/*    INPUT1     -1------}| -ZZZZ----  |                                */
/*                        |   R   |    |----}   Netlist node            */
/*    RVAL       -2------}|      ---   |                                */
/*                        |      ---C  |                                */
/*    CVAL       -3------}|       |    |                                */
/*                        |            |                                */
/*                         ------------                                 */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_RCFILTER(name of node,                                  */
/*                       enable                                         */
/*                       input node (or value)                          */
/*                       resistor value in OHMS                         */
/*                       capacitor value in FARADS)                     */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_RCFILTER(NODE_11,1,NODE_10,100,1e-6)                    */
/*                                                                      */
/*  Defines an always enabled RC filter with a 100R & 1uF network       */
/*  the input is fed from NODE_10.                                      */
/*                                                                      */
/*  This can be also thought of as a low pass filter with a 3dB cutoff  */
/*  at:                                                                 */
/*                                  1                                   */
/*            Fcuttoff =      --------------                            */
/*                            2*Pi*RVAL*CVAL                            */
/*                                                                      */
/*  (3dB cutoff is where the output power has dropped by 3dB ie Half)   */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_NE555 - NE555 Chip simulation                               */
/*                                                                      */
/*                         ------------                                 */
/*                        |            |                                */
/*    RESET      -0------}|            |                                */
/*                        |            |                                */
/*    TRIGGER    -1------}|            |                                */
/*                        |            |                                */
/*    THRESHOLD  -2------}|   NE555    |----}   Netlist node            */
/*                        |            |                                */
/*    CONTROL V  -3------}|            |                                */
/*                        |            |                                */
/*    VCC        -4------}|            |                                */
/*                        |            |                                */
/*                         ------------                                 */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_NE555(reset node (or value) - <0.7 causes a reset       */
/*                    trigger node (or value)                           */
/*                    threshold node (or value)                         */
/*                    ctrl volt node (or value) - Use NODE_NC for N/C   */
/*                    vcc node (or value) - Needed for comparators)     */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_NE555(NODE32,1,NODE_31,NODE_31,NODE_NC,5.0)             */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_OUTPUT - Single output node to Mame mixer and output        */
/*                                                                      */
/*                             ----------                               */
/*                            |          |     -/|                      */
/*      Netlist node --------}| OUTPUT   |----|  | Sound Output         */
/*                            |          |     -\|                      */
/*                             ----------                               */
/*                                                                      */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_OUTPUT(name of output node)                             */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_OUTPUT(NODE_02)                                         */
/*                                                                      */
/*  Output stream will be generated from the NODE_02 output stream.     */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DISCRETE_OUTPUT_STEREO - Single output node to Mame mixer and output */
/*                                                                      */
/*                                ----------                            */
/*                               |          |                           */
/*    Left  Netlist node -------}|          |     -/|                   */
/*                               | OUTPUT   |----|  | Sound Output      */
/*    Right Netlist node -------}|          |     -\|     L/R           */
/*                               |          |                           */
/*                                ----------                            */
/*  Declaration syntax                                                  */
/*                                                                      */
/*     DISCRETE_OUTPUT_STEREO(left output node, right output node)      */
/*                                                                      */
/*  Example config line                                                 */
/*                                                                      */
/*     DISCRETE_OUTPUT_STEREO(NODE_02,NODE12)                           */
/*                                                                      */
/*  Output stream will be generated from the NODE_02 and NODE_12        */
/*  streams.                                                            */
/*                                                                      */
/************************************************************************/

struct discrete_sound_block
{
	int node;                                   /* Output node number */
	int type;                                   /* see defines below */
	int input_node0;                            /* input/control nodes */
	int input_node1;                            /* input/control nodes */
	int input_node2;                            /* input/control nodes */
	int input_node3;                            /* input/control nodes */
	int input_node4;                            /* input/control nodes */
	int input_node5;                            /* input/control nodes */
	double initial0;                            /* Initial values */
	double initial1;                            /* Initial values */
	double initial2;                            /* Initial values */
	double initial3;                            /* Initial values */
	double initial4;                            /* Initial values */
	double initial5;                            /* Initial values */
};

struct node_description
{
	int 	node;								/* The nodes index number in the node list      */
	int		module;								/* The nodes module number from the module list */
	double	output;								/* The nodes last output value                  */
	struct node_description *input_node0;		/* Either pointer to input node OR NULL in which case use the input value */
	struct node_description *input_node1;
	struct node_description *input_node2;
	struct node_description *input_node3;
	struct node_description *input_node4;
	struct node_description *input_node5;
	double	input0;
	double	input1;
	double	input2;
	double	input3;
	double	input4;
	double	input5;
	void	*context;
};

struct discrete_module
{
	int type;
	char *name;
	int (*init) (struct node_description *node);
	int (*kill) (struct node_description *node);
	int (*reset) (struct node_description *node);
	int (*step) (struct node_description *node);
};

enum { NODE_00=0x40000000
              , NODE_01, NODE_02, NODE_03, NODE_04, NODE_05, NODE_06, NODE_07, NODE_08, NODE_09,
       NODE_10, NODE_11, NODE_12, NODE_13, NODE_14, NODE_15, NODE_16, NODE_17, NODE_18, NODE_19,
       NODE_20, NODE_21, NODE_22, NODE_23, NODE_24, NODE_25, NODE_26, NODE_27, NODE_28, NODE_29,
       NODE_30, NODE_31, NODE_32, NODE_33, NODE_34, NODE_35, NODE_36, NODE_37, NODE_38, NODE_39,
       NODE_40, NODE_41, NODE_42, NODE_43, NODE_44, NODE_45, NODE_46, NODE_47, NODE_48, NODE_49,
       NODE_50, NODE_51, NODE_52, NODE_53, NODE_54, NODE_55, NODE_56, NODE_57, NODE_58, NODE_59,
       NODE_60, NODE_61, NODE_62, NODE_63, NODE_64, NODE_65, NODE_66, NODE_67, NODE_68, NODE_69,
       NODE_70, NODE_71, NODE_72, NODE_73, NODE_74, NODE_75, NODE_76, NODE_77, NODE_78, NODE_79,
       NODE_80, NODE_81, NODE_82, NODE_83, NODE_84, NODE_85, NODE_86, NODE_87, NODE_88, NODE_89,
       NODE_90, NODE_91, NODE_92, NODE_93, NODE_94, NODE_95, NODE_96, NODE_97, NODE_98, NODE_99 };

/* Some Pre-defined nodes for convenience */

#define NODE_NC  NODE_00
#define NODE_OP  (NODE_00+0x00ff)

#define NODE_START	NODE_00
#define NODE_END	NODE_OP

/************************************************************************/
/*                                                                      */
/*        Enumerated values for Node types in the simulation            */
/*                                                                      */
/*  DSS - Discrete Sound Source                                         */
/*  DST - Discrete Sound Transform                                      */
/*  DSD - Discrete Sound Device                                         */
/*  DSO - Discrete Sound Output                                         */
/*                                                                      */
/************************************************************************/
enum {
	/* Sources */
		DSS_NULL,                                /* Nothing, nill, zippo, only to be used as terminating node */
		DSS_CONSTANT,                            /* Constant node */
		DSS_INPUT,                               /* Memory mapped input node */
		DSS_NOISE,                               /* Noise generator */
		DSS_SQUAREWAVE,                          /* Square Wave generator */
		DSS_SINEWAVE,                            /* Sine Wave generator */
//		DSS_TRIANGLEWAVE,                        /* Triangle wave generator */
//		DSS_SAWTOOTHWAVE,                        /* Sawtooth wave generator
	/* Transforms */
		DST_RCFILTER,							 /* Simple RC Filter network */
//		DST_LPF,                                 /* Low Pass Filter */
//		DST_HPF,                                 /* High Pass Filter */
		DST_GAIN,                                /* Gain Block */
//		DST_MODULATOR,                           /* C = A*B  */
		DST_ADDER,                               /* C = A+B */
//		DST_DELAY,                               /* Phase shift/Delay line */
	/* Devices */
		DSD_NE555,                               /* NE555 Emulation */
	/* Custom */
//		DST_CUSTOM,                              /* whatever you want someday */
    /* Output Node */
		DSO_OUTPUT                              /* The final output node */
};


/************************************************************************/
/*                                                                      */
/*        Encapsulation macros for defining your simulation             */
/*                                                                      */
/************************************************************************/
#define DISCRETE_SOUND_START(STRUCTURENAME) struct discrete_sound_block STRUCTURENAME[] = {
#define DISCRETE_SOUND_END                                       { NODE_00, DSS_NULL       ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,0     ,0     ,0     ,0     ,0     ,0     }  };

#define DISCRETE_CONSTANT(NODE,CONST)                            { NODE   , DSS_CONSTANT   ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,CONST ,0     ,0     ,0     ,0     ,0     },
#define DISCRETE_INPUT(NODE,ADDR,MASK,INIT0)                     { NODE   , DSS_INPUT      ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,INIT0 ,ADDR  ,MASK  ,1     ,0     ,INIT0 },
#define DISCRETE_INPUTX(NODE,ADDR,MASK,GAIN,OFFSET,INIT0)        { NODE   , DSS_INPUT      ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,INIT0 ,ADDR  ,MASK  ,GAIN  ,OFFSET,INIT0 },
#define DISCRETE_SQUAREWAVE(NODE,ENAB,FREQ,AMPL,DUTY,PHASE)      { NODE   , DSS_SQUAREWAVE ,ENAB   ,FREQ   ,AMPL   ,DUTY   ,PHASE  ,NODE_NC,ENAB  ,FREQ  ,AMPL  ,DUTY  ,0     ,PHASE },
#define DISCRETE_SINEWAVE(NODE,ENAB,FREQ,AMPL,PHASE)             { NODE   , DSS_SINEWAVE   ,ENAB   ,FREQ   ,AMPL   ,NODE_NC,NODE_NC,NODE_NC,ENAB  ,FREQ  ,AMPL  ,0     ,0     ,PHASE },
#define DISCRETE_NOISE(NODE,ENAB,FREQ,AMPL)                      { NODE   , DSS_NOISE      ,ENAB   ,FREQ   ,AMPL   ,NODE_NC,NODE_NC,NODE_NC,ENAB  ,FREQ  ,AMPL  ,0     ,0     ,0     },

#define DISCRETE_RCFILTER(NODE,ENAB,INP0,RVAL,CVAL)              { NODE   , DST_RCFILTER   ,ENAB   ,INP0   ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,ENAB  ,INP0  ,RVAL  ,CVAL  ,0     ,0     },
#define DISCRETE_INVERT(NODE,INP0)                               { NODE   , DST_GAIN       ,NODE_NC,INP0   ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,1     ,0     ,-1    ,0     ,0     ,0     },
#define DISCRETE_GAIN(NODE,INP0,GAIN)                            { NODE   , DST_GAIN       ,NODE_NC,INP0   ,GAIN   ,NODE_NC,NODE_NC,NODE_NC,1     ,0     ,GAIN  ,0     ,0     ,0     },
#define DISCRETE_ONOFF(NODE,ENAB,INP0)                           { NODE   , DST_GAIN       ,ENAB   ,INP0   ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,ENAB  ,0     ,1     ,0     ,0     ,0     },
#define DISCRETE_ADDER2(NODE,ENAB,INP0,INP1)                     { NODE   , DST_ADDER      ,ENAB   ,INP0   ,INP1   ,NODE_NC,NODE_NC,NODE_NC,ENAB  ,INP0  ,INP1  ,0     ,0     ,0     },
#define DISCRETE_ADDER3(NODE,ENAB,INP0,INP1,INP2)                { NODE   , DST_ADDER      ,ENAB   ,INP0   ,INP1   ,INP2   ,NODE_NC,NODE_NC,ENAB  ,INP0  ,INP1  ,INP2  ,0     ,0     },
#define DISCRETE_ADDER4(NODE,ENAB,INP0,INP1,INP2,INP3)           { NODE   , DST_ADDER      ,ENAB   ,INP0   ,INP1   ,INP2   ,INP3   ,NODE_NC,ENAB  ,INP0  ,INP1  ,INP2  ,INP3  ,0     },

#define DISCRETE_NE555(NODE,RESET,TRIGR,THRSH,CTRLV,VCC)         { NODE   , DSD_NE555      ,RESET  ,TRIGR  ,THRSH  ,CTRLV  ,VCC    ,NODE_NC,RESET ,TRIGR ,THRSH ,CTRLV ,VCC   ,0     },

#define DISCRETE_OUTPUT(OPNODE)                                  { NODE_OP, DSO_OUTPUT     ,OPNODE ,OPNODE ,NODE_NC,NODE_NC,NODE_NC,NODE_NC,0     ,0     ,0     ,0     ,0     ,0     },
#define DISCRETE_OUTPUT_STEREO(OPNODEL,OPNODER)                  { NODE_OP, DSO_OUTPUT     ,OPNODEL,OPNODER,NODE_NC,NODE_NC,NODE_NC,NODE_NC,0     ,0     ,0     ,0     ,0     ,0     },


/************************************************************************/
/*                                                                      */
/*        Software interface to the external world i.e Into Mame        */
/*                                                                      */
/************************************************************************/

int  discrete_sh_start (const struct MachineSound *msound);
void discrete_sh_stop (void);
void discrete_sh_reset (void);
void discrete_sh_update (void);

WRITE_HANDLER(discrete_sound_w);
READ_HANDLER(discrete_sound_r);


#endif
