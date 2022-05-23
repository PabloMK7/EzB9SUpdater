#include "main.h"

u32 APT_Reboot(u64 titleID, FS_MediaType mediatype, u32 firmtidlow) {
	u32 cmdbuf[16];
	cmdbuf[0] = 0x00490180;
	*((u64*)(cmdbuf + 1)) = titleID;
	cmdbuf[3] = mediatype & 0xFF;
	cmdbuf[5] = 1;
	cmdbuf[6] = firmtidlow;

	return aptSendCommand(cmdbuf);
}