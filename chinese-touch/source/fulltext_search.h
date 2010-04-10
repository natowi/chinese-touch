#ifndef FULLTEXT_SEARCH_H
#define FULLTEXT_SEARCH_H

#include "touch_keyboard.h"
#include "lesson.h"
#include "settings_dialog.h"


class FulltextSearch : public TouchKeyboard, public NewWordRenderSettings
{
public:
	FulltextSearch( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, Library& _library );
	virtual void init_mode();
	virtual void init_vram();
	virtual void render( Screen screen );
	virtual ButtonAction handle_button_pressed( TextButton* text_button );
public:
	RenderScreen word_screen;
	Library& library;
	NewWordList words;
	NewWordList::iterator current_word;
	TextButton left_button, right_button,
		hanzi_tab, pinyin_tab, latin_tab, rating_bar,
		rating_easy, rating_medium, rating_hard, rating_impossible,
		settings_button, down_button, search_button;
	Settings settings;
};

#endif // FULLTEXT_SEARCH_H