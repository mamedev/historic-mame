extern int pending_commands;

int sound_command_r(int offset);
int sound_command_latch_r(int offset);
void sound_command_w(int offset,int data);
