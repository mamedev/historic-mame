
int  starwars_sh_start(void);
void starwars_sh_stop(void);
void starwars_sh_update(void);

void starwars_pokey_sound_w(int offset,int data);
void starwars_pokey_ctl_w(int offset,int data);

int sin_r(int);
int PIA_port_r(int);
int PIA_timer_r(int);

void sout_w(int, int);
void PIA_port_w(int, int);
void PIA_timer_w(int, int);

int starwars_snd_interrupt(void);

