#ifndef NEW_WORDS_H
#define NEW_WORDS_H

#include "freetype_renderer.h"
#include "lesson_menu.h"
#include "drawing_pad.h"
#include "config.h"
#include "settings_dialog.h"

class NewWordsViewer : public NewWordRenderSettings
{
	public:
		FreetypeRenderer& freetype_renderer;
		DrawingPad drawing_pad;
		RenderScreen word_screen, drawing_screen;
		NewWordList& words;
		NewWordList::iterator current_word;
		Config* config;
		TextButton left_button, right_button, exit_button, clear_button, 
			hanzi_tab, pinyin_tab, latin_tab, rating_bar, 
			rating_easy, rating_medium, rating_hard, rating_impossible;
		TextButtonList text_buttons;
		static int BUTTON_ACTIVATION_DRAW_LIMIT;
		Settings settings;
	public:
		NewWordsViewer( FreetypeRenderer& _freetype_renderer, NewWordList& _words, Config* _config=0 );
		void render( Screen screen );
		void run_until_exit();
		void switch_forward();
		void switch_backwards();
};

#endif // NEW_WORDS_H