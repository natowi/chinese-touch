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


FulltextSearch::FulltextSearch( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, Library& _library )
	: TouchKeyboard( _ui_lang, _freetype_renderer ), library(_library), current_word(words.begin()),
		left_button(&oamSub,"<",SpriteSize_32x16,0,0,freetype_renderer.latin_face,10,0,0), 
		right_button(&oamSub,">",SpriteSize_32x16,keyboard_screen.res_x-32,0,freetype_renderer.latin_face,10,2,0), 
		hanzi_tab(&oamSub,"汉字",SpriteSize_32x16,keyboard_screen.res_x/2-16-32-8,0,freetype_renderer.han_face,9),
		pinyin_tab(&oamSub,"拼音",SpriteSize_32x16,keyboard_screen.res_x/2-16,0,freetype_renderer.han_face,9,1,-1),
		latin_tab(&oamSub,"latin",SpriteSize_32x16,keyboard_screen.res_x/2+16+8,0,freetype_renderer.latin_face,7,0,1),
		rating_bar(&oamSub,"",SpriteSize_64x32,keyboard_screen.res_x/2-32,keyboard_screen.res_y-32,freetype_renderer.latin_face,7,0,0),
		rating_easy(&oamSub,"",SpriteSize_16x16,keyboard_screen.res_x/2-32,keyboard_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_medium(&oamSub,"",SpriteSize_16x16,keyboard_screen.res_x/2-16,keyboard_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_hard(&oamSub,"",SpriteSize_16x16,keyboard_screen.res_x/2,keyboard_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_impossible(&oamSub,"",SpriteSize_16x16,keyboard_screen.res_x/2+16,keyboard_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		settings_button(&oamSub,"s",SpriteSize_16x16,keyboard_screen.res_x-16,keyboard_screen.res_y-16,freetype_renderer.latin_face,10,1,1),
		down_button(&oamSub,"下",SpriteSize_16x16,44,0,freetype_renderer.han_face,9,0,0),
		search_button(&oamSub,"找",SpriteSize_32x16,keyboard_screen.res_x/2-16,keyboard_screen.res_y-64,freetype_renderer.han_face,9,1,1)
{
	this->text_buttons.push_back( &this->left_button );
	this->text_buttons.push_back( &this->right_button );
	this->text_buttons.push_back( &this->hanzi_tab );
	this->text_buttons.push_back( &this->pinyin_tab );
	this->text_buttons.push_back( &this->latin_tab );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->down_button );
	this->text_buttons.push_back( &this->search_button );
	
	this->init_screens();
}

void FulltextSearch::handle_init_screens()
{
	this->freetype_renderer.init_screen( SCREEN_MAIN, this->word_screen );
	bgShow( this->word_screen.bg_id );
	this->word_screen.clear();
	bgHide( this->word_screen.bg_id );
	
	// vorgerenderte Spritegrafiken laden:
	this->left_button.init_vram( top_left_buttonBitmap, this->left_button.bg_vram );
	this->left_button.init_vram( top_left_button_activeBitmap, this->left_button.bg_active_vram );
	this->right_button.init_vram( top_right_buttonBitmap, this->right_button.bg_vram );
	this->right_button.init_vram( top_right_button_activeBitmap, this->right_button.bg_active_vram );
	this->hanzi_tab.init_vram( top_paper_tabBitmap, this->hanzi_tab.bg_vram );
	this->hanzi_tab.init_vram( top_paper_tab_activeBitmap, this->hanzi_tab.bg_active_vram );
	this->hanzi_tab.init_vram( top_paper_tab_inactiveBitmap, this->hanzi_tab.bg_inactive_vram );
	this->rating_bar.init_vram( bottom_rating_barBitmap, this->rating_bar.bg_vram );
	this->rating_bar.bg_prio = 2; // place bar behind rating emotes
	this->rating_easy.init_vram( bottom_rating_easyBitmap, this->rating_easy.bg_vram );
	this->rating_medium.init_vram( bottom_rating_mediumBitmap, this->rating_medium.bg_vram );
	this->rating_hard.init_vram( bottom_rating_hardBitmap, this->rating_hard.bg_vram );
	this->rating_impossible.init_vram( bottom_rating_impossibleBitmap, this->rating_impossible.bg_vram );
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	this->down_button.init_vram( small_top_buttonBitmap, this->down_button.bg_vram );
	this->down_button.init_vram( small_top_button_activeBitmap, this->down_button.bg_active_vram );
	this->search_button.init_vram( menu_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( menu_button_activeBitmap, this->search_button.bg_active_vram );

	this->pinyin_tab.bg_vram = hanzi_tab.bg_vram;
	this->pinyin_tab.bg_active_vram = hanzi_tab.bg_active_vram;
	this->pinyin_tab.bg_inactive_vram = hanzi_tab.bg_inactive_vram;
	this->pinyin_tab.owns_bg_vram = false;
	this->latin_tab.bg_vram = hanzi_tab.bg_vram;
	this->latin_tab.bg_active_vram = hanzi_tab.bg_active_vram;
	this->latin_tab.bg_inactive_vram = hanzi_tab.bg_inactive_vram;
	this->latin_tab.owns_bg_vram = false;
}

void FulltextSearch::render_prepare()
{
	if( this->current_word == this->words.begin() )
		this->left_button.invisible = true;
	else this->left_button.invisible = false;
	NewWordList::iterator test_it = this->current_word;
	if( this->current_word != this->words.end() )
		test_it++;
	if( test_it == this->words.end() )
		this->right_button.invisible = true;
	else this->right_button.invisible = false;
}

bool FulltextSearch::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->left_button )
	{
		if( this->current_word!=this->words.begin() )
		{
			this->current_word--;
			NewWordRenderSettings render_settings;
			(*this->current_word)->render( this->freetype_renderer, this->word_screen, render_settings, this->library );
		}
	}
	else if( text_button == &this->right_button )
	{
		if( this->current_word != this->words.end() )
		{
			NewWordList::iterator test_it = this->current_word;
			if( ++test_it!=this->words.end() )
			{
				this->current_word++;
				NewWordRenderSettings render_settings;
				(*this->current_word)->render( this->freetype_renderer, this->word_screen, render_settings, this->library );
			}
		}
	}
	else if( text_button == &this->search_button )
	{
		this->words.clear();
		this->current_word = this->words.begin();
		// query available static book databases
		std::string sql_cond = "pattern='"+this->written_text+"'";
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
						static_db->query_static_fulltext( this->library, sql_cond, this->words, book_it->second->dictionary_lesson );
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
		this->words.sort( hanzi_min_length_sort_predicate );
		this->current_word = this->words.begin();
		if( this->words.size() )
		{
			NewWordRenderSettings render_settings;
			(*this->words.begin())->render( this->freetype_renderer, this->word_screen, render_settings, this->library );
		}
	}
	else return false;
	return true;
}
