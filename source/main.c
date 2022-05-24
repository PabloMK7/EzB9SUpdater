#include "main.h"
#include "draw.h"
#include <time.h>
#include "Unicode.h"

bool              g_exit = false;
bool			  restartneeded = false;
extern sprite_t* topInfoSprite;

aptHookCookie aptCookie;

bool checkRunningUnsupported() {
	s64 out = 0;
	svcGetSystemInfo(&out, 0x10000, 0);
	return GET_VERSION_MAJOR(out) <= 8;
}

bool isReboot() {
	return fileExists("/ezb9stemp/reboot.flag");
}

int main()
{
	gfxInitDefault();
	romfsInit();
	ptmSysmInit();
	drawInit();

	initUI();
	bool continueInstall = true;
	bool rebootintoitself = false;
	bool unsupported = checkRunningUnsupported();
	clearTop();
	if (isReboot()) {
		clearTop();
		newAppTop(DEFAULT_COLOR, MEDIUM | BOLD | CENTER, "EzB9SUpdater");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "You should have returned from");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "SafeB9SInstaller. If that wasn't");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "the case, you probably released");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "the START button too early.");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "If you installed B9S properly");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "you can exit this app.");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Having issues? Ask here:");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "https://discord.gg/C29hYvh");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, FONT_B": Exit");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "START: Launch SafeB9SInstaller");
		u32 keys = 0;
		bool warningloop = true;
		while (warningloop && aptMainLoop())
		{
			if (keys & KEY_B) {
				warningloop = false;
				ezB9SCleanup();
			}
			if (keys & KEY_START) {
				warningloop = false;
				rebootintoitself = true;
			}
			updateUI();
			keys = hidKeysDown();
		}			
	} else {
		if (unsupported) {
			clearTop();
			newAppTop(COLOR_RED, MEDIUM | BOLD | CENTER, "Fatal Error");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Unsupported environment.");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Very outdated custom firmware");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "or Citra detected. Please ask");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "for help in the Nintendo");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Homebrew discord server:");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "https://discord.gg/C29hYvh");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "\n"FONT_B": Exit");
			u32 keys = 0;
			bool warningloop = true;
			while (warningloop && aptMainLoop())
			{
				if (keys & KEY_B) {
					warningloop = false;
					continueInstall = false;
				}
				updateUI();
				keys = hidKeysDown();
			}
		}
		if (continueInstall && !aptShouldClose()) {
			clearTop();
			newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Use this tool to update");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "B9S to the latest version.");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Running this tool with the");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "latest version already installed");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "has have no effect, so feel");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "free to try it anyways.\n");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, FONT_A": Proceed");
			newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, FONT_B": Exit");
			u32 keys = 0;
			bool installLoop = true;
			while (installLoop && aptMainLoop()) {
				if ((keys & KEY_A)) {
					CURL_lastErrorCode[0] = '\0';
					u64 ret = ezB9SPerform();
					u32 retlow = (u32)ret;
					u32 rethigh = (u32)(ret >> 32);
					clearTop();
					if (!rethigh) {
						
						newAppTop(COLOR_GREEN, BOLD | MEDIUM | CENTER, "Download successful!");
						newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "You will now reboot into");
						newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "SafeB9SInstaller to complete");
						newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "the update procedure.");
						updateUI();
						svcSleepThread(10000000000);
						newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "\n\nPress and hold START until");
						newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "you see the SafeB9SInstaller");
						newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "menu and follow the instructions.");
					}
					else
					{	
						if (retlow == 2) {
							newAppTop(COLOR_RED, BOLD | MEDIUM | CENTER, "Download failed!");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Unable to connect to the internet,");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "please try again. If this problem");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "keeps happening look for an updated");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "version of this app.");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "\nYou can ask for help here");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "https://discord.gg/C29hYvh\n");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, FONT_B": Exit");
						} else if (retlow == 5) {
							newAppTop(COLOR_RED, BOLD | MEDIUM | CENTER, "Download failed!");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "The author of this app has marked");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "it as \"obsolete\". Please look");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "for an updated version of this app.");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "\nYou can ask for help here");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "https://discord.gg/C29hYvh\n");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, FONT_B": Exit");
						} else {
							newAppTop(COLOR_RED, BOLD | MEDIUM | CENTER, "Download failed!");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "An error has occurred:");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "%08X %08X", rethigh, retlow);
							if (CURL_lastErrorCode[0])
								newAppTopMultiline(DEFAULT_COLOR, SMALL | CENTER, CURL_lastErrorCode);
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "\nYou can ask for help here");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "https://discord.gg/C29hYvh\n");
							newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, FONT_B": Exit");
						}
					}
					u32 keys2 = 0;
					bool installLoop2 = true;
					while (installLoop2 && aptMainLoop())
					{
						if ((retlow && (keys2 & KEY_B)) || (!retlow && (keys2 & KEY_START))) {
							installLoop2 = false;
							if (!retlow && (keys2 & KEY_START))
								rebootintoitself = true;
						}
						updateUI();
						keys2 = hidKeysDown();
					}
					installLoop = false;
				}
				if (keys & KEY_B) {
					installLoop = false;
				}
				updateUI();
				keys = hidKeysDown();
			}
		}
	}
	
	greyExit();
	updateUI();
	svcSleepThread(500000000);

	exitUI();
	drawExit();
	httpcExit();
	romfsExit();

	gfxExit();
	ptmSysmExit();
	if (rebootintoitself) {
		nsInit();
		NS_RebootToTitle(MEDIATYPE_SD, 0x000400000ECB9500);
		for(;;);
	}
	return (0);
}