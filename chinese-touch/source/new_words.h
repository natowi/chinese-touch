#ifndef NEW_WORDS_H
#define NEW_WORDS_H

#include "freetype_renderer.h"
#include "lesson_menu.h"
#include "drawing_pad.h"
#include "config.h"
#include "settings_dialog.h"


/*! Base class for modes with word list browsing capabilities */
class WordListBrowser : public ButtonProvider
{
	public:
		NewWordList& words;
		NewWordList::iterator current_word;
		RenderScreen& button_screen;
		Library& library;
		bool render_foreign_word, render_pronuciation, render_translation;
		bool init_render_foreign_word, init_render_pronuciation, init_render_translation;
		bool restore_on_switch, clear_on_switch;
		TextButton left_button, right_button, 
			foreign_word_tab, pronunciation_tab, translation_tab, 
			rating_bar, 
			rating_easy, rating_medium, rating_hard, rating_impossible,
			down_button, add_button, remove_button;
	public:
		WordListBrowser( ButtonProviderList& provider_list, 
						 FreetypeRenderer& _freetype_renderer, 
						 NewWordList& _words, 
						 RenderScreen& _button_screen,
						 Library& _library );
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
		virtual void init_button_vram();
		virtual void render_buttons( OamState* oam_state, int& oam_entry );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		bool switch_forward();
		bool switch_backwards();
		virtual ButtonAction handle_console_button_event( int pressed, int held, int released );
};


class NewWordsViewer : public Mode
{
	public:
		RenderScreen word_screen, drawing_screen;
		WordListBrowser word_browser;
		DrawingPad drawing_pad;
		Library& library;
		Config* config;
		TextButton exit_button, clear_button, settings_button;
		static int BUTTON_ACTIVATION_DRAW_LIMIT;
		Settings settings;
		touchPosition old_touch;
		int old_distance;
		int pixels_drawn;
	public:
		NewWordsViewer( FreetypeRenderer& _freetype_renderer, NewWordList& _words, Library& _library, Config* _config=0 );
		void init_mode();
		void init_vram();
		void init_button_vram();
		void show_settings();
		void render( Screen screen );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		virtual ButtonAction handle_touch_begin( touchPosition touch );
		virtual ButtonAction handle_touch_drag( touchPosition touch );
		virtual ButtonAction handle_touch_end( touchPosition touch );
};

#endif // NEW_WORDS_H