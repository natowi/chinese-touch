#include <nds/arm9/sprite.h>
#include <fat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#include "touch_keyboard.h"
#include "freetype_renderer.h"
#include "words_db.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "error_console.h"
#include "key.h"
#include "key_active.h"
#include "small_menu_button_active.h"
#include "layout_label_frame.h"


TouchKeyboard::TouchKeyboard( ButtonProviderList& button_provider_list, Program& _program, 
							  RenderScreen& _keyboard_screen )  
	: ButtonProvider(button_provider_list, *_program.ft), db(0), program(_program), keyboard_screen(_keyboard_screen),
		reference_key(keyboard_screen,"",SpriteSize_32x32,0,0,button_ft.han_face,10,0,2),
		layout_minus(keyboard_screen,"<",SpriteSize_16x16,24,keyboard_screen.res_y-16,button_ft.latin_face,10,0,2),
		layout_label(keyboard_screen,"",SpriteSize_16x16,24+16,keyboard_screen.res_y-16,button_ft.latin_face,7,0,2),
		layout_plus(keyboard_screen,">",SpriteSize_16x16,24+32,keyboard_screen.res_y-16,button_ft.latin_face,10,0,2),
		layouts_path( program.base_dir+"/keyboards" ), current_layout( this->layouts.begin() )
{
	this->reference_key.hidden = true;
	this->reference_key.sensor_height = this->reference_key.sensor_width = 22;
	this->reference_key.text_sprite_size = SpriteSize_16x16;
	this->reference_key.get_dimensions_from_sprite_size( this->reference_key.text_sprite_size, 
														 this->reference_key.text_width, this->reference_key.text_height );
	
	DIR* layouts_dir = opendir( this->layouts_path.c_str() );
	if( !layouts_dir )
	{
		std::stringstream msg;
		msg << "failed to open: " << this->layouts_path << " (" << strerror(errno) << ")";
		throw ERROR( msg.str() );
	}
	struct dirent* layouts_entry;
	while( (layouts_entry = readdir(layouts_dir))!=NULL )
	{
		struct stat layout_stat;
		std::string layout_name = layouts_entry->d_name;
		std::string layout_path = this->layouts_path + "/" + layout_name;
		if( stat(layout_path.c_str(), &layout_stat)==-1 )
		{
			LOG( "failed to stat: " << layouts_entry->d_name );
			LOG( strerror(errno) );
			continue;
		}
		if( !S_ISDIR(layout_stat.st_mode) && layout_name.length()>3 && layout_name.substr(layout_name.length()-3)==".db" )
		{
			this->layouts.push_back( layout_path );
		}
	}
	closedir( layouts_dir );
	
	// try to load previously selected keyboard layout from settings:
	std::string previous_layout = this->program.config->get( "touch_keyboard.current_layout", "" );
	for( this->current_layout = this->layouts.begin(); 
		this->current_layout != this->layouts.end() && *this->current_layout != previous_layout;
		this->current_layout++ );
	if( this->current_layout == this->layouts.end() ) this->current_layout = this->layouts.begin();
}

TouchKeyboard::~TouchKeyboard()
{
	this->close_db();
}

void TouchKeyboard::close_db()
{
	if( this->db )
	{
		sqlite3_close( this->db );
		this->db = 0;
	}
}

