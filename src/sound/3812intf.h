#ifndef YM3812INTF_H
#define YM3812INTF_H


#define MAX_3812 1
#define MAX_8950 MAX_3812

#if 0
struct YM3812interface
{
	int num;
	int baseclock;
	int mixing_level[MAX_3812];
	void (*handler[MAX_3812])(void);
};
#endif

struct YM3812interface
{
	int num;
	int baseclock;
	int mixing_level[MAX_3812];
	void (*handler)(void);
};

struct Y8950interface
{
	int num;
	int baseclock;
	int mixing_level[MAX_8950];
	void (*handler[MAX_8950])(void);
	int (*keyboardread[MAX_8950])(int offset);
	void (*keyboardwrite[MAX_8950])(int offset,int data);
	int (*portread[MAX_8950])(int offset);
	void (*portwrite[MAX_8950])(int offset,int data);
};

#define YM3526interface YM3812interface

int YM3812_status_port_0_r(int offset);
void YM3812_control_port_0_w(int offset,int data);
void YM3812_write_port_0_w(int offset,int data);

int YM3812_sh_start(const struct MachineSound *msound);
void YM3812_sh_stop(void);
void YM3812_sh_update(void);

#define YM3526_status_port_0_r YM3812_status_port_0_r
#define YM3526_control_port_0_w YM3812_control_port_0_w
#define YM3526_write_port_0_w YM3812_write_port_0_w
int YM3526_sh_start(const struct MachineSound *msound);
#define YM3526_sh_stop YM3812_sh_stop
#define YM3526_shupdate YM3812_sh_update

#define Y8950_status_port_0_r YM3812_status_port_0_r
#define Y8950_control_port_0_w YM3812_control_port_0_w
#define Y8950_write_port_0_w YM3812_write_port_0_w
int Y8950_sh_start(const struct MachineSound *msound);
#define Y8950_sh_stop YM3812_sh_stop
#define Y8950_shupdate YM3812_sh_update

#endif
