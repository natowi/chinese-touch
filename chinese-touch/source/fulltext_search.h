#ifndef FULLTEXT_SEARCH_H
#define FULLTEXT_SEARCH_H

#include "touch_keyboard.h"
#include "lesson.h"
#include "settings_dialog.h"
#include "new_words.h"


class FulltextSearch : public Mode
{
public:
	FulltextSearch( UILanguage& _ui_language, FreetypeRenderer& _freetype_renderer, Library& _library );
	virtual void init_mode();
	virtual void init_vram();
	virtual void init_button_vram();
	virtual void render( Screen screen );
	virtual ButtonAction handle_button_pressed( TextButton* text_button );
public:
	UILanguage& ui_language;
	RenderScreen word_screen, keyboard_screen;
	Library& library;
	TouchKeyboard touch_keyboard;
	NewWordList current_words;
	WordListBrowser word_browser;
	TextButton settings_button, search_button, clear_button;
	Settings settings;
	std::string prev_rendered_text;
};

#endif // FULLTEXT_SEARCH_H