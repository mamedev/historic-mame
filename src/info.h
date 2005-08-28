/***************************************************************************

    info.h

    Dumps the MAME internal data as an XML file.

***************************************************************************/

#pragma once

#ifndef __INFO_H__
#define __INFO_H__

/* Print the MAME database in XML format */
void print_mame_xml(FILE* out, const game_driver* games[]);

#endif	/* __INFO_H__ */
