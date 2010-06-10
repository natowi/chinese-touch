#include "settings_dialog.h"

Settings::~Settings()
{
	for( Settings::iterator s_it = this->begin(); s_it != this->end(); s_it++ )
	{
		if( s_it->second )
			delete s_it->second;
	}
	this->clear();
}

void Settings::add_setting( Setting* setting )
{
	if( setting )
	{
		if( this->count(setting->name) && (*this)[setting->name]!=setting )
		{
			// clean-up old setting:
			if( (*this)[setting->name] )
				delete (*this)[setting->name];
		}
		(*this)[setting->name] = setting;
	}
}

Setting* Settings::get_setting( const std::string& name )
{
	if( this->count(name) )
	{
		return (*this)[name];
	} else return 0;
}

bool Settings::get_boolean_setting( const std::string& name )
{
	Setting* setting = this->get_setting(name);
	if( !setting ) 
		return false;
	BooleanSetting* boolean_setting = dynamic_cast<BooleanSetting*>( setting );
	if( boolean_setting )
		return boolean_setting->value;
	else return false;
}

#include <nds.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>

#include "string.h"

#include "text_button.h"
#include "sprite_helper.h"
#include "error_console.h"
#include "big_bottom_right_button.h"
#include "big_bottom_right_button_active.h"
#include "checkbox.h"
#include "checkbox_active.h"
#include "menu_button.h"
#include "menu_button_active.h"
#include "menu_button_inactive.h"


SettingsDialog::SettingsDialog( Program& _program, int _recursion_depth, Settings& _settings, const std::string& _title )
	: Mode(_program, _recursion_depth), settings(_settings), settings_screen(SCREEN_SUB), title(_title),
		ok_button(settings_screen,"OK",SpriteSize_32x32,settings_screen.res_x-32,settings_screen.res_y-32,_program.ft->latin_face,12,0,8),
		dummy_checkbox(settings_screen,"",SpriteSize_16x16,0,0,_program.ft->latin_face,8,1,1),
		dummy_start_button(settings_screen,"",SpriteSize_32x16,0,0,_program.ft->latin_face,8,1,1)
{
	this->init_mode();
	
	/* Even if we create the menu dynamically, we will render the text just once on the background
		and store the item positions for later sprite updates. The Dialog will not be scrollable 
		that way, but who wants overcomplicated configuration menus anyway? */
	int top = 4; // vertical start offset
	RenderStyle render_style;
	render_style.center_x = true;
	RenderInfo render_info = this->program.ft->render( 
		this->settings_screen, this->title, 
		this->program.ft->latin_face, 14, 0, top, &render_style );
	top += render_info.height + 5;
	memset( this->settings_screen.base_address+this->settings_screen.res_x*(top++)/2, 
			255, this->settings_screen.res_x );
	memset( this->settings_screen.base_address+this->settings_screen.res_x*(top++)/2, 
			64, this->settings_screen.res_x );
	top += 10;
	for( Settings::iterator s_it = this->settings.begin(); s_it != this->settings.end(); s_it++ )
	{
		Setting* setting = s_it->second;
		BooleanSetting* boolean_setting = dynamic_cast<BooleanSetting*>( s_it->second );
		ActionButton* action_button = dynamic_cast<ActionButton*>( s_it->second );
		int left = 10;
		TextButton* new_button = 0;
		if( boolean_setting )
		{
			new_button = new TextButton( this->dummy_checkbox );
			this->checkboxes.add_text_button( boolean_setting->name, new_button );
		}
		else if( action_button )
		{
			new_button = new TextButton( this->dummy_start_button );
			new_button->text = action_button->button_label;
			this->start_buttons.add_text_button( action_button->name, new_button );
		}
		if( new_button )
		{
			new_button->owns_bg_vram = false;
			new_button->owns_text_vram = true;
			new_button->x = left;
			new_button->y = top;
			this->text_buttons.push_back( new_button );
			left += new_button->sensor_width + 5;
		}
		RenderInfo render_info = this->program.ft->render( this->settings_screen, setting->description, 
														this->program.ft->latin_face, 11,
														left, top+1 );
		if( render_info.height < 16 )
			top += 16;
		else top += render_info.height;
		top += 5;
	}

	this->text_buttons.push_back( &this->ok_button );
	this->text_buttons.push_back( &this->dummy_checkbox );
	this->text_buttons.push_back( &this->dummy_start_button );
	
	this->dummy_checkbox.hidden = true;
	this->dummy_start_button.hidden = true;
	
	this->init_vram();
}

