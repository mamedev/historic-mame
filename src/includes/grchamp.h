/*************************************************************************

    Taito Grand Champ hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define GRCHAMP_ENGINE_CS_EN				NODE_01
#define GRCHAMP_SIFT_DATA					NODE_02
#define GRCHAMP_ATTACK_UP_DATA				NODE_03
#define GRCHAMP_IDLING_EN					NODE_04
#define GRCHAMP_FOG_EN						NODE_05
#define GRCHAMP_PLAYER_SPEED_DATA			NODE_06
#define GRCHAMP_ATTACK_SPEED_DATA			NODE_07
#define GRCHAMP_A_DATA						NODE_08
#define GRCHAMP_B_DATA						NODE_09

/*----------- defined in sndhrdw/grchamp.c -----------*/

extern struct discrete_sound_block grchamp_discrete_interface[];
