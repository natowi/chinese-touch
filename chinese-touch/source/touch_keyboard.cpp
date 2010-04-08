#include <nds/arm9/sprite.h>

#include "touch_keyboard.h"
#include "freetype_renderer.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "checkbox.h"
#include "checkbox_active.h"
#include "error_console.h"
#include "greys256.h"


TouchKeyboard::TouchKeyboard( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer )  
//SettingsDialog::SettingsDialog( FreetypeRenderer& _freetype_renderer, Settings& _settings, const std::string& _title )
	: ui_lang(_ui_lang), freetype_renderer(_freetype_renderer),
		reference_key(&oamSub,"",SpriteSize_16x16,0,0,freetype_renderer.han_face,9,0,1),
		exit_button(&oamSub,"x",SpriteSize_16x16,0,keyboard_screen.res_y-16,freetype_renderer.latin_face,10,-1,1)
{
	this->text_buttons.push_back( &this->exit_button );
	
	this->modifier_map["¯a"] = "ā";
	this->modifier_map["¯e"] = "ē";
	this->modifier_map["¯i"] = "ī";
	this->modifier_map["¯o"] = "ō";
	this->modifier_map["¯u"] = "ū";
	this->modifier_map["¯ü"] = "ǖ";
	this->modifier_map["¯v"] = "ǖ";
	this->modifier_map["´a"] = "á";
	this->modifier_map["´e"] = "é";
	this->modifier_map["´i"] = "í";
	this->modifier_map["´o"] = "ó";
	this->modifier_map["´u"] = "ú";
	this->modifier_map["´ü"] = "ǘ";
	this->modifier_map["´v"] = "ǘ";
	this->modifier_map["ˇa"] = "ǎ";
	this->modifier_map["ˇe"] = "ě";
	this->modifier_map["ˇi"] = "ǐ";
	this->modifier_map["ˇo"] = "ǒ";
	this->modifier_map["ˇu"] = "ǔ";
	this->modifier_map["ˇü"] = "ǚ";
	this->modifier_map["ˇv"] = "ǚ";
	this->modifier_map["`a"] = "à";
	this->modifier_map["`e"] = "è";
	this->modifier_map["`i"] = "ì";
	this->modifier_map["`o"] = "ò";
	this->modifier_map["`u"] = "ù";
	this->modifier_map["`ü"] = "ǜ";
	this->modifier_map["`v"] = "ǜ";
}

void TouchKeyboard::init_screens()
{
	this->freetype_renderer.init_screen( SCREEN_SUB, this->keyboard_screen );
	this->keyboard_screen.clear();

	// unteren Bildschirm für Spritenutzung initialisieren:
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	oamAllocReset( &oamSub );
	oamEnable( &oamSub );

	// vorgerenderte Spritegrafiken laden:
	this->reference_key.init_vram( checkboxBitmap, this->reference_key.bg_vram );
	this->reference_key.init_vram( checkbox_activeBitmap, this->reference_key.bg_active_vram );
	
	int key_count = 0;
	int keys_per_line = 10;
	int key_x_start = 5;
	int key_x_offset = (this->keyboard_screen.res_x-key_x_start*2) / keys_per_line;
	int key_y_start = 50;
	int key_y_offset = 16+4;
	for( StringList::iterator kci = this->ui_lang.keyboard_characters.begin();
		kci != this->ui_lang.keyboard_characters.end(); kci++, key_count++ )
	{
		TextButton* new_key = new TextButton( this->reference_key );
		new_key->owns_bg_vram = false;
		new_key->text = *kci;
		if( new_key->text == "¯" || new_key->text == "´"
			/*|| new_key->text == "ˇ"*/ || new_key->text == "`" )
		{
			new_key->text_y_offset += 3;
			new_key->font_size += 2;
		}
		new_key->x = key_x_start + ( key_count % keys_per_line ) * key_x_offset;
		new_key->y = key_y_start + ( key_count / keys_per_line ) * key_y_offset;
		this->text_buttons.push_back( new_key );
		this->keys.push_back( new_key );
	}
	
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	
	this->handle_init_screens();
	
	TextButtonList init_list;
	init_list.insert( init_list.end(), this->text_buttons.begin(), this->text_buttons.end() );
	init_list.push_back( &this->reference_key );
	for( TextButtonList::iterator i=init_list.begin(); i!=init_list.end(); i++ )
	{
		(*i)->init_text_layer( this->freetype_renderer );
	}
	// Palette für 8-Bit-Buttonbeschriftungen mit speziell vorbereiteter Palette initialisieren:
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );	
}

void TouchKeyboard::render( Screen screen )
{
	this->render_prepare();
	if( screen == SCREEN_MAIN )
	{
	}
	else if( screen == SCREEN_SUB )
	{
		oamClear( &oamSub, 0, 0 );
		int oam_entry = 0;
		
		for( TextButtonList::iterator ki = this->text_buttons.begin(); ki != this->text_buttons.end(); ki++ )
		{
			(*ki)->render_to( oam_entry );
		}
		
		this->keyboard_screen.clear();
		int top = 20;
		RenderStyle render_style;
		render_style.center_x = true;
		RenderInfo render_info = this->freetype_renderer.render( 
			this->keyboard_screen, this->written_text, 
			this->freetype_renderer.han_face, 12, 0, top, &render_style );
		top = 45;
		memset( this->keyboard_screen.base_address+this->keyboard_screen.res_x*(top++)/2, 
				255, this->keyboard_screen.res_x );
		memset( this->keyboard_screen.base_address+this->keyboard_screen.res_x*(top++)/2, 
				64, this->keyboard_screen.res_x );
		
		swiWaitForVBlank();
		oamUpdate( &oamSub );
	}
}

void TouchKeyboard::run_until_exit()
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
					if( (*b_it) == &this->exit_button )
					{
						return;
					}
					else if( !this->handle_button_pressed(*b_it) )
					{
						std::string button_text = this->handle_key_pressed( (*b_it)->text );
						if( button_text == "¯" || button_text == "´" 
							|| button_text == "ˇ" || button_text == "`" )
						{
							this->modifier = button_text;
						}
						else if( button_text == "<" )
						{
							if( this->written_text.length() )
							{
								this->written_text = this->written_text.substr( 0, this->written_text.length()-1 );
								this->handle_text_changed( this->written_text );
							}
							this->modifier = "";
						}
						else
						{
							
							if( this->modifier.length() )
							{
								button_text = this->modifier + button_text;
								StringMap::iterator modifier_val_it = this->modifier_map.find( button_text );
								if( modifier_val_it != this->modifier_map.end() )
								{
									button_text = modifier_val_it->second;
								}
							}
							this->written_text += button_text;
							this->modifier = "";
							this->handle_text_changed( this->written_text );
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
