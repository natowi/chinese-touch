#ifndef NEW_WORDS_H
#define NEW_WORDS_H

#include "freetype_renderer.h"
#include "lesson_menu.h"
#include "drawing_pad.h"
#include "config.h"

class NewWordsViewer
{
	public:
		FreetypeRenderer& freetype_renderer;
		DrawingPad drawing_pad;
		RenderScreen word_screen, drawing_screen;
		Lesson& lesson;
		unsigned int word_index;
		Config& config;
		TextButton left_button, right_button, exit_button, clear_button, 
			hanzi_tab, pinyin_tab, latin_tab, rating_bar, 
			rating_happy, rating_ok, rating_confused, rating_dead;
		TextButtonList text_buttons;
		static int BUTTON_ACTIVATION_DRAW_LIMIT;
	public:
		NewWordsViewer( FreetypeRenderer& _freetype_renderer, Lesson& _lesson, Config& _config );
		void render( Screen screen );
		void run_until_exit();
};

#endif // NEW_WORDS_H