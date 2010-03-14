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

Setting* Settings::get_setting( std::string name )
{
	if( this->count(name) )
	{
		return (*this)[name];
	} else return 0;
}

bool Settings::get_boolean_setting( std::string name )
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


SettingsDialog::SettingsDialog( FreetypeRenderer& _freetype_renderer, Settings& _settings )
	: freetype_renderer(_freetype_renderer), settings(_settings),
		ok_button(&oamSub,"OK",SpriteSize_32x32,settings_screen.res_x-32,settings_screen.res_y-32,freetype_renderer.latin_face,12,0,4),
		dummy_checkbox(&oamSub,"",SpriteSize_16x16,0,0,freetype_renderer.latin_face,10,1,1)
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
	
	/* Even if we create the menu dynamically, we will render the text just once on the background
		and store the item positions for later sprite updates. The Dialog will not be scrollable 
		that way, but who wants overcomplicated configuration menus anyway? */
	int top = 4; // vertical start offset
	RenderStyle render_style;
	render_style.center_x = true;
	RenderInfo render_info = this->freetype_renderer.render( 
		this->settings_screen, "Word List - Settings", 
		this->freetype_renderer.latin_face, 14, 0, top, &render_style );
	top += render_info.height + 5;
	memset( this->settings_screen.base_address+this->settings_screen.res_x*(top++)/2, 
			255, this->settings_screen.res_x );
	memset( this->settings_screen.base_address+this->settings_screen.res_x*(top++)/2, 
			64, this->settings_screen.res_x );
	top += 10;
	for( Settings::iterator s_it = this->settings.begin(); s_it != this->settings.end(); s_it++ )
	{
		BooleanSetting* boolean_setting = dynamic_cast<BooleanSetting*>( s_it->second );
		if( boolean_setting )
		{
			TextButton* new_checkbox = new TextButton( &oamSub, "", SpriteSize_16x16, 10, top, freetype_renderer.latin_face, 11 );
			new_checkbox->owns_bg_vram = false;
			new_checkbox->bg_vram = this->dummy_checkbox.bg_vram;
			new_checkbox->bg_active_vram = this->dummy_checkbox.bg_active_vram;
			this->checkboxes.add_text_button( boolean_setting->name, new_checkbox );
			this->text_buttons.push_back( new_checkbox );
			RenderInfo render_info = this->freetype_renderer.render( this->settings_screen, s_it->second->description, 
																	new_checkbox->face, new_checkbox->font_size, 
																	new_checkbox->x+21, top+1 );
			if( render_info.height < 16 )
				top += 16;
			else top += render_info.height;
			top += 5;
		}
	}

	this->text_buttons.push_back( &this->ok_button );
	TextButtonList init_list;
	init_list.insert( init_list.end(), this->text_buttons.begin(), this->text_buttons.end() );
	init_list.push_back( &this->dummy_checkbox );
	for( TextButtonList::iterator i=init_list.begin(); i!=init_list.end(); i++ )
	{
		if( (*i)->owns_bg_vram )
		{
			// Alpha-Bits bei definierten Spritepixeln auf "undurchsichtig" setzen:
			if( (*i)->bg_vram ) set_16bpp_sprite_opague( (*i)->bg_vram, (*i)->width, (*i)->height, 0 );
			if( (*i)->bg_active_vram ) set_16bpp_sprite_opague( (*i)->bg_active_vram, (*i)->width, (*i)->height, 0 );
			if( (*i)->bg_inactive_vram ) set_16bpp_sprite_opague( (*i)->bg_inactive_vram, (*i)->width, (*i)->height, 0 );
		}
		if( (*i)->text.length() )
		{
			// VRAM für 8-Bit-Buttonbeschriftungs-Sprites reservieren:
			(*i)->text_vram = oamAllocateGfx( &oamSub, (*i)->sprite_size, SpriteColorFormat_256Color );
			RenderScreenBuffer button_text( (*i)->width, (*i)->height );
			RenderStyle render_style;
			render_style.center_x = true;
			this->freetype_renderer.render( button_text, (*i)->text, 
				(*i)->face, (*i)->font_size, (*i)->text_x_offset, (*i)->text_y_offset, &render_style );
			// Spritekonvertierung:
			// (Zwischenpufferung aus Bequemlichkeit, weil VRAM nur mit 16-bit-Wörtern beschreibbbar)
			u8 conversion_buffer[(*i)->width * (*i)->height];
			tile_8bpp_sprite( (u8*)(button_text.base_address), conversion_buffer, (*i)->width, (*i)->height );
			memcpy( (*i)->text_vram, conversion_buffer, (*i)->width * (*i)->height * 1 );
		}
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
						BooleanSetting* boolean_setting = dynamic_cast<BooleanSetting*>( this->settings.get_setting((*b_it)->name) );
						if( boolean_setting )
						{
							boolean_setting->value = !boolean_setting->value;
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
