/***************************************************************************

	config.h

	Wrappers for handling MAME configuration files

***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "mame.h"
#include "input.h"


/*************************************
 *
 *	Function prototypes
 *
 *************************************/

int config_load(const struct InputPort *input_ports_default, struct InputPort *input_ports);
void config_save(const struct InputPort *input_ports_default, const struct InputPort *input_ports);
int config_load_default(const struct InputPortDefinition *input_ports_backup, struct InputPortDefinition *input_ports);
void config_save_default(const struct InputPortDefinition *input_ports_backup, const struct InputPortDefinition *input_ports);
int config_load_controller(const char *name, struct InputPortDefinition *input_ports);


#endif /* CONFIG_H */
