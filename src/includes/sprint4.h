/***************************************************************************

Atari Sprint 4 + Ultra Tank driver

***************************************************************************/

#include "sound/discrete.h"

/* discrete sound input nodes */
#define SPRINT4_SKID1_EN        NODE_01
#define SPRINT4_SKID2_EN        NODE_02
#define SPRINT4_SKID3_EN        NODE_03
#define SPRINT4_SKID4_EN        NODE_04
#define SPRINT4_MOTOR1_DATA     NODE_05
#define SPRINT4_MOTOR2_DATA     NODE_06
#define SPRINT4_MOTOR3_DATA     NODE_07
#define SPRINT4_MOTOR4_DATA     NODE_08
#define SPRINT4_CRASH_DATA      NODE_09
#define SPRINT4_ATTRACT_EN      NODE_10


extern struct discrete_sound_block sprint4_discrete_interface[];
