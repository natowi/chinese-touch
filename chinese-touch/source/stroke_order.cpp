#include <cmath>

#include "stroke_order.h"

#include "top_left_button.h"
#include "top_left_button_active.h"
#include "top_right_button.h"
#include "top_right_button_active.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "bottom_right_button.h"
#include "bottom_right_button_active.h"
#include "right_center_button.h"
#include "right_center_button_active.h"
#include "image_png.h"


int StrokeOrderViewer::BUTTON_ACTIVATION_DRAW_LIMIT = 5;

StrokeOrderViewer::StrokeOrderViewer( Program& _program, int _recursion_depth, NewWord& _word )
	: Mode(_program, _recursion_depth), word_screen(SCREEN_MAIN), drawing_screen(SCREEN_SUB),
		drawing_pad(drawing_screen), word(_word),
		left_button(drawing_screen,"<",SpriteSize_32x16,0,0,button_ft.latin_face,10,0,0), 
		right_button(drawing_screen,">",SpriteSize_32x16,drawing_screen.res_x-32,0,button_ft.latin_face,10,2,0), 
		exit_button(drawing_screen,"x",SpriteSize_16x16,0,drawing_screen.res_y-16,_program.ft->latin_face,10,-1,1),
		clear_button(drawing_screen,"C\nL\nE\nA\nR",SpriteSize_32x64,drawing_screen.res_x-16,drawing_screen.res_y/2-32,_program.ft->latin_face,9,-7,3),
		settings_button(drawing_screen,"s",SpriteSize_16x16,drawing_screen.res_x-16,drawing_screen.res_y-16,_program.ft->latin_face,10,1,1),
		pixels_drawn(0), clear_on_switch(true)
{
	utf8_to_utf8_char_list( (const unsigned char*)this->word.hanzi.c_str(), this->char_list );
	// TODO: jump to character possibly selected in parent mode?
	this->current_char = this->char_list.begin();
	
	this->render_settings.render_translation = false;
	
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new BooleanSetting("1_clear_screen","Clear Writing Screen on Switch",this->clear_on_switch) );
	
	this->text_buttons.push_back( &this->left_button );
	this->text_buttons.push_back( &this->right_button );
	this->text_buttons.push_back( &this->exit_button );
	this->text_buttons.push_back( &this->clear_button );
	this->text_buttons.push_back( &this->settings_button );
	
	this->init_mode();
	this->init_vram();
}

void StrokeOrderViewer::init_mode()
{
	this->Mode::button_ft.init_screen( this->word_screen );
	this->word_screen.clear();
	
	this->Mode::button_ft.init_screen( this->drawing_screen );
	this->drawing_screen.clear();

	Mode::init_mode();
}

void StrokeOrderViewer::init_vram()
{
	int width=this->word_screen.res_x, height=this->word_screen.res_y;
	read_png( this->program.base_dir+"/stroke-order/test2.png", this->word_screen.bg_base_address, width, height );
	
	Mode::init_vram();
}

void StrokeOrderViewer::init_button_vram()
{
	this->left_button.init_vram( top_left_buttonBitmap, this->left_button.bg_vram );
	this->left_button.init_vram( top_left_button_activeBitmap, this->left_button.bg_active_vram );
	this->right_button.init_vram( top_right_buttonBitmap, this->right_button.bg_vram );
	this->right_button.init_vram( top_right_button_activeBitmap, this->right_button.bg_active_vram );
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	this->clear_button.init_vram( right_center_buttonBitmap, this->clear_button.bg_vram );
	this->clear_button.init_vram( right_center_button_activeBitmap, this->clear_button.bg_active_vram );
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	
	ButtonProvider::init_button_vram();
}

void StrokeOrderViewer::show_settings()
{
	this->free_vram();
	SettingsDialog settings_dialog( this->program, this->recursion_depth, this->settings, "Stroke Order View Settings" );
	settings_dialog.run_until_exit();
	this->init_mode();
	this->init_vram();
}

void StrokeOrderViewer::render( Screen screen )
{
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		this->word.render( *this->program.ft, this->word_screen, this->render_settings, *this->program.library );
	}
	
	Mode::render( screen );
}

