#ifndef MODE_H
#define MODE_H

#include "freetype_renderer.h"
#include "text_button.h"


enum ButtonAction
{
	BUTTON_ACTION_UNHANDLED=0,
	BUTTON_ACTION_HANDLED,
	BUTTON_ACTION_EXIT_MODE
};

class Mode
{
public:
	FreetypeRenderer& freetype_renderer;
	TextButtonList text_buttons;
public:
	Mode( FreetypeRenderer& _freetype_renderer ) : freetype_renderer(_freetype_renderer) {}
	virtual void init_mode();
	virtual void init_vram();
	virtual void free_vram();
	virtual void render( Screen screen );
	virtual void run_until_exit();
	virtual ButtonAction handle_button_pressed( TextButton* text_button ) { return BUTTON_ACTION_UNHANDLED; }
};

#endif // MODE_H