#ifndef TOUCH_KEYBOARD_H
#define TOUCH_KEYBOARD_H

#include "mode.h"
#include "ui_language.h"
#include "freetype_renderer.h"
#include "text_button.h"

typedef std::map<std::string,std::string> StringMap;

class TouchKeyboard : public Mode
{
protected:
	UILanguage& ui_lang;
	RenderScreen keyboard_screen;
	TextButton reference_key, exit_button;
	TextButtonSetStorage keys;
	std::string written_text, modifier;
	StringMap modifier_map;
public:
	TouchKeyboard( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer );
	virtual void init_mode();
	virtual void init_vram();
	virtual void render( Screen screen );
	virtual ButtonAction handle_button_pressed( TextButton* text_button );
	virtual std::string handle_key_pressed( const std::string& input ) { return input; }
	virtual void handle_text_changed( std::string& written_text ) {}
};

#endif // TOUCH_KEYBOARD_H