void StrokeOrderViewer::render_buttons(OamState* oam_state, int& oam_entry)
{
#if 0
	// hide left button when at the beginning of the word list:
	if( this->current_word == this->words.begin() ) this->left_button.hidden = this->left_button.disabled = true;
	else this->left_button.hidden = this->left_button.disabled = false;
	// hide right button when at the end of the word list:
	NewWordList::iterator test_it = this->current_word;
	if( this->current_word != this->words.end() ) test_it++;
	if( test_it == this->words.end() ) this->right_button.hidden = this->right_button.disabled = true;
	else this->right_button.hidden = this->right_button.disabled = false;
#endif

	ButtonProvider::render_buttons(oam_state, oam_entry);
}


ButtonAction StrokeOrderViewer::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->left_button && this->switch_backwards() )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	if( text_button == &this->right_button && this->switch_forward() )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	if( text_button == &this->clear_button )
	{
		this->drawing_pad.clear();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->exit_button )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->settings_button )
	{
		this->show_settings();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( this->clear_on_switch
		&& (text_button == &this->left_button
			|| text_button == &this->right_button) )
	{
		this->drawing_pad.clear();
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}

bool StrokeOrderViewer::switch_forward()
{
	if( this->current_char != this->char_list.end() )
	{
		StringList::iterator test_it = this->current_char;
		if( ++test_it != this->char_list.end() )
		{
			this->current_char++;
			// FIXME?: this->restore_init_settings_if_needed();
			return true;
		}
	}
	return false;
}

bool StrokeOrderViewer::switch_backwards()
{
	if( this->current_char != this->char_list.begin() )
	{
		this->current_char--;
		// FIXME?: this->restore_init_settings_if_needed();
		return true;
	}
	return false;
}

ButtonAction StrokeOrderViewer::handle_console_button_event( int pressed, int held, int released )
{
	// delegate console L and R buttons to touch screen left/right button handlers:
	if( pressed & KEY_L )
	{
		return this->handle_button_pressed( &this->left_button );
	}
	else if( pressed & KEY_R )
	{
		return this->handle_button_pressed( &this->right_button );
	}
	
	return ButtonProvider::handle_console_button_event( pressed, held, released );
}

ButtonAction StrokeOrderViewer::handle_touch_begin( touchPosition touch )
{
	this->old_touch = touch;
	this->old_distance = 0;
	this->pixels_drawn = 0;
	// check for touch screen button activation:
	ButtonAction action = Mode::handle_touch_begin( touch );
	// only draw a pixel if no button was activated by the touch:
	if( action == BUTTON_ACTION_UNHANDLED )
	{
		this->drawing_pad.draw_point( touch.px, touch.py );
		this->pixels_drawn += 1;
		action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return action;
}

ButtonAction StrokeOrderViewer::handle_touch_drag( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	int x_diff = touch.px - this->old_touch.px;
	int y_diff = touch.py - this->old_touch.py;
	int distance = (int)std::sqrt( std::pow(x_diff,2) + std::pow(y_diff,2) );
	// check for touch screen button activation, but only if the users
	// is not already writing to the screen:
	if( this->pixels_drawn < StrokeOrderViewer::BUTTON_ACTIVATION_DRAW_LIMIT )
	{
		action |= Mode::handle_touch_drag( touch );
	}
	// if we are not hovering a touch screen button, draw a line:
	if( (action == BUTTON_ACTION_UNHANDLED)
		&& (distance 
			&& ((this->old_distance && (distance <= this->old_distance*DrawingPad::MAX_ACCELERATION_FACTOR)) 
				|| (distance <= DrawingPad::MAX_ACCELERATION_FACTOR))) )
	{
		this->drawing_pad.draw_line( touch.px, touch.py, this->old_touch.px, this->old_touch.py );
		this->pixels_drawn += distance;
		action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
	}
	this->old_distance = distance;
	this->old_touch = touch;
	
	return action;
}

ButtonAction StrokeOrderViewer::handle_touch_end( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// check for touch screen button activation, but only if the users
	// is not already writing to the screen:
	if( this->pixels_drawn < StrokeOrderViewer::BUTTON_ACTIVATION_DRAW_LIMIT )
	{
		action |= Mode::handle_touch_end( touch );
	}
	if( action == BUTTON_ACTION_UNHANDLED )
		action |= this->handle_touch_drag( touch );
	return action;
}
