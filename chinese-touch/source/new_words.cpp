#include <iostream>
#include <cmath>

#include <nds.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>
#include <time.h>

#include "new_words.h"
#include "config.h"
#include "error_console.h"
#include "sprite_helper.h"
#include "words_db.h"
#include "text_view.h"

#include "greys256.h"
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


void NewWord::render( FreetypeRenderer& ft, RenderScreen& render_screen, WordListBrowser& render_settings, Library& library )
{
	// try to read the corresponding entry from the database:
	if( this->lesson && this->lesson->book && this->lesson->book->library )
	{
		bool in_user_db = this->lesson->book->library->words_db.read_word(*this);
		// update words access time:
		this->atime = time(0);
		// write updated word to the users database, but prevent unrated words from static 
		// dictionaries from being copied to the user db automatically:
		if( !this->from_static_db || in_user_db )
		{
			this->lesson->book->library->words_db.add_or_write_word( *this );
		}
	}
	render_screen.clear();

	// render hanzi centered
	RenderStyle render_style;
	render_style.center_x = true;
	int top = 5;
	int size = 32;
	if( render_settings.render_foreign_word )
	{
		RenderInfo rect = ft.render( render_screen, this->hanzi, ft.han_face, size, 0, top, &render_style );
		top += rect.height;
	}
	else
	{
		top += size*1.2;
	}
	top += 10;

	// render pinyin centered
	size = 14;
	if( render_settings.render_pronuciation )
	{
		RenderInfo rect = ft.render( render_screen, this->pinyin, ft.han_face, size, 0, top, &render_style );
		top += rect.height;
	}
	else
	{
		top += size*1.2;
	}
	top += 10;

	std::string lang = "de";

	if( render_settings.render_translation && this->definitions.count(lang) )
	{
		if( this->definitions[lang]->word_type.length() )
		{
			// render word type
			size = 7;
			RenderInfo rect = ft.render( render_screen, this->definitions[lang]->word_type, ft.latin_face, size, 0, top, &render_style );
			top += rect.height+5;
		}
		unsigned int char_limit = 200;
		if( this->definitions[lang]->translation.length() )
		{
			// render first n characters of translation
			size = 9;
			std::string text = this->definitions[lang]->translation.substr(0,char_limit);
			if( text.length()==char_limit ) text += "...";
			char_limit -= text.length();
			RenderInfo rect = ft.render( render_screen, text, ft.latin_face, size, 0, top, &render_style );
			top += rect.height+10;
		}
		if( this->definitions[lang]->comment.length() )
		{
			// render first n characters of comment
			size = 8;
			std::string text = this->definitions[lang]->comment.substr(0,char_limit);
			if( text.length()==char_limit ) text += "...";
			RenderInfo rect = ft.render( render_screen, text, ft.latin_face, size, 0, top, &render_style );
			top += rect.height;
		}
	}
	
	/* render book title and lesson number for information, but only if there is enough space left and no
		part of the word entry is hidden, as we do not want to disturb memorization by giving hints */
	int booK_lesson_height = 12;
	int book_lesson_top = render_screen.res_y-booK_lesson_height;
	if( render_settings.render_foreign_word 
		&& render_settings.render_pronuciation 
		&& render_settings.render_translation 
		&& this->lesson && this->lesson->book && (top <= book_lesson_top) )
	{
		size = 6;
		std::stringstream text_stream;
		text_stream << "(" << this->lesson->book->title << ": " << this->lesson->number << ")";
		ft.render( render_screen, text_stream.str(), ft.latin_face, size, 0, book_lesson_top );
	}
}


