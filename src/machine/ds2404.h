#ifndef DS2404_H
#define DS2404_H

void DS2404_init(void);
void DS2404_load(mame_file *file);
void DS2404_save(mame_file *file);

/* 1-wire interface reset */
WRITE_HANDLER( DS2404_1W_reset_w );

/* 3-wire interface reset  */
WRITE_HANDLER( DS2404_3W_reset_w );

READ_HANDLER( DS2404_data_r );
WRITE_HANDLER( DS2404_data_w );
WRITE_HANDLER( DS2404_clk_w );

#endif