void TouchKeyboard::reload_current_layout()
{
	this->free_button_vram();
	for( TextButtonSetStorage::iterator ki=this->keys.begin(); ki!=this->keys.end(); ki++ )
	{
		if( *ki )
		{
			delete *ki;
		}
	}
	this->keys.clear();
	this->text_buttons.clear();
	this->text_buttons.push_back( &this->reference_key );
	this->text_buttons.push_back( &this->layout_minus );
	this->text_buttons.push_back( &this->layout_label );
	this->text_buttons.push_back( &this->layout_plus );
	
	this->close_db();
	if( this->current_layout != this->layouts.end() )
	{
		int rc;
		int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;
		if( (rc = sqlite3_open_v2( this->current_layout->c_str(), &db, flags, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << ": " << rc << " (" << *this->current_layout << ")";
			this->close_db();
			throw ERROR( msg.str() );
		}
		std::string stmt = "select key, value from settings";
		MapList result;
		if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
		for( MapList::iterator ri=result.begin(); ri!=result.end(); ri++ )
		{
			if( (*ri)["key"]=="description" ) this->current_layout_description = (*ri)["value"];
			if( (*ri)["key"]=="language" ) this->current_layout_language = (*ri)["value"];
			if( (*ri)["key"]=="keys" ) this->current_layout_keys = (*ri)["value"];
			if( (*ri)["key"]=="dead_keys" ) this->current_layout_dead_keys = (*ri)["value"];
		}
	}
	this->layout_label.text = this->current_layout_language;
	StringList keyboard_characters;
	utf8_to_utf8_char_list( (const unsigned char*)this->current_layout_keys.c_str(), keyboard_characters );
	
	int key_x_start = 1;
	int key_x_offset = this->reference_key.sensor_width+1;
	int key_x = key_x_start;
	int key_y_start = 50;
	int key_y_offset = this->reference_key.sensor_height+1;
	int key_y = key_y_start;
	for( StringList::iterator kci = keyboard_characters.begin();
		kci != keyboard_characters.end(); kci++ )
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
}

void TouchKeyboard::init_button_vram()
{
	this->reload_current_layout();
	
	// load sprite graphics into vram:
	this->reference_key.init_vram( keyBitmap, this->reference_key.bg_vram );
	this->reference_key.init_vram( key_activeBitmap, this->reference_key.bg_active_vram );
	this->layout_minus.init_vram( small_menu_button_activeBitmap, this->layout_minus.bg_active_vram );
	this->layout_plus.bg_active_vram = this->layout_minus.bg_active_vram;
	this->layout_plus.owns_bg_vram = false;
	this->layout_label.init_vram( layout_label_frameBitmap, this->layout_label.bg_vram );
	
	for( TextButtonSetStorage::iterator key_it = this->keys.begin(); key_it != this->keys.end(); key_it++ )
	{
		(*key_it)->bg_vram = this->reference_key.bg_vram;
		(*key_it)->bg_active_vram = this->reference_key.bg_active_vram;
	}
	
	ButtonProvider::init_button_vram();
}

void TouchKeyboard::render_buttons(OamState* target_oam, int& oam_entry)
{
	this->layout_minus.hidden = this->layout_minus.disabled = this->current_layout == this->layouts.begin();
	this->layout_plus.hidden = this->layout_plus.disabled = (&*this->current_layout == &*this->layouts.rbegin());
	
	ButtonProvider::render_buttons(target_oam, oam_entry);
}


ButtonAction TouchKeyboard::handle_button_pressed( TextButton* text_button )
{
	if( this->keys.find(text_button) != this->keys.end() )
	{
		std::string button_text = this->handle_key_pressed( text_button->text );
		// FIXME: besser db-patterns abfragen!
		if( this->current_layout_dead_keys.find(button_text)!=std::string::npos )
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
				
				if( db )
				{
					int rc;
					button_text = replace_pattern( button_text, "'", "\'" );
					std::string stmt = "select result from modifiers where pattern='"+button_text+"'";
					MapList result;
					if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
					{
						std::stringstream msg;
						msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
						throw ERROR( msg.str() );
					}
					for( MapList::iterator ri=result.begin(); ri!=result.end(); ri++ )
					{
						button_text = (*ri)["result"];
						break;
					}
				}
			}
			utf8_to_utf8_char_list( (const unsigned char*)button_text.c_str(), this->written_chars );
			this->modifier = "";
			this->handle_text_changed();
		}
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	else if( text_button == &this->layout_minus )
	{
		if( this->current_layout != this->layouts.begin() )
		{
			this->current_layout--;
			this->init_button_vram();
			this->program.config->set( "touch_keyboard.current_layout", *this->current_layout );
		}
		 // WARNING: button release handler will crash because of iterator invalidation if STOP_HANDLER ist not set:
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_STOP_HANDLER;
	}
	else if( text_button == &this->layout_plus )
	{
		if( this->layouts.size() && &*this->current_layout != &*this->layouts.rbegin() )
		{
			this->current_layout++;
			this->init_button_vram();
			this->program.config->set( "touch_keyboard.current_layout", *this->current_layout );
		}
		 // WARNING: button release handler will crash because of iterator invalidation when STOP_HANDLER ist not set:
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_STOP_HANDLER;
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
