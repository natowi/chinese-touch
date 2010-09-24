#include <nds/arm9/sprite.h>

#include "touch_keyboard.h"
#include "freetype_renderer.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "error_console.h"
#include "key.h"
#include "key_active.h"


TouchKeyboard::TouchKeyboard( ButtonProviderList& button_provider_list, UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, RenderScreen& _keyboard_screen )  
	: ButtonProvider(button_provider_list, _freetype_renderer), ui_lang(_ui_lang), keyboard_screen(_keyboard_screen),
		reference_key(keyboard_screen,"",SpriteSize_32x32,0,0,button_ft.han_face,10,0,2)
{
	this->text_buttons.push_back( &this->reference_key );
	
	this->reference_key.hidden = true;
	this->reference_key.sensor_height = this->reference_key.sensor_width = 22;
	this->reference_key.text_sprite_size = SpriteSize_16x16;
	this->reference_key.get_dimensions_from_sprite_size( this->reference_key.text_sprite_size, 
														 this->reference_key.text_width, this->reference_key.text_height );

	int key_x_start = 1;
	int key_x_offset = this->reference_key.sensor_width+1;
	int key_x = key_x_start;
	int key_y_start = 50;
	int key_y_offset = this->reference_key.sensor_height+1;
	int key_y = key_y_start;
	for( StringList::iterator kci = this->ui_lang.keyboard_characters.begin();
		kci != this->ui_lang.keyboard_characters.end(); kci++ )
	{
		if( *kci=="\n" )
		{
			key_x = key_x_start;
			key_y += key_y_offset;
			continue;
		}
		if( *kci=="\t" )
		{
			key_x += key_x_offset / 2;
			continue;
		}
		TextButton* new_key = new TextButton( this->reference_key );
		new_key->hidden = false;
		new_key->owns_bg_vram = false;
		new_key->owns_text_vram = true;
		new_key->text = *kci;
		if( new_key->text == "ˇ" )
		{
			new_key->text_y_offset += 3;
		}
		else if( new_key->text == "¯" || new_key->text == "´"
			|| new_key->text == "`" )
		{
			new_key->text_y_offset += 3;
			new_key->font_size += 2;
		}
		new_key->x = key_x;
		new_key->y = key_y;
		this->text_buttons.push_back( new_key );
		this->keys.insert( new_key );
		key_x += key_x_offset;
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
	this->reference_key.init_vram( keyBitmap, this->reference_key.bg_vram );
	this->reference_key.init_vram( key_activeBitmap, this->reference_key.bg_active_vram );
	for( TextButtonSetStorage::iterator key_it = this->keys.begin(); key_it != this->keys.end(); key_it++ )
	{
		(*key_it)->bg_vram = this->reference_key.bg_vram;
		(*key_it)->bg_active_vram = this->reference_key.bg_active_vram;
	}
	
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
			if( this->modifier.length() )
			{
				this->modifier = "";
			}
			else if( this->written_chars.size() )
			{
				this->written_chars.pop_back();
				this->handle_text_changed();
			}
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
			utf8_to_utf8_char_list( (const unsigned char*)button_text.c_str(), this->written_chars );
			this->modifier = "";
			this->handle_text_changed();
		}
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return this->ButtonProvider::handle_button_pressed( text_button );
}

std::string TouchKeyboard::get_written_text()
{
	std::string result;
	for( StringList::iterator ci=this->written_chars.begin(); ci!=this->written_chars.end(); ci++ )
	{
		result += *ci;
	}
	
	return result;
}

void TouchKeyboard::set_written_text(const std::string& text)
{
	this->written_chars.clear();
	utf8_to_utf8_char_list( (const unsigned char*)text.c_str(), this->written_chars );
	this->handle_text_changed();
}
