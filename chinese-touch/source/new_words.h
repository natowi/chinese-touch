#ifndef NEW_WORDS_H
#define NEW_WORDS_H

#include "chinese-touch.h"
#include "freetype_renderer.h"
#include "lesson_menu.h"
#include "drawing_pad.h"
#include "config.h"
#include "settings_dialog.h"
#include "ui_language.h"


class RenderSettings
{
public:
	bool render_foreign_word, render_pronuciation, render_translation, render_stroke_order;
	bool init_render_foreign_word, init_render_pronuciation, init_render_translation, init_render_stroke_order;
	bool restore_on_switch;
	int stroke_order_scroll_left, stroke_order_scroll_top;
	UCChar stroke_order_image_char, highlight_char;
	u16* stroke_order_image_buffer;
	int stroke_order_image_buffer_width, stroke_order_image_buffer_height;
	bool stroke_order_full_update;
public:
	RenderSettings();
	~RenderSettings();
	void free_buffers();
};

/*! Base class for modes with word list browsing capabilities */
class WordListBrowser : public ButtonProvider, public RenderSettings
{
	public:
		NewWordList& words;
		NewWordList::iterator current_word;
		RenderScreen& button_screen;
		Library& library;
		TextButton left_button, right_button, 
			as_text_tab, foreign_word_tab, pronunciation_tab, translation_tab, stroke_order_tab, 
			rating_bar, 
			rating_easy, rating_medium, rating_hard, rating_impossible,
			add_button, remove_button, search_button;
		UCCharList current_char_list;
		UCCharList::iterator current_char;
	public:
		WordListBrowser( ButtonProviderList& provider_list, 
						 FreetypeRenderer& _freetype_renderer, 
						 NewWordList& _words, 
						 RenderScreen& _button_screen,
						 Library& _library );
		void toggle_foreign_word();
		void toggle_pronunciation();
		void toggle_translation();
		void toggle_stroke_order();
		void restore_init_settings();
		void restore_init_settings_if_needed();
		virtual void init_button_vram();
		virtual void free_button_vram();
		virtual void render_buttons( OamState* oam_state, int& oam_entry );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		bool switch_forward();
		bool switch_backwards();
		virtual ButtonAction handle_console_button_event( int pressed, int held, int released );
		void randomize_list();
};


class NewWordsViewer : public Mode
{
	public:
		bool save_position;
		RenderScreen word_screen, drawing_screen;
		WordListBrowser word_browser;
		DrawingPad drawing_pad;
		TextButton exit_button, clear_button, settings_button, 
				scroll_field_overlay_0, scroll_field_overlay_1, scroll_field_overlay_2, scroll_field_overlay_3;
		static int BUTTON_ACTIVATION_DRAW_LIMIT;
		Settings settings;
		touchPosition old_touch;
		int old_distance;
		int pixels_drawn;
		bool clear_on_switch, randomize_list;
		bool scrolling;
	public:
		NewWordsViewer( Program& _program, int _recursion_depth, NewWordList& _words, bool _save_position, bool _randomize_list, bool _show_settings );
		void init_mode();
		void init_vram();
		void init_button_vram();
		virtual ~NewWordsViewer();
		void show_settings();
		void render( Screen screen );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		virtual ButtonAction handle_touch_begin( touchPosition touch );
		virtual ButtonAction handle_touch_drag( touchPosition touch );
		virtual ButtonAction handle_touch_end( touchPosition touch );
};

#endif // NEW_WORDS_H