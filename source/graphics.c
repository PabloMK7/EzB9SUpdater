#include "graphics.h"
#include "drawableObject.h"

sprite_t         *bottomSprite = NULL;
sprite_t         *topSprite = NULL;

sprite_t         *topInfoSprite = NULL;

drawableScreen_t *botScreen = NULL;
drawableScreen_t *topScreen = NULL;
appInfoObject_t  *appTop = NULL;

char textVersion[20] = { 0 };

void    initUI(void)
{
    backgroundScreen_t *bg;

    newSpriteFromPNG(&topSprite, "romfs:/sprites/topBackground.png");
    newSpriteFromPNG(&bottomSprite, "romfs:/sprites/bottomBackground.png");    

    newSpriteFromPNG(&topInfoSprite, "romfs:/sprites/topInfoBackground.png");

    setSpritePos(topSprite, 0, 0);
    setSpritePos(bottomSprite, 0, 0);
    
    bg = newBackgroundObject(bottomSprite, NULL, NULL);
    botScreen = newDrawableScreen(bg);
    bg = newBackgroundObject(topSprite, NULL, NULL);
    topScreen = newDrawableScreen(bg);

    setSpritePos(topInfoSprite, 50, 20);
    appTop = newAppInfoObject(topInfoSprite, 14, 58.0f, 30.0f);
    appInfoSetTextBoundaries(appTop, 343.0f, 220.0f);
}

void    exitUI(void)
{
    deleteAppInfoObject(appTop);
    deleteSprite(bottomSprite);
    deleteSprite(topSprite);
    deleteSprite(topInfoSprite);
}

void greyExit() {
	botScreen->background->background->isGreyedOut = true;
	topScreen->background->background->isGreyedOut = true;
	if (topInfoSprite) topInfoSprite->isGreyedOut = true;
}

static inline void drawUITop(void)
{
    setScreen(GFX_TOP);
    
    topScreen->draw(topScreen);

    setTextColor(COLOR_BLANK);
    if (textVersion[0] == '\0') {
        sprintf(textVersion, "Ver. %d.%d.%d", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_MICRO);
    }
    renderText(1.0f, 1.0f, 0.4f, 0.45f, false, textVersion, NULL, 0.0f);
    
    drawAppInfo(appTop);
}

static inline void drawUIBottom(void)
{
    setScreen(GFX_BOTTOM);
    
    botScreen->draw(botScreen);
}

int   updateUI(void)
{
    hidScanInput();
    if (aptMainLoop()) {
        drawUITop();
        drawUIBottom();
        updateScreen();
    }
    return (1);
}

void    addTopObject(void *object)
{
    addObjectToScreen(topScreen, object);
}

void    setExTopObject(void *object)
{
	setExTopObjectToScreen(topScreen, object);
}

void    addBottomObject(void *object)
{
    addObjectToScreen(botScreen, object);
}

void    changeTopFooter(sprite_t *footer)
{
    changeBackgroundFooter(topScreen->background, footer);
}

void    changeTopHeader(sprite_t *header)
{
    changeBackgroundHeader(topScreen->background, header);
}

void    changeBottomFooter(sprite_t *footer)
{
    changeBackgroundFooter(botScreen->background, footer);
}

void    changeBottomHeader(sprite_t *header)
{
    changeBackgroundHeader(botScreen->background, header);
}

void    clearTopScreen(void)
{
    clearObjectListFromScreen(topScreen);
}

void    clearBottomScreen(void)
{
    clearObjectListFromScreen(botScreen);
}