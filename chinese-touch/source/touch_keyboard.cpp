#include <nds/arm9/sprite.h>

#include "touch_keyboard.h"
#include "freetype_renderer.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "checkbox.h"
#include "checkbox_active.h"
#include "error_console.h"


TouchKeyboard::TouchKeyboard( ButtonProviderList& button_provider_list, UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, RenderScreen& _keyboard_screen )  
	: ButtonProvider(button_provider_list, _freetype_renderer), ui_lang(_ui_lang), keyboard_screen(_keyboard_screen),
		reference_key(&oamSub,"",SpriteSize_16x16,0,0,button_ft.han_face,9,0,1),
		exit_button(&oamSub,"x",SpriteSize_16x16,0,keyboard_screen.res_y-16,button_ft.latin_face,10,-1,1)
{
	this->text_buttons.push_back( &this->reference_key );
	this->text_buttons.push_back( &this->exit_button );
	
	this->reference_key.hidden = true;

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
		new_key->hidden = false;
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
		this->keys.insert( new_key );
	}
	
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

void TouchKeyboard::init_button_vram()
{
	// load sprite graphics into vram:
	this->reference_key.init_vram( checkboxBitmap, this->reference_key.bg_vram );
	this->reference_key.init_vram( checkbox_activeBitmap, this->reference_key.bg_active_vram );
	for( TextButtonSetStorage::iterator key_it = this->keys.begin(); key_it != this->keys.end(); key_it++ )
	{
		(*key_it)->bg_vram = this->reference_key.bg_vram;
		(*key_it)->bg_active_vram = this->reference_key.bg_active_vram;
	}
	
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	
	ButtonProvider::init_button_vram();
}

ButtonAction TouchKeyboard::handle_button_pressed( TextButton* text_button )
{
	if( this->keys.find(text_button) != this->keys.end() )
	{
		std::string button_text = this->handle_key_pressed( text_button->text );
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
		return BUTTON_ACTION_PRESSED;
	}
	else if( text_button == &this->exit_button )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	
	return this->ButtonProvider::handle_button_pressed( text_button );
}
