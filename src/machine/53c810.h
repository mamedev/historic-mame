#ifndef LSI53C810_H
#define LSI53C810_H

UINT8 lsi53c810_reg_r(int reg);
void lsi53c810_reg_w(int reg, UINT8 value);
void lsi53c810_init(UINT32 (* fetch)(UINT32 dsp), void (* irq_callback)(void), void (* dma_callback)(UINT32, UINT32, int, int));

#endif
