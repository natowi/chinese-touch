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


SettingsDialog::SettingsDialog( FreetypeRenderer& _freetype_renderer, Settings& _settings, const std::string& _title )
	: freetype_renderer(_freetype_renderer), settings(_settings), title(_title),
		ok_button(&oamSub,"OK",SpriteSize_32x32,settings_screen.res_x-32,settings_screen.res_y-32,freetype_renderer.latin_face,12,0,8),
		dummy_checkbox(&oamSub,"",SpriteSize_16x16,0,0,freetype_renderer.latin_face,8,1,1),
		dummy_start_button(&oamSub,"",SpriteSize_32x16,0,0,freetype_renderer.latin_face,8,1,1)
{
	this->freetype_renderer.init_screen( SCREEN_SUB, this->settings_screen );
	this->settings_screen.clear();

	// unteren Bildschirm für Spritenutzung initialisieren:
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	oamAllocReset( &oamSub );
	oamEnable( &oamSub );

	// vorgerenderte Spritegrafiken laden:
	this->ok_button.init_vram( big_bottom_right_buttonBitmap, this->ok_button.bg_vram );
	this->ok_button.init_vram( big_bottom_right_button_activeBitmap, this->ok_button.bg_active_vram );
	this->dummy_checkbox.init_vram( checkboxBitmap, this->dummy_checkbox.bg_vram );
	this->dummy_checkbox.init_vram( checkbox_activeBitmap, this->dummy_checkbox.bg_active_vram );
	this->dummy_start_button.init_vram( menu_buttonBitmap, this->dummy_start_button.bg_vram );
	this->dummy_start_button.init_vram( menu_button_activeBitmap, this->dummy_start_button.bg_active_vram );
	
	/* Even if we create the menu dynamically, we will render the text just once on the background
		and store the item positions for later sprite updates. The Dialog will not be scrollable 
		that way, but who wants overcomplicated configuration menus anyway? */
	int top = 4; // vertical start offset
	RenderStyle render_style;
	render_style.center_x = true;
	RenderInfo render_info = this->freetype_renderer.render( 
		this->settings_screen, this->title, 
		this->freetype_renderer.latin_face, 14, 0, top, &render_style );
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
		if( boolean_setting )
		{
			TextButton* new_button = new TextButton( &oamSub, "", SpriteSize_16x16, 
													 left, top, freetype_renderer.latin_face, 8,1,1 );
			new_button->owns_bg_vram = false;
			new_button->bg_vram = this->dummy_checkbox.bg_vram;
			new_button->bg_active_vram = this->dummy_checkbox.bg_active_vram;
			this->checkboxes.add_text_button( boolean_setting->name, new_button );
			this->text_buttons.push_back( new_button );
			left += new_button->width + 5;
		}
		else if( action_button )
		{
			TextButton* new_button = new TextButton( &oamSub, action_button->button_label, SpriteSize_32x16, 
													 left, top, freetype_renderer.latin_face, 8,1,1 );
			new_button->owns_bg_vram = false;
			new_button->bg_vram = this->dummy_start_button.bg_vram;
			new_button->bg_active_vram = this->dummy_start_button.bg_active_vram;
			this->start_buttons.add_text_button( action_button->name, new_button );
			this->text_buttons.push_back( new_button );
			left += new_button->width + 5;
		}
		RenderInfo render_info = this->freetype_renderer.render( this->settings_screen, setting->description, 
																freetype_renderer.latin_face, 11,
																left, top+1 );
		if( render_info.height < 16 )
			top += 16;
		else top += render_info.height;
		top += 5;
	}

	this->text_buttons.push_back( &this->ok_button );
	TextButtonList init_list;
	init_list.insert( init_list.end(), this->text_buttons.begin(), this->text_buttons.end() );
	init_list.push_back( &this->dummy_checkbox );
	for( TextButtonList::iterator i=init_list.begin(); i!=init_list.end(); i++ )
	{
		(*i)->init_text_layer( this->freetype_renderer );
	}
}

void SettingsDialog::render( Screen screen )
{
	if( screen == SCREEN_MAIN )
	{
	}
	else if( screen == SCREEN_SUB )
	{
		oamClear( &oamSub, 0, 0 );
		int oam_entry = 0;
		this->ok_button.render_to( oam_entry );
		
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
				checkbox->render_to( oam_entry );
			}
			TextButton* start_button = this->start_buttons.get_text_button( s_it->first );
			if( start_button && s_it->second && s_it->second->description.length() )
			{
				start_button->render_to( oam_entry );
			}
		}
		// gepufferte Bilddaten einblenden bzw. in den VRAM kopieren:
		swiWaitForVBlank();
		oamUpdate( &oamSub );
	}
}

void SettingsDialog::run_until_exit()
{
	this->render( SCREEN_SUB );
	touchPosition old_touch;
    touchRead( &old_touch );
	bool touched = false;
    while( true )
    {
        scanKeys();
		int pressed = keysDown();
		int released = keysUp();
		int held = keysHeld();
		if( held & KEY_SELECT && pressed & KEY_UP )
		{
			ErrorConsole::init_screen( SCREEN_MAIN );
		}
		if( held & KEY_SELECT && pressed & KEY_DOWN )
		{
			ErrorConsole::init_screen( SCREEN_SUB );
		}
		if( held & KEY_SELECT && pressed & KEY_LEFT )
		{
			ErrorConsole::clear();
		}
		if( held & KEY_SELECT && pressed & KEY_RIGHT )
		{
			ErrorConsole::dump();
		}
		
		touchPosition touch;
        touchRead( &touch );
        if( keysCurrent() & KEY_TOUCH )
        {
			if( !touched ) 
			{
				touched = true;
				old_touch = touch;
			}
			bool changed = false;
			for( TextButtonList::iterator b_it = this->text_buttons.begin(); 
					b_it != this->text_buttons.end(); b_it++ )
			{
				if( (*b_it)->is_responsible(touch.px, touch.py) )
				{
					changed |= !(*b_it)->active;
					(*b_it)->active = true;
				}
				else
				{
					changed |= (*b_it)->active;
					(*b_it)->active = false;
				}
			}
			if( changed ) 
				this->render( SCREEN_SUB );
			old_touch = touch;
		}
		else if( touched )
		{
			touched = false;
			for( TextButtonList::iterator b_it = this->text_buttons.begin(); 
					b_it != this->text_buttons.end(); b_it++ )
			{
				if( (*b_it)->is_responsible(old_touch.px, old_touch.py)
					&& (*b_it)->active )
				{
					(*b_it)->active = false;
					if( (*b_it) == &(this->ok_button) )
					{
						return;
					}
					else if( this->settings.count((*b_it)->name) )
					{
						Setting* setting = this->settings.get_setting( (*b_it)->name );
						BooleanSetting* boolean_setting = dynamic_cast<BooleanSetting*>( setting );
						ActionButton* action_button = dynamic_cast<ActionButton*>( setting );
						if( boolean_setting )
						{
							boolean_setting->value = !boolean_setting->value;
						}
						else if( action_button )
						{
							action_button->run_action();
						}
					}
					this->render( SCREEN_SUB );
				}
			}
		}
		else
		{
			swiWaitForVBlank();
			touched = false;
			bool changed = false;
			for( TextButtonList::iterator i=this->text_buttons.begin(); 
				i!=this->text_buttons.end(); i++ )
			{
				changed |= (*i)->active;
				(*i)->active = false;
			}
			if( changed )
				this->render( SCREEN_SUB );
		}
    }
}
