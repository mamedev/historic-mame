#ifndef pokyintf_h
#define pokyintf_h

int pokey1_sh_start (void);
int pokey2_sh_start (void);
int pokey4_sh_start (void);

void pokey_sh_stop (void);

int pokey1_r (int offset);
int pokey2_r (int offset);
int pokey3_r (int offset);
int pokey4_r (int offset);

void pokey1_w (int offset,int data);
void pokey2_w (int offset,int data);
void pokey3_w (int offset,int data);
void pokey4_w (int offset,int data);

void pokey_sh_update (void);

#endif