WordListBrowser::WordListBrowser( ButtonProviderList& provider_list, 
								  FreetypeRenderer& _freetype_renderer, 
								  NewWordList& _words, 
								  RenderScreen& _button_screen,
								  Library& _library ) 
	: 	ButtonProvider(provider_list, _freetype_renderer), 
		words(_words), current_word(words.begin()),
		button_screen(_button_screen), library(_library),
		render_foreign_word(true), render_pronuciation(true), render_translation(true),
		init_render_foreign_word(true), init_render_pronuciation(true), init_render_translation(true),
		restore_on_switch(true), clear_on_switch(true),
		left_button(&oamSub,"<",SpriteSize_32x16,0,0,button_ft.latin_face,10,0,0), 
		right_button(&oamSub,">",SpriteSize_32x16,button_screen.res_x-32,0,button_ft.latin_face,10,2,0), 
		foreign_word_tab(&oamSub,"汉字",SpriteSize_32x16,button_screen.res_x/2-16-32-8,0,button_ft.han_face,9),
		pronunciation_tab(&oamSub,"拼音",SpriteSize_32x16,button_screen.res_x/2-16,0,button_ft.han_face,9,1,-1),
		translation_tab(&oamSub,"latin",SpriteSize_32x16,button_screen.res_x/2+16+8,0,button_ft.latin_face,7,0,1),
		rating_bar(&oamSub,"",SpriteSize_64x32,button_screen.res_x/2-32,button_screen.res_y-32,button_ft.latin_face,7,0,0),
		rating_easy(&oamSub,"",SpriteSize_16x16,button_screen.res_x/2-32,button_screen.res_y-16,button_ft.latin_face,7,0,0),
		rating_medium(&oamSub,"",SpriteSize_16x16,button_screen.res_x/2-16,button_screen.res_y-16,button_ft.latin_face,7,0,0),
		rating_hard(&oamSub,"",SpriteSize_16x16,button_screen.res_x/2,button_screen.res_y-16,button_ft.latin_face,7,0,0),
		rating_impossible(&oamSub,"",SpriteSize_16x16,button_screen.res_x/2+16,button_screen.res_y-16,button_ft.latin_face,7,0,0),
		down_button(&oamSub,"下",SpriteSize_16x16,44,0,button_ft.han_face,9,0,0)
{
	this->text_buttons.push_back( &this->left_button );
	this->text_buttons.push_back( &this->right_button );
	this->text_buttons.push_back( &this->foreign_word_tab );
	this->text_buttons.push_back( &this->pronunciation_tab );
	this->text_buttons.push_back( &this->translation_tab );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );
	this->text_buttons.push_back( &this->down_button );
}

void WordListBrowser::init_button_vram()
{
	// vorgerenderte Spritegrafiken laden:
	this->left_button.init_vram( top_left_buttonBitmap, this->left_button.bg_vram );
	this->left_button.init_vram( top_left_button_activeBitmap, this->left_button.bg_active_vram );
	this->right_button.init_vram( top_right_buttonBitmap, this->right_button.bg_vram );
	this->right_button.init_vram( top_right_button_activeBitmap, this->right_button.bg_active_vram );
	this->foreign_word_tab.init_vram( top_paper_tabBitmap, this->foreign_word_tab.bg_vram );
	this->foreign_word_tab.init_vram( top_paper_tab_activeBitmap, this->foreign_word_tab.bg_active_vram );
	this->foreign_word_tab.init_vram( top_paper_tab_inactiveBitmap, this->foreign_word_tab.bg_inactive_vram );
	this->rating_bar.init_vram( bottom_rating_barBitmap, this->rating_bar.bg_vram );
	this->rating_bar.bg_prio = 2; // place bar behind rating emotes
	this->rating_easy.init_vram( bottom_rating_easyBitmap, this->rating_easy.bg_vram );
	this->rating_medium.init_vram( bottom_rating_mediumBitmap, this->rating_medium.bg_vram );
	this->rating_hard.init_vram( bottom_rating_hardBitmap, this->rating_hard.bg_vram );
	this->rating_impossible.init_vram( bottom_rating_impossibleBitmap, this->rating_impossible.bg_vram );
	this->down_button.init_vram( small_top_buttonBitmap, this->down_button.bg_vram );
	this->down_button.init_vram( small_top_button_activeBitmap, this->down_button.bg_active_vram );

	this->pronunciation_tab.bg_vram = this->foreign_word_tab.bg_vram;
	this->pronunciation_tab.bg_active_vram = this->foreign_word_tab.bg_active_vram;
	this->pronunciation_tab.bg_inactive_vram = this->foreign_word_tab.bg_inactive_vram;
	this->pronunciation_tab.owns_bg_vram = false;
	this->translation_tab.bg_vram = this->foreign_word_tab.bg_vram;
	this->translation_tab.bg_active_vram = this->foreign_word_tab.bg_active_vram;
	this->translation_tab.bg_inactive_vram = this->foreign_word_tab.bg_inactive_vram;
	this->translation_tab.owns_bg_vram = false;
	
	ButtonProvider::init_button_vram();
}

