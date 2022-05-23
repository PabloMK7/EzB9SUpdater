#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "draw.h"
#include "main.h"
#include "appInfo.h"

#define STACKSIZE 0x1000

void    initUI(void);
void    exitUI(void);
int     updateUI(void);
void    greyExit();
void    setExTopObject(void *object);
void    addTopObject(void *object);
void    addBottomObject(void *object);
void    changeTopFooter(sprite_t *footer);
void    changeTopHeader(sprite_t *header);
void    changeBottomFooter(sprite_t *footer);
void    changeBottomHeader(sprite_t *header);
void    clearTopScreen(void);
void    clearBottomScreen(void);

extern appInfoObject_t  *appTop;
extern appInfoObject_t  *appBottom;

#define newAppTop(...) newAppInfoEntry(appTop, __VA_ARGS__)
#define newAppTopMultiline(color, flags, text) drawMultilineText(color, flags, text)
#define removeAppTop() removeAppInfoEntry(appTop)
#define clearTop()    clearAppInfo(appTop)

#define TRACE() {newAppTop(DEFAULT_COLOR, SMALL, "%s:%d",__FUNCTION__, __LINE__); svcSleepThread(1000000000); updateUI(); svcSleepThread(1000000000);}
#define XTRACE(str, ...) {newAppTop(DEFAULT_COLOR, SMALL, str, __VA_ARGS__); updateUI(); svcSleepThread(500000000);}  

#endif