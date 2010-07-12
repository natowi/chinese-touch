#ifndef STROKE_ORDER_H
#define STROKE_ORDER_H

#include "mode.h"
#include "drawing_pad.h"
#include "settings_dialog.h"
#include "new_words.h"

class StrokeOrderViewer : public Mode
{
	public:
		RenderScreen word_screen, drawing_screen;
		DrawingPad drawing_pad;
		RenderSettings render_settings;
		NewWord& word;
		StringList char_list;
		StringList::iterator current_char;
		TextButton left_button, right_button, exit_button, clear_button, settings_button;
		static int BUTTON_ACTIVATION_DRAW_LIMIT;
		Settings settings;
		touchPosition old_touch;
		int old_distance;
		int pixels_drawn;
		bool clear_on_switch;
	public:
		StrokeOrderViewer( Program& _program, int _recursion_depth, NewWord& word );
		virtual void init_mode();
		virtual void init_vram();
		virtual void init_button_vram();
		void show_settings();
		virtual void render( Screen screen );
		virtual void render_buttons( OamState* oam_state, int& oam_entry );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		bool switch_forward();
		bool switch_backwards();
		virtual ButtonAction handle_console_button_event( int pressed, int held, int released );
		virtual ButtonAction handle_touch_begin( touchPosition touch );
		virtual ButtonAction handle_touch_drag( touchPosition touch );
		virtual ButtonAction handle_touch_end( touchPosition touch );
};

#endif // STROKE_ORDER_H