/***************************************************************************

    CDRDAO TOC parser for CHD compression frontend

***************************************************************************/

#pragma once

#ifndef __CHDCD_H__
#define __CHDCD_H__

int cdrom_parse_toc(char *tocfname, struct cdrom_toc *outtoc, struct cdrom_track_input_info *outinfo);

#endif	/* __CHDCD_H__ */
