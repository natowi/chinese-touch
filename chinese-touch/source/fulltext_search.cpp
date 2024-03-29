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
#include "key.h"
#include "key_active.h"
#include "settings-icon.h"


FulltextSearch::FulltextSearch( Program& _program, int _recursion_depth, Lesson* _lesson )
	: Mode(_program, _recursion_depth), lesson(_lesson), word_screen(SCREEN_MAIN), keyboard_screen(SCREEN_SUB),
		touch_keyboard(button_provider_list, _program, keyboard_screen),
		word_browser(button_provider_list, *_program.ft, current_words, keyboard_screen, *_program.library),
		settings_button(keyboard_screen,"",SpriteSize_16x16,keyboard_screen.res_x-16,keyboard_screen.res_y-16,_program.ft->latin_face,10,1,1),
		search_button(keyboard_screen,"查词典",SpriteSize_64x32,keyboard_screen.res_x-67,keyboard_screen.res_y-50,_program.ft->han_face,14,0,4),
		clear_button(keyboard_screen,"clr",SpriteSize_32x32,0 /*dynamic*/,16+3,_program.ft->latin_face,8,-6,4),
		search_done(false)
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
		this->word_browser.as_text_tab.hidden = this->word_browser.as_text_tab.disabled = true;
		this->word_browser.search_button.hidden = this->word_browser.search_button.disabled = true;
		this->word_browser.stroke_order_tab.hidden = this->word_browser.stroke_order_tab.disabled = true;
		this->word_browser.components_tab.hidden = this->word_browser.components_tab.disabled = true;
	}
	
	this->init_mode();
	this->init_vram();
}

void FulltextSearch::init_mode()
{
	this->program.ft->init_screen( this->word_screen );
	this->word_screen.clear();
	this->word_screen.clear_bg();

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
	this->settings_button.init_vram( settings_iconBitmap, this->settings_button.fg_vram );
	this->search_button.init_vram( large_center_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( large_center_button_activeBitmap, this->search_button.bg_active_vram );
	this->search_button.init_vram( large_searchBitmap, this->search_button.fg_vram );
	this->clear_button.init_vram( keyBitmap, this->clear_button.bg_vram );
	this->clear_button.init_vram( key_activeBitmap, this->clear_button.bg_active_vram );
	
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
		if( new_word )
		{
			// FIXME: code duplicate in multiple Mode::render
			if( (this->word_browser.render_stroke_order || this->word_browser.render_components)
				&& this->word_browser.current_char!=this->word_browser.current_char_list.end() )
			{
				this->word_browser.highlight_char = *this->word_browser.current_char;
			}
			else
			{
				this->word_browser.highlight_char.init();
			}
			new_word->render( this->program, this->word_screen, this->word_browser );
		}
		else
		{
			std::string message;
			if( !this->search_done )
			{
				message = "Welcome to Dictionary Search!";
				message += "\n ";
				message += "\n- Type a search term and hit the search \n  button";
			}
			else
			{
				message = "No entry found :(";
				message += "\n ";
				message += "\n- Download dictionaries from:";
				message += "\n  http://code.google.com/p/chinese-touch";
			}
			RenderStyle style;
			style.center_x = style.center_y = false;
			this->program.ft->render( this->word_screen, message, this->program.ft->latin_face, 8, 5, 60, &style );
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
		
		this->clear_button.x = touch_keyboard.written_chars.size() ? keyboard_screen.res_x-18 : keyboard_screen.res_x-4;
		int top = 20;
		std::string written_text = this->touch_keyboard.get_written_text();
		if( this->prev_rendered_text != written_text )
		{
			this->prev_rendered_text = written_text;
			this->keyboard_screen.clear();
			RenderStyle render_style;
			render_style.center_x = true;
			RenderInfo render_info = this->program.ft->render( 
				this->keyboard_screen, written_text, 
				this->program.ft->han_face, 12, 0, top, &render_style );
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
	if( text_button == &this->word_browser.as_text_tab
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		if( this->word_browser.render_components )
		{
			// FIXME: code duplicate in multiple Mode::handle_button_pressed
			NewWord *word = new NewWord( (*this->word_browser.current_word)->hanzi.substr(
				this->word_browser.highlight_char.source_offset, this->word_browser.highlight_char.source_length), "", 0 );
			word->definitions["components"] = new Definition();
			word->definitions["components"]->comment = this->word_browser.char_components_cache + "\n" + this->word_browser.char_component_usage_cache;
			TextView::show_word_as_text( this->program, word, 0, this->recursion_depth );
			delete word;
		}
		else
		{
			TextView::show_word_as_text( this->program, *this->word_browser.current_word, this->lesson, this->recursion_depth );
		}
		this->prev_rendered_text=""; // force rerendering of current search text
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->search_button )
	{
		this->search_done = true;
		this->word_browser.words.clear();
		this->word_browser.current_word = this->word_browser.words.begin();
		// query available static book databases
		StringList patterns;
		extract_words( this->touch_keyboard.get_written_text(), patterns );
		this->program.words_db->query_static_fulltext( *this->program.library, patterns, this->word_browser.words, 0 );
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
		// FIXME: word list initialization is duplicated in TextView::handle_touch_end!
		this->word_browser.current_word = this->word_browser.words.begin();
		// initialize component display, if needed:
		// (stroke order display is not a problem, because it always triggeres a child mode for writing)
		if( this->word_browser.render_components )
		{
			this->word_browser.render_components = false;
			this->word_browser.toggle_components();
		}
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
		this->touch_keyboard.written_chars.clear();
		this->touch_keyboard.handle_text_changed();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.stroke_order_tab
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		// FIXME: code duplicate in TextView::handle_button_pressed
		this->free_vram();
		NewWordList *single_word_list = new NewWordList();
		single_word_list->push_back( *this->word_browser.current_word );
		NewWordsViewer *word_viewer = new NewWordsViewer( this->program, this->recursion_depth, *single_word_list, 
														  false /*no position saving*/, false /*no shuffle*/, 
														  false /*don't show settings*/ );
		word_viewer->word_browser.toggle_stroke_order();
		// explicitly replace exit button with dog-ear to show user, that she is in a sub mode
		word_viewer->word_browser.exit_button.hidden = word_viewer->word_browser.exit_button.disabled = true;
		word_viewer->word_browser.dogear.hidden = word_viewer->word_browser.dogear.disabled = false;
		word_viewer->run_until_exit();
		delete word_viewer;
		single_word_list->erase( single_word_list->begin(), single_word_list->end() );
		delete single_word_list;
		this->prev_rendered_text=""; // force rerendering of current search text
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.components_tab )
	{
		this->word_browser.toggle_components();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}