void SettingsDialog::init_mode()
{
	this->program.ft->init_screen( this->settings_screen );
	this->settings_screen.clear();
	
	Mode::init_mode();
}

void SettingsDialog::init_vram()
{
	// load sprite graphics into vram:
	this->ok_button.init_vram( big_bottom_right_buttonBitmap, this->ok_button.bg_vram );
	this->ok_button.init_vram( big_bottom_right_button_activeBitmap, this->ok_button.bg_active_vram );
	this->dummy_checkbox.init_vram( checkboxBitmap, this->dummy_checkbox.bg_vram );
	this->dummy_checkbox.init_vram( checkbox_activeBitmap, this->dummy_checkbox.bg_active_vram );
	for( TextButtonMapStorage::iterator bi = this->checkboxes.begin(); bi != this->checkboxes.end(); bi++ )
	{
		bi->second->bg_vram = this->dummy_checkbox.bg_vram;
		bi->second->bg_active_vram = this->dummy_checkbox.bg_active_vram;
	}
	this->dummy_start_button.init_vram( menu_buttonBitmap, this->dummy_start_button.bg_vram );
	this->dummy_start_button.init_vram( menu_button_activeBitmap, this->dummy_start_button.bg_active_vram );
	for( TextButtonMapStorage::iterator bi = this->start_buttons.begin(); bi != this->start_buttons.end(); bi++ )
	{
		bi->second->bg_vram = this->dummy_start_button.bg_vram;
		bi->second->bg_active_vram = this->dummy_start_button.bg_active_vram;
	}
	
	Mode::init_vram();
}

void SettingsDialog::render( Screen screen )
{
	if( screen == SCREEN_MAIN )
	{
	}
	else if( screen == SCREEN_SUB )
	{
		for( Settings::iterator s_it = this->settings.begin(); s_it != this->settings.end(); s_it++ )
		{
			TextButton* checkbox = this->checkboxes.get_text_button( s_it->first );
			if( checkbox && s_it->second && s_it->second->description.length() )
			{
				if( this->settings.get_boolean_setting(s_it->first) )
				{
					checkbox->bg_vram = this->dummy_checkbox.bg_active_vram;
					checkbox->bg_active_vram = this->dummy_checkbox.bg_vram;
				}
				else
				{
					checkbox->bg_vram = this->dummy_checkbox.bg_vram;
					checkbox->bg_active_vram = this->dummy_checkbox.bg_active_vram;
				}
			}
		}
	}
	
	Mode::render( screen );
}

ButtonAction SettingsDialog::handle_button_pressed(TextButton* text_button)
{
	if( text_button == &(this->ok_button) )
	{
		return BUTTON_ACTION_EXIT_MODE;
	}
	else if( this->settings.count(text_button->name) )
	{
		Setting* setting = this->settings.get_setting( text_button->name );
		BooleanSetting* boolean_setting = dynamic_cast<BooleanSetting*>( setting );
		ActionButton* action_button = dynamic_cast<ActionButton*>( setting );
		if( boolean_setting )
		{
			boolean_setting->value = !boolean_setting->value;
			return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
		}
		else if( action_button )
		{
			action_button->run_action();
			return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
		}
	}
	
	return ButtonProvider::handle_button_pressed(text_button);
}
