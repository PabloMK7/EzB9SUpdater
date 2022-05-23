#include "main.h"

extern u8       *tmpBuffer;
extern char     *g_primary_error;
extern char     *g_secondary_error;
extern bool     g_exit;

void    flushDataCache(void)
{
    u32 *ptr = (u32 *)tmpBuffer;
    u32 i;

    for (i = 0; i < (0x20000 / 4); i++)
    {
        ptr[i] += 0x11111111;
    }
}

void doStallCpu(void)
{
    vu32 i;
    for (i = 0; i < 0x00100000; i++)
    {
    }
}

void    doWait(void)
{
    svcSleepThread(500000000);
}

void    strJoin(char *dst, const char *s1, const char *s2)
{
    if (!dst || !s1 || !s2) return;

    while (*s1)
        *dst++ = *s1++;
    while (*s2)
        *dst++ = *s2++;
    *dst = '\0';
}

void    strInsert(char *dst, char *src, int index)
{
    char    *bak;
    int     srcSize;

    if (!dst || !src) return;

    bak = strdup(dst + index);
    strcpy(dst + index, src);
    srcSize = strlen(src);
    strcpy(dst + srcSize, bak);
    free(bak);
}

void    strncpyFromTail(char *dst, char *src, int nb)
{
    if (!src || !dst || nb == 0) return;
    while (*src) src++;
    src--;
    nb--;
    dst += nb;
    while (nb-- >= 0)
        *dst-- = *src--;
}

bool    inputPathKeyboard(char *dst, char *hintText, char *initialText, int bufSize)
{
    static SwkbdState           keyboard;
    u32                         features = 0;
    SwkbdButton                 button = SWKBD_BUTTON_NONE;

    swkbdInit(&keyboard, SWKBD_TYPE_QWERTY, 2, -1);
    swkbdSetFeatures(&keyboard, features);
    swkbdSetHintText(&keyboard, hintText);
    if (initialText)
        swkbdSetInitialText(&keyboard, initialText);
    swkbdSetButton(&keyboard, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&keyboard, SWKBD_BUTTON_RIGHT, "Ok", true);
    button = swkbdInputText(&keyboard, dst, bufSize);
    if (button == SWKBD_BUTTON_LEFT)
        return (false);
    else
        return (true);
}

void customBreak(u32 r0, u32 r1, u32 r2, u32 r3) {
	__asm__ __volatile__("svc 0x3C");
	for (;;);
}

void    waitAllKeysReleased(void)
{
    while (1)
    {
        hidScanInput();
        if ((hidKeysDown() | hidKeysHeld() | hidKeysUp()) == 0)
            return;
    }
}

void    debug(char *str, int seconds)
{
    time_t t;

    newAppTop(DEFAULT_COLOR, 0, "%s", str);
    t = time(NULL);
    while (t + seconds != time(NULL))
        updateUI();
}

void    wait(int seconds)
{
    time_t  t;

    t = time(NULL);
    while (t + seconds != time(NULL));
}
