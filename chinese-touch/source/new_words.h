#ifndef NEW_WORDS_H
#define NEW_WORDS_H

#include "freetype_renderer.h"
#include "lesson_menu.h"
#include "drawing_pad.h"
#include "config.h"
#include "settings_dialog.h"


/*! Stores which parts of dictionary entries shall be displayed on top screen. */
class NewWordRenderSettings
{
	public:
		bool render_foreign_word, render_pronuciation, render_translation;
		bool init_render_foreign_word, init_render_pronuciation, init_render_translation;
		bool restore_on_switch, clear_on_switch;
	public:
		NewWordRenderSettings() 
			: render_foreign_word(true), render_pronuciation(true), render_translation(true),
				init_render_foreign_word(true), init_render_pronuciation(true), init_render_translation(true),
				restore_on_switch(true), clear_on_switch(true) {}
		void toggle_foreign_word() { this->render_foreign_word = !this->render_foreign_word; }
		void toggle_pronunciation() { this->render_pronuciation = !this->render_pronuciation; }
		void toggle_translation() { this->render_translation = !this->render_translation; }
		void restore_init_settings()
		{
			this->render_foreign_word = this->init_render_foreign_word;
			this->render_pronuciation = this->init_render_pronuciation;
			this->render_translation = this->init_render_translation;
		}
		void restore_init_settings_if_needed() { if(this->restore_on_switch) this->restore_init_settings(); }
};


class NewWordsViewer : public NewWordRenderSettings
{
	public:
		FreetypeRenderer& freetype_renderer;
		DrawingPad drawing_pad;
		RenderScreen word_screen, drawing_screen;
		NewWordList& words;
		NewWordList::iterator current_word;
		Library& library;
		Config* config;
		TextButton left_button, right_button, exit_button, clear_button, 
			hanzi_tab, pinyin_tab, latin_tab, rating_bar, 
			rating_easy, rating_medium, rating_hard, rating_impossible,
			settings_button, down_button;
		TextButtonList text_buttons;
		static int BUTTON_ACTIVATION_DRAW_LIMIT;
		Settings settings;
	public:
		NewWordsViewer( FreetypeRenderer& _freetype_renderer, NewWordList& _words, Library& _library, Config* _config=0 );
		void init_subscreen();
		void show_settings();
		void render( Screen screen );
		void run_until_exit();
		void switch_forward();
		void switch_backwards();
};

#endif // NEW_WORDS_H