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
	FreetypeRenderer& freetype_renderer;
	RenderScreen keyboard_screen;
	TextButton reference_key, exit_button;
	TextButtonList text_buttons;
	TextButtonListStorage keys;
	std::string written_text, modifier;
	StringMap modifier_map;
public:
	TouchKeyboard( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer );
	void init_screens();
	virtual void handle_init_screens() {};
	virtual void render_prepare() {}
	void render( Screen screen );
	void run_until_exit();
	virtual bool handle_button_pressed( TextButton* text_button ) { return false; }
	virtual std::string handle_key_pressed( const std::string& input ) { return input; }
	virtual void handle_text_changed( std::string& written_text ) {}
};

#endif // TOUCH_KEYBOARD_H