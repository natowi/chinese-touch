#ifndef TOUCH_KEYBOARD_H
#define TOUCH_KEYBOARD_H

#include "mode.h"
#include "ui_language.h"
#include "freetype_renderer.h"
#include "text_button.h"

typedef std::map<std::string,std::string> StringMap;

class TouchKeyboard : public ButtonProvider
{
public:
	UILanguage& ui_lang;
	RenderScreen& keyboard_screen;
	TextButton reference_key;
	TextButtonSetStorage keys;
	StringList written_chars;
	std::string modifier;
	StringMap modifier_map;
public:
	TouchKeyboard( ButtonProviderList& button_provider_list, UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, RenderScreen& _keyboard_screen );
	virtual void init_button_vram();
	virtual ButtonAction handle_button_pressed( TextButton* text_button );
	virtual std::string handle_key_pressed( const std::string& input ) { return input; }
	virtual void handle_text_changed() {}
	virtual std::string get_written_text();
	virtual void set_written_text( const std::string& text );
};

#endif // TOUCH_KEYBOARD_H