#ifndef TOUCH_KEYBOARD_H
#define TOUCH_KEYBOARD_H

#include "mode.h"
#include "ui_language.h"
#include "freetype_renderer.h"
#include "text_button.h"

typedef std::map<std::string,std::string> StringMap;

class TouchKeyboard : public Mode
{
private:
	UILanguage& ui_lang;
	FreetypeRenderer& freetype_renderer;
	RenderScreen keyboard_screen;
	TextButton ok_button, reference_key;
	TextButtonList text_buttons;
	TextButtonListStorage keys;
	std::string written_text, modifier;
	StringMap modifier_map;
public:
	TouchKeyboard( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer );
	void render( Screen screen );
	void run_until_exit();
};

#endif // TOUCH_KEYBOARD_H