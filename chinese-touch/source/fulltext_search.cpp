#include <nds/arm9/background.h>

#include "fulltext_search.h"
#include "error_console.h"
#include "top_left_button.h"
#include "top_left_button_active.h"
#include "top_right_button.h"
#include "top_right_button_active.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "bottom_right_button.h"
#include "bottom_right_button_active.h"
#include "top_paper_tab.h"
#include "top_paper_tab_active.h"
#include "top_paper_tab_inactive.h"
#include "bg_dragon.h"
#include "bottom_rating_bar.h"
#include "bottom_rating_easy.h"
#include "bottom_rating_medium.h"
#include "bottom_rating_hard.h"
#include "bottom_rating_impossible.h"
#include "settings_dialog.h"
#include "right_center_button.h"
#include "right_center_button_active.h"
#include "small_top_button.h"
#include "small_top_button_active.h"
#include "menu_button.h"
#include "menu_button_active.h"
#include "text_view.h"


FulltextSearch::FulltextSearch( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, Library& _library )
	: Mode(_freetype_renderer), library(_library), 
		touch_keyboard(button_provider_list, _ui_lang, _freetype_renderer, keyboard_screen), 
		word_browser(button_provider_list, _freetype_renderer, current_words, keyboard_screen, _library),
		settings_button(&oamSub,"s",SpriteSize_16x16,keyboard_screen.res_x-16,keyboard_screen.res_y-16,_freetype_renderer.latin_face,10,1,1),
		search_button(&oamSub,"找",SpriteSize_32x16,keyboard_screen.res_x/2-16,keyboard_screen.res_y-64,_freetype_renderer.han_face,9,1,1)
{
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->search_button );
	
	this->init_mode();
	this->init_vram();
}

void FulltextSearch::init_mode()
{
	this->mode_ft.init_screen( SCREEN_MAIN, this->word_screen );
	bgShow( this->word_screen.bg_id );
	this->word_screen.clear();
	bgHide( this->word_screen.bg_id );

	this->mode_ft.init_screen( SCREEN_SUB, this->keyboard_screen );
	this->keyboard_screen.clear();
	
	this->Mode::init_mode();
}

void FulltextSearch::init_button_vram()
{
	// load sprite graphics into vram:
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	this->search_button.init_vram( menu_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( menu_button_activeBitmap, this->search_button.bg_active_vram );
	
	ButtonProvider::init_button_vram();
}

void FulltextSearch::render( Screen screen )
{
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		if( this->word_browser.words.size() && this->word_browser.current_word != this->word_browser.words.end() )
		{
			(*this->word_browser.current_word)->render( this->mode_ft, this->word_screen, this->word_browser, this->library );
		}
	}
	else if( screen == SCREEN_SUB )
	{
		this->keyboard_screen.clear();
		int top = 20;
		RenderStyle render_style;
		render_style.center_x = true;
		RenderInfo render_info = this->mode_ft.render( 
			this->keyboard_screen, this->touch_keyboard.written_text, 
			this->mode_ft.han_face, 12, 0, top, &render_style );
		top = 45;
		memset( this->keyboard_screen.base_address+this->keyboard_screen.res_x*(top++)/2, 
				255, this->keyboard_screen.res_x );
		memset( this->keyboard_screen.base_address+this->keyboard_screen.res_x*(top++)/2, 
				64, this->keyboard_screen.res_x );
	}
	
	Mode::render( screen );
}

ButtonAction FulltextSearch::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->word_browser.down_button
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		TextView::show_word_as_text( this->mode_ft, this->library, *this->word_browser.current_word, 0 );
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_HANDLED;
	}
	if( text_button == &this->word_browser.left_button
		|| text_button == &this->word_browser.right_button
		|| text_button == &this->word_browser.foreign_word_tab
		|| text_button == &this->word_browser.pronunciation_tab
		|| text_button == &this->word_browser.translation_tab )
	{
		this->render( SCREEN_MAIN );
		return BUTTON_ACTION_HANDLED;
	}
	if( text_button == &this->search_button )
	{
		this->word_browser.words.clear();
		this->word_browser.current_word = this->word_browser.words.begin();
		// query available static book databases
		std::string sql_cond = "pattern='"+this->touch_keyboard.written_text+"'";
		for( Library::iterator book_it = this->library.begin(); book_it != this->library.end(); book_it++ )
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
						static_db->query_static_fulltext( this->library, sql_cond, this->word_browser.words, book_it->second->dictionary_lesson );
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
		this->render( SCREEN_MAIN );
		return BUTTON_ACTION_HANDLED;
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}
