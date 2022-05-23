#include "drawableObject.h"
#include "draw.h"

backgroundScreen_t  *newBackgroundObject(sprite_t *background, \
    sprite_t *header, sprite_t *footer)
{
    backgroundScreen_t  *ret;

    ret = (backgroundScreen_t *)calloc(1, sizeof(backgroundScreen_t));
    if (!ret) goto error;
    
    if (background) ret->background = background;
    if (header) ret->headerText = header;
    if (footer) ret->footerText = footer;
    ret->draw = drawBackground;
    return (ret);
error:
    return (NULL);
}

static bool drawableSpriteDraw(void* self) {
    if (((drawableSprite_t*)self)->sprite) {
        drawSprite(((drawableSprite_t*)self)->sprite);
        return true;
    }
    return false;
}

void initDrawableSprite(drawableSprite_t* object, sprite_t* sprite) {
    object->sprite = sprite;
    object->draw = drawableSpriteDraw;
}

bool        drawBackground(void *self)
{
    backgroundScreen_t *bg;

    if (!self) goto error;
    bg = (backgroundScreen_t *)self;
    drawSprite(bg->background);
    drawSprite(bg->headerText);
    drawSprite(bg->footerText);
error:
    return (false);
}

void        changeBackgroundHeader(backgroundScreen_t *bg, sprite_t *header)

{
    if (!bg) goto error;
    bg->headerText = header;
error:
    return;
}

void        changeBackgroundFooter(backgroundScreen_t *bg, sprite_t *footer)
{
    if (!bg) goto error;
    bg->footerText = footer;
error:
    return;
}
drawableScreen_t    *newDrawableScreen(backgroundScreen_t *background)
{
    drawableScreen_t    *ret;

    ret = (drawableScreen_t *)calloc(1, sizeof(drawableScreen_t));
    if (!ret) goto error;
    if (background) ret->background = background;
    ret->draw = drawScreen;
    return (ret);
error:
    return (NULL);
}

bool        drawScreen(void *self)
{
    drawableScreen_t    *screen;
    drawableObject_t    *obj;
    int                 i;

    if (!self) goto error;
	screen = (drawableScreen_t *)self;
    if (screen->background)
        screen->background->draw(screen->background);
    for (i = 0; i < MAX_ELEMENTS; i++)
    {
        obj = (drawableObject_t *)screen->elementList[i];
        if (!obj) continue;
        obj->draw(obj);
    }
    return (true);
error:
    return (false);
}

void        setExTopObjectToScreen(drawableScreen_t *screen, void *object)
{
	screen->elementList[MAX_ELEMENTS-1] = (int)object;
	return;
}

void        addObjectToScreen(drawableScreen_t *screen, void *object)
{
    if (!screen || !object || screen->elementsCount > MAX_ELEMENTS) goto error;
    screen->elementList[screen->elementsCount] = (int)object;
    screen->elementsCount++;
error:
    return;
}

void        *removeLastObjectFromScreen(drawableScreen_t *screen)
{
    void    *ret;
    if (!screen || !screen->elementsCount) goto error;
    ret = (void *)screen->elementList[screen->elementsCount - 1];
    screen->elementList[screen->elementsCount - 1] = 0;
    screen->elementsCount--;
    return (ret);
error:
    return (NULL);
}

void        clearObjectListFromScreen(drawableScreen_t *screen)
{
    void    *ret;

    if (!screen) goto error;
    while (1)
    {
        ret = removeLastObjectFromScreen(screen);
        if (!ret)
            break;
    }
error:
    return;
}

bool        drawImage(void *self)
{
    image_t *img;

    if (!self) goto error;
    img = (image_t *)self;
    drawSprite(img->sprite);
    return (true);
error:
    return (false);
}

window_t    *newWindow(sprite_t *background, sprite_t *title, \
    sprite_t *content)
{
    window_t    *window;
    
    window = (window_t *)calloc(1, sizeof(window_t));
    if (!window) goto error;
    if (background) window->background = background;
    if (title) window->title = title;
    if (content) window->content = content;
    window->draw = drawWindow;
    return (window);
error:
    return (NULL);
}

bool       drawWindow(void *self)
{
    window_t *window;

    if (!self) goto error;
    window = (window_t *)self;
    if (window->background) drawSprite(window->background);
    if (window->title) drawSprite(window->title);
    if (window->content) drawSprite(window->content);
    return (true);
error:
    return (false);
}

void    changeWindowContent(window_t *window, sprite_t *content)
{
    if (!window) goto error;
    window->content = content;
error:
    return;
}

void    changeWindowTitle(window_t *window, sprite_t *title)
{
    if (!window) goto error;
    window->title = title;
error:
    return;
}

text_t  *newText(char *str, float posX, float posY, float scaleX, float scaleY, int color)
{
    text_t *ret;

    ret = (text_t *)calloc(1, sizeof(text_t));
    if (!ret) goto error;
    ret->str = str;
    ret->posX = posX;
    ret->posY = posY;
    ret->scaleX = scaleX;
    ret->scaleY = scaleY;
    ret->color = color;
    ret->visible = false;
    ret->draw = drawTextObject;
    return (ret);   
error:
    return (NULL);
}

bool    drawTextObject(void *self)
{
    text_t *t;

    if (!self) goto error;
    t = (text_t *)self;
    if (!t->str || !t->visible) goto error;
    setTextColor(t->color);
    renderText(t->posX, t->posY, t->scaleX, t->scaleY, false, t->str, NULL, 0.0f);
    return (true);
error:
    return (false);
}

void    hideText(text_t *text)
{
    if (text)
        text->visible = false;
}

void    showText(text_t *text)
{
    if (text)
        text->visible = true;
}

bool drawRectangleObject(void* self) {
	if (!self) return false;
	drawRectangle((rectangle_t*)self);
	return true;
}

void deleteRectangle(rectangle_t* rec) {
	if (!rec) return;
	deleteSprite(rec->sprite);
	free(rec);
	rec = NULL;
}

rectangle_t *newRectangle(float posX, float posY, float width, float height, float initialamount, sprite_t* sprite) {
	rectangle_t* ret;
	ret = (rectangle_t *)calloc(1, sizeof(rectangle_t));
	if (!ret) goto error;
	if (initialamount < 0) initialamount = 0;
	if (initialamount > 1) initialamount = 1;
	ret->amount = initialamount;
	ret->sprite = sprite;
	ret->posX = posX;
	ret->posY = posY;
	ret->height = height;
	ret->width = width;
	ret->draw = drawRectangleObject;
	return ret;
error:
	return NULL;
}