void WordListBrowser::render_buttons( OamState* oam_state, int& oam_entry )
{
	// hide left button when at the beginning of the word list:
	if( this->current_word == this->words.begin() ) this->left_button.hidden = true;
	else this->left_button.hidden = false;
	// hide right button when at the end of the word list:
	NewWordList::iterator test_it = this->current_word;
	if( this->current_word != this->words.end() ) test_it++;
	if( test_it == this->words.end() ) this->right_button.hidden = true;
	else this->right_button.hidden = false;
	
	NewWord* word = 0;
	if( this->current_word != this->words.end() ) word = *this->current_word;
	
	// retract several edge sensors depending on the current list/display state:
	// FIXME: it may be to hard for the user to tap on half retracted display setting tabs
	this->foreign_word_tab.y = word ? ( this->render_foreign_word ? 0 : -8 ) : -12;
	this->pronunciation_tab.y = word ? ( this->render_pronuciation ? 0 : -8 ) : -12;
	this->translation_tab.y = word ? ( this->render_translation ? 0 : -8 ) : -12;
	this->rating_bar.y = word ? this->button_screen.res_y-32 : this->button_screen.res_y-32+12;
	this->rating_easy.y = word ? this->button_screen.res_y-16 : this->button_screen.res_y-16+12;
	this->rating_medium.y = word ? this->button_screen.res_y-16 : this->button_screen.res_y-16+12;
	this->rating_hard.y = word ? this->button_screen.res_y-16 : this->button_screen.res_y-16+12;
	this->rating_impossible.y = word ? this->button_screen.res_y-16 : this->button_screen.res_y-16+12;
	
	// hide all but currently active rating:
	if( word )
	{
		this->library.words_db.read_word( *word );
		this->rating_easy.hidden = !(this->rating_easy.active || word->rating==RATING_EASY);
		this->rating_medium.hidden = !(this->rating_medium.active || word->rating==RATING_MEDIUM);
		this->rating_hard.hidden = !(this->rating_hard.active || word->rating==RATING_HARD);
		this->rating_impossible.hidden = !(this->rating_impossible.active || word->rating==RATING_IMPOSSIBLE);
	}
	
	// let ButtonProvider do the actual rendering
	ButtonProvider::render_buttons(oam_state, oam_entry);
}

ButtonAction WordListBrowser::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->left_button )
	{
		this->switch_backwards();
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->right_button )
	{
		this->switch_forward();
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->foreign_word_tab )
	{
		this->toggle_foreign_word();
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->pronunciation_tab )
	{
		this->toggle_pronunciation();
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->translation_tab )
	{
		this->toggle_translation();
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->rating_easy 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_EASY;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->rating_medium 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_MEDIUM;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->rating_hard 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_HARD;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_HANDLED;
	}
	else if( text_button == &this->rating_impossible 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_IMPOSSIBLE;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_HANDLED;
	}
	
	return ButtonProvider::handle_button_pressed(text_button);
}

void WordListBrowser::switch_forward()
{
	if( this->current_word != this->words.end() )
	{
		NewWordList::iterator test_it = this->current_word;
		if( ++test_it != this->words.end() )
			this->current_word++;
	}
	this->restore_init_settings_if_needed();
}

void WordListBrowser::switch_backwards()
{
	if( this->current_word != this->words.begin() )
		this->current_word--;
	this->restore_init_settings_if_needed();
}







int NewWordsViewer::BUTTON_ACTIVATION_DRAW_LIMIT = 5;

NewWordsViewer::NewWordsViewer( FreetypeRenderer& _freetype_renderer, NewWordList& _words, Library& _library, Config* _config )
	: Mode(_freetype_renderer), word_browser(button_provider_list, _freetype_renderer, _words, drawing_screen, _library),
		drawing_pad(drawing_screen), library(_library), config(_config),
		exit_button(&oamSub,"x",SpriteSize_16x16,0,drawing_screen.res_y-16,_freetype_renderer.latin_face,10,-1,1),
		clear_button(&oamSub,"C\nL\nE\nA\nR",SpriteSize_32x64,drawing_screen.res_x-16,drawing_screen.res_y/2-32,_freetype_renderer.latin_face,9,-7,3),
		settings_button(&oamSub,"s",SpriteSize_16x16,drawing_screen.res_x-16,drawing_screen.res_y-16,_freetype_renderer.latin_face,10,1,1),
		buttons_locked(false)
{
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new BooleanSetting("0_show_foreign_word","Show Foreign Word",this->word_browser.init_render_foreign_word) );
	this->settings.add_setting( new BooleanSetting("1_show_pronunciation","Show Pronunciation",this->word_browser.init_render_pronuciation) );
	this->settings.add_setting( new BooleanSetting("2_show_translation","Show Translation",this->word_browser.init_render_translation) );
	this->settings.add_setting( new BooleanSetting("3_restore_state","Restore Above Settings on Switch",this->word_browser.restore_on_switch) );
	this->settings.add_setting( new BooleanSetting("4_clear_screen","Clear Writing Screen on Switch",this->word_browser.clear_on_switch) );
	
	this->text_buttons.push_back( &this->exit_button );
	this->text_buttons.push_back( &this->clear_button );
	this->text_buttons.push_back( &this->settings_button );
	
	// initialize word list on stored word index (only used when calling new words from a lesson):
	if( this->config ) 
	{
		int word_id = this->config->get_current_word_id();
		for( this->word_browser.current_word = this->word_browser.words.begin(); 
			this->word_browser.current_word != this->word_browser.words.end(); 
			this->word_browser.current_word++ )
		{
			if( (*this->word_browser.current_word)->id == word_id ) break;
		}
		if( this->word_browser.current_word == this->word_browser.words.end() ) 
			this->word_browser.current_word = this->word_browser.words.begin();
	}
	
	this->show_settings();
	// show_settings() calls init_vram(), so no need to do this again
}

