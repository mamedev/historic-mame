extern int pending_commands;

void sound_command_w(int offset,int data);
int sound_command_r(int offset);
int sound_command_latch_r(int offset);
int sound_pending_commands_r(int offset);
