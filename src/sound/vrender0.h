#define MAX_VR0	(1)

struct VR0Interface
{
	UINT32 RegBase;
	int mixing_level;
};

int VR0_sh_start(const struct MachineSound *msound);
void VR0_sh_stop(void);
void VR0_Snd_Set_Areas(UINT32 *Texture,UINT32 *Frame);

READ32_HANDLER(VR0_Snd_Read);
WRITE32_HANDLER(VR0_Snd_Write);