void NewWordsViewer::init_mode()
{
	this->Mode::button_ft.init_screen( SCREEN_MAIN, this->word_screen );
	this->word_screen.clear();
	bgHide( this->word_screen.bg_id );
	
	this->Mode::button_ft.init_screen( SCREEN_SUB, this->drawing_screen );
	this->drawing_screen.clear();

	Mode::init_mode();
}

void NewWordsViewer::init_vram()
{
	this->render( SCREEN_MAIN );
	
	Mode::init_vram();
}

void NewWordsViewer::init_button_vram()
{
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	this->clear_button.init_vram( right_center_buttonBitmap, this->clear_button.bg_vram );
	this->clear_button.init_vram( right_center_button_activeBitmap, this->clear_button.bg_active_vram );
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	
	ButtonProvider::init_button_vram();
}

void NewWordsViewer::show_settings()
{
	this->free_vram();
	SettingsDialog settings_dialog( this->Mode::button_ft, this->settings, "Word List Settings" );
	settings_dialog.run_until_exit();
	this->word_browser.restore_init_settings();
	this->init_mode();
	this->init_vram();
}

void NewWordsViewer::render( Screen screen )
{
	NewWord* new_word = 0;
	if( this->word_browser.current_word!=this->word_browser.words.end() )
	{
		new_word = *this->word_browser.current_word;
	}
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		if( new_word )
		{
			if( this->config ) this->config->save_position( new_word );
			new_word->render( this->Mode::button_ft, this->word_screen, this->word_browser, this->library );
		}
	}
	
	Mode::render( screen );
}

ButtonAction NewWordsViewer::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->clear_button )
	{
		this->drawing_pad.clear();
		return BUTTON_ACTION_HANDLED;
	}
	if( text_button == &this->exit_button )
	{
		if( this->config && (this->word_browser.current_word != this->word_browser.words.end()) )
		{
			this->config->save_position( *this->word_browser.current_word );
		}
		return BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->settings_button )
	{
		this->show_settings();
		return BUTTON_ACTION_HANDLED;
	}
	if( text_button == &this->word_browser.down_button
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		// FIXME: TextView::show_word_as_text( this->Mode::freetype_renderer, this->library, this->word_browser.current_word, 0 );
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
		if( this->word_browser.clear_on_switch
			&& (text_button == &this->word_browser.left_button
				|| text_button == &this->word_browser.right_button) )
		{
			this->drawing_pad.clear();
		}
		this->render( SCREEN_MAIN );
		return BUTTON_ACTION_HANDLED;
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}

ButtonAction NewWordsViewer::handle_touch_begin( touchPosition touch )
{
	this->old_touch = touch;
	this->old_distance = 0;
	if( !this->current_active_button )
	{
		this->drawing_pad.draw_point( touch.px, touch.py );
		return BUTTON_ACTION_HANDLED;
	}
	
	return Mode::handle_touch_begin( touch );
}

ButtonAction NewWordsViewer::handle_touch_drag( touchPosition touch )
{
	int x_diff = touch.px - this->old_touch.px;
	int y_diff = touch.py - this->old_touch.py;
	int distance = (int)std::sqrt( std::pow(x_diff,2) + std::pow(y_diff,2) );
	if( !this->current_active_button 
		&& (distance && ((this->old_distance && (distance <= this->old_distance*DrawingPad::MAX_ACCELERATION_FACTOR)) 
			|| (distance <= DrawingPad::MAX_ACCELERATION_FACTOR))) );
	{
		this->old_distance = distance;
		//pixels_drawn += distance;
		this->drawing_pad.draw_line( touch.px, touch.py, this->old_touch.px, this->old_touch.py );
		this->old_touch = touch;
		return BUTTON_ACTION_HANDLED;
	}
	this->old_touch = touch;
	
	return Mode::handle_touch_drag( touch );
}

ButtonAction NewWordsViewer::handle_touch_end( touchPosition touch )
{
	return this->handle_touch_drag( touch );
}

ButtonAction NewWordsViewer::handle_console_button_pressed( int pressed )
{
	// delegate console L and R buttons to touch screen left/right button handlers:
	if( pressed & KEY_L )
	{
		return this->word_browser.handle_button_pressed( &this->word_browser.left_button );
	}
	else if( pressed & KEY_R )
	{
		return this->word_browser.handle_button_pressed( &this->word_browser.right_button );
	}
	
	return Mode::handle_console_button_pressed(pressed);
}
