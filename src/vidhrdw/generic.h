#include "driver.h"


extern unsigned char *videoram;
extern int videoram_size;
extern unsigned char *colorram;
extern unsigned char *spriteram;
extern int spriteram_size;
extern unsigned char *spriteram_2;
extern int spriteram_2_size;
extern unsigned char *spriteram_3;
extern int spriteram_3_size;
extern unsigned char *flip_screen;
extern unsigned char *flip_screen_x;
extern unsigned char *flip_screen_y;
extern unsigned char *dirtybuffer;
extern struct osd_bitmap *tmpbitmap;

int generic_vh_start(void);
void generic_vh_stop(void);
int videoram_r(int offset);
int colorram_r(int offset);
void videoram_w(int offset,int data);
void colorram_w(int offset,int data);
int spriteram_r(int offset);
void spriteram_w(int offset,int data);


extern unsigned char *videoram00,*videoram01,*videoram02,*videoram03;
extern unsigned char *videoram10,*videoram11,*videoram12,*videoram13;
extern unsigned char *videoram20,*videoram21,*videoram22,*videoram23;
extern unsigned char *videoram30,*videoram31,*videoram32,*videoram33;

void videoram00_w(int offset,int data);
void videoram01_w(int offset,int data);
void videoram02_w(int offset,int data);
void videoram03_w(int offset,int data);
void videoram10_w(int offset,int data);
void videoram11_w(int offset,int data);
void videoram12_w(int offset,int data);
void videoram13_w(int offset,int data);
void videoram20_w(int offset,int data);
void videoram21_w(int offset,int data);
void videoram22_w(int offset,int data);
void videoram23_w(int offset,int data);
void videoram30_w(int offset,int data);
void videoram31_w(int offset,int data);
void videoram32_w(int offset,int data);
void videoram33_w(int offset,int data);

int videoram00_r(int offset);
int videoram01_r(int offset);
int videoram02_r(int offset);
int videoram03_r(int offset);
int videoram10_r(int offset);
int videoram11_r(int offset);
int videoram12_r(int offset);
int videoram13_r(int offset);
int videoram20_r(int offset);
int videoram21_r(int offset);
int videoram22_r(int offset);
int videoram23_r(int offset);
int videoram30_r(int offset);
int videoram31_r(int offset);
int videoram32_r(int offset);
int videoram33_r(int offset);

void videoram00_word_w(int offset,int data);
void videoram01_word_w(int offset,int data);
void videoram02_word_w(int offset,int data);
void videoram03_word_w(int offset,int data);
void videoram10_word_w(int offset,int data);
void videoram11_word_w(int offset,int data);
void videoram12_word_w(int offset,int data);
void videoram13_word_w(int offset,int data);
void videoram20_word_w(int offset,int data);
void videoram21_word_w(int offset,int data);
void videoram22_word_w(int offset,int data);
void videoram23_word_w(int offset,int data);
void videoram30_word_w(int offset,int data);
void videoram31_word_w(int offset,int data);
void videoram32_word_w(int offset,int data);
void videoram33_word_w(int offset,int data);

int videoram00_word_r(int offset);
int videoram01_word_r(int offset);
int videoram02_word_r(int offset);
int videoram03_word_r(int offset);
int videoram10_word_r(int offset);
int videoram11_word_r(int offset);
int videoram12_word_r(int offset);
int videoram13_word_r(int offset);
int videoram20_word_r(int offset);
int videoram21_word_r(int offset);
int videoram22_word_r(int offset);
int videoram23_word_r(int offset);
int videoram30_word_r(int offset);
int videoram31_word_r(int offset);
int videoram32_word_r(int offset);
int videoram33_word_r(int offset);
