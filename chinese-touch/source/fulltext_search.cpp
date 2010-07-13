#include <nds/arm9/background.h>

#include "fulltext_search.h"
#include "error_console.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "bottom_right_button.h"
#include "bottom_right_button_active.h"
#include "settings_dialog.h"
#include "text_view.h"
#include "large_center_button.h"
#include "large_center_button_active.h"
#include <small_top_button.h>
#include <small_top_button_active.h>


FulltextSearch::FulltextSearch( Program& _program, int _recursion_depth, Lesson* _lesson )
	: Mode(_program, _recursion_depth), lesson(_lesson), word_screen(SCREEN_MAIN), keyboard_screen(SCREEN_SUB),
		touch_keyboard(button_provider_list, *_program.ui_lang, *_program.ft, keyboard_screen), 
		word_browser(button_provider_list, *_program.ft, current_words, keyboard_screen, *_program.library),
		settings_button(keyboard_screen,"s",SpriteSize_16x16,keyboard_screen.res_x-16,keyboard_screen.res_y-16,_program.ft->latin_face,10,1,1),
		search_button(keyboard_screen,"查词典",SpriteSize_64x32,keyboard_screen.res_x-74,keyboard_screen.res_y-60,_program.ft->han_face,14,0,4),
		clear_button(keyboard_screen,"c",SpriteSize_16x16,keyboard_screen.res_x-44-16,0,_program.ft->latin_face,10,1,-1)
{
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->search_button );
	this->text_buttons.push_back( &this->clear_button );
	
	// disable currently unused settings button:
	this->settings_button.hidden = this->settings_button.disabled = true;
	
	// disable word_browsers redundant search_button:
	this->word_browser.search_button.hidden = this->word_browser.search_button.disabled = true;
	
	// disable child mode buttons when recursion limit is reached:
	if( this->recursion_depth>=Mode::MAX_RECURSION_DEPTH )
	{
		this->word_browser.down_button.hidden = this->word_browser.down_button.disabled = true;
		this->word_browser.search_button.hidden = this->word_browser.search_button.disabled = true;
	}
	
	this->init_mode();
	this->init_vram();
}

void FulltextSearch::init_mode()
{
	this->program.ft->init_screen( this->word_screen );
	this->word_screen.clear();

	this->program.ft->init_screen( this->keyboard_screen );
	this->keyboard_screen.clear();
	
	this->Mode::init_mode();
}

void FulltextSearch::init_vram()
{
	Mode::init_vram();
}

void FulltextSearch::init_button_vram()
{
	// load sprite graphics into vram:
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	this->search_button.init_vram( large_center_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( large_center_button_activeBitmap, this->search_button.bg_active_vram );
	this->search_button.init_vram( large_searchBitmap, this->search_button.fg_vram );
	this->clear_button.init_vram( small_top_buttonBitmap, this->clear_button.bg_vram );
	this->clear_button.init_vram( small_top_button_activeBitmap, this->clear_button.bg_active_vram );
	
	ButtonProvider::init_button_vram();
}

void FulltextSearch::render( Screen screen )
{
	NewWord* new_word = 0;
	if( this->word_browser.current_word != this->word_browser.words.end() )
	{
		new_word = *this->word_browser.current_word;
	}
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		if( this->word_browser.words.size() && this->word_browser.current_word != this->word_browser.words.end() )
		{
			(*this->word_browser.current_word)->render( this->program, this->word_screen, this->word_browser );
		}
	}
	else if( screen == SCREEN_SUB )
	{
		// make add button available for words from other books or dictionaries, to allow them to be 
		// loosely associated with the current lesson:
		this->word_browser.add_button.hidden = 
			this->word_browser.add_button.disabled = 
				!( this->lesson && new_word 
					&& ( (new_word->lesson && new_word->lesson->number==0) /*dictionaries*/
							|| (new_word->lesson && new_word->lesson->book!=this->lesson->book) /*other books*/
							|| !new_word->lesson /*lost words*/ ) );
		
		this->clear_button.y = touch_keyboard.written_text.length() ? 0 : -12;
		int top = 20;
		if( this->prev_rendered_text != this->touch_keyboard.written_text )
		{
			this->keyboard_screen.clear();
			RenderStyle render_style;
			render_style.center_x = true;
			RenderInfo render_info = this->program.ft->render( 
				this->keyboard_screen, this->touch_keyboard.written_text, 
				this->program.ft->han_face, 12, 0, top, &render_style );
			this->prev_rendered_text = this->touch_keyboard.written_text;
		}
		top = 45;
		memset( this->keyboard_screen.base_address+this->keyboard_screen.res_x*(top++)/2, 
				255, this->keyboard_screen.res_x );
		memset( this->keyboard_screen.base_address+this->keyboard_screen.res_x*(top++)/2, 
				64, this->keyboard_screen.res_x );
	}
	
	Mode::render( screen );
}

void extract_words( const std::string& text, StringList& patterns )
{
	size_t start = 0;
	size_t pos = 0;
	do
	{
		pos = text.find_first_of( ' ', start );
		int len = (pos != std::string::npos) ? pos-start : pos;
		patterns.push_back( text.substr(start, len) );
		start = pos+1;
	} while( pos!=std::string::npos );
}

ButtonAction FulltextSearch::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->word_browser.down_button
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		TextView::show_word_as_text( this->program, *this->word_browser.current_word, this->lesson, this->recursion_depth );
		this->prev_rendered_text=""; // force rerendering of current search text
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->search_button )
	{
		this->word_browser.words.clear();
		this->word_browser.current_word = this->word_browser.words.begin();
		// query available static book databases
		StringList patterns;
		extract_words( this->touch_keyboard.written_text, patterns );
		for( Library::iterator book_it = this->program.library->begin(); book_it != this->program.library->end(); book_it++ )
		{
			if( book_it->second 
				&& book_it->second->dictionary_lesson
				&& book_it->second->static_words_db_path.length() )
			{
				WordsDB* static_db = new WordsDB();
				try
				{
					static_db->open( book_it->second->static_words_db_path );
					try
					{
						static_db->query_static_fulltext( *this->program.library, patterns, this->word_browser.words, book_it->second->dictionary_lesson );
					}
					catch( Error& e )
					{
						WARN( e.what() );
					}
					static_db->close();
				}
				catch( Error& e )
				{
					WARN( e.what() );
				}
				delete static_db;
			}
		}
		this->word_browser.words.sort( hanzi_min_length_sort_predicate );
		this->word_browser.current_word = this->word_browser.words.begin();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.add_button
		&& this->word_browser.current_word!=this->word_browser.words.end()
		&& this->lesson )
	{
		NewWord* new_word = *this->word_browser.current_word;
		// loosely associate word with current lesson:
		new_word->lesson = this->lesson;
		new_word->id = new_word->file_id = new_word->file_offset = 0;
		// HACK: set duplicate_id to an unusual high value, to prevent overwriting duplicate words from *.dict-files
		// FIXME: this might overwrite previously associated duplicate words
		new_word->duplicate_id = 1000;
		this->program.words_db->add_or_write_word( *new_word );
		this->program.words_db->read_word( *new_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->clear_button )
	{
		this->touch_keyboard.written_text = "";
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}
