#define FD1094_STATE_RESET	0x0100
#define FD1094_STATE_IRQ	0x0200
#define FD1094_STATE_RTE	0x0300
#define FD1094_STATE_VECTOR	0x12340000

void fd1094_init(int cpu_number);
int fd1094_set_state(int state);
int fd1094_decode(int address,int val);
