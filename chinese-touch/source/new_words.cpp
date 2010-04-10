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


void NewWord::render( FreetypeRenderer& ft, RenderScreen& render_screen, NewWordRenderSettings& render_settings, Library& library )
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


int NewWordsViewer::BUTTON_ACTIVATION_DRAW_LIMIT = 5;

NewWordsViewer::NewWordsViewer( FreetypeRenderer& _freetype_renderer, NewWordList& _words, Library& _library, Config* _config )
	: freetype_renderer(_freetype_renderer), drawing_pad(drawing_screen), words(_words), 
		current_word(words.begin()), library(_library), config(_config), 
		left_button(&oamSub,"<",SpriteSize_32x16,0,0,freetype_renderer.latin_face,10,0,0), 
		right_button(&oamSub,">",SpriteSize_32x16,drawing_screen.res_x-32,0,freetype_renderer.latin_face,10,2,0), 
		exit_button(&oamSub,"x",SpriteSize_16x16,0,drawing_screen.res_y-16,freetype_renderer.latin_face,10,-1,1),
		clear_button(&oamSub,"C\nL\nE\nA\nR",SpriteSize_32x64,drawing_screen.res_x-16,drawing_screen.res_y/2-32,freetype_renderer.latin_face,9,-7,3),
		hanzi_tab(&oamSub,"汉字",SpriteSize_32x16,drawing_screen.res_x/2-16-32-8,0,freetype_renderer.han_face,9),
		pinyin_tab(&oamSub,"拼音",SpriteSize_32x16,drawing_screen.res_x/2-16,0,freetype_renderer.han_face,9,1,-1),
		latin_tab(&oamSub,"latin",SpriteSize_32x16,drawing_screen.res_x/2+16+8,0,freetype_renderer.latin_face,7,0,1),
		rating_bar(&oamSub,"",SpriteSize_64x32,drawing_screen.res_x/2-32,drawing_screen.res_y-32,freetype_renderer.latin_face,7,0,0),
		rating_easy(&oamSub,"",SpriteSize_16x16,drawing_screen.res_x/2-32,drawing_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_medium(&oamSub,"",SpriteSize_16x16,drawing_screen.res_x/2-16,drawing_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_hard(&oamSub,"",SpriteSize_16x16,drawing_screen.res_x/2,drawing_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_impossible(&oamSub,"",SpriteSize_16x16,drawing_screen.res_x/2+16,drawing_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		settings_button(&oamSub,"s",SpriteSize_16x16,drawing_screen.res_x-16,drawing_screen.res_y-16,freetype_renderer.latin_face,10,1,1),
		down_button(&oamSub,"下",SpriteSize_16x16,44,0,freetype_renderer.han_face,9,0,0)
{
	this->freetype_renderer.init_screen( SCREEN_MAIN, this->word_screen );
	dmaCopy( bg_dragonBitmap, this->word_screen.bg_base_address, sizeof(bg_dragonBitmap) );
	set_16bpp_sprite_opague( this->word_screen.bg_base_address, 256, 192 );
	bgShow( this->word_screen.bg_id );
	this->word_screen.clear();
	
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new BooleanSetting("0_show_foreign_word","Show Foreign Word",this->init_render_foreign_word) );
	this->settings.add_setting( new BooleanSetting("1_show_pronunciation","Show Pronunciation",this->init_render_pronuciation) );
	this->settings.add_setting( new BooleanSetting("2_show_translation","Show Translation",this->init_render_translation) );
	this->settings.add_setting( new BooleanSetting("3_restore_state","Restore Above Settings on Switch",this->restore_on_switch) );
	this->settings.add_setting( new BooleanSetting("4_clear_screen","Clear Writing Screen on Switch",this->clear_on_switch) );
	
	this->text_buttons.push_back( &this->left_button );
	this->text_buttons.push_back( &this->right_button );
	this->text_buttons.push_back( &this->exit_button );
	this->text_buttons.push_back( &this->clear_button );
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
	
	// Wortliste initialisieren und auf gespeicherten Index positionieren:
	if( this->config ) 
	{
		int word_id = this->config->get_current_word_id();
		for( this->current_word=this->words.begin(); this->current_word!=this->words.end(); this->current_word++ )
		{
			if( (*this->current_word)->id == word_id ) break;
		}
		if( this->current_word==this->words.end() ) this->current_word=this->words.begin();
	}
	
	this->show_settings();
	// Settings would clobber the subscreen, so we initalize it afterwards:
	this->init_subscreen();
	
	bgHide( this->word_screen.bg_id );
}

void NewWordsViewer::init_subscreen()
{
	this->freetype_renderer.init_screen( SCREEN_SUB, this->drawing_screen );
	this->drawing_screen.clear();

	// unteren Bildschirm für Spritenutzung initialisieren:
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	//oamAllocReset( &oamSub );
	oamEnable( &oamSub );

	// vorgerenderte Spritegrafiken laden:
	this->left_button.init_vram( top_left_buttonBitmap, this->left_button.bg_vram );
	this->left_button.init_vram( top_left_button_activeBitmap, this->left_button.bg_active_vram );
	this->right_button.init_vram( top_right_buttonBitmap, this->right_button.bg_vram );
	this->right_button.init_vram( top_right_button_activeBitmap, this->right_button.bg_active_vram );
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	this->clear_button.init_vram( right_center_buttonBitmap, this->clear_button.bg_vram );
	this->clear_button.init_vram( right_center_button_activeBitmap, this->clear_button.bg_active_vram );
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

	this->pinyin_tab.bg_vram = hanzi_tab.bg_vram;
	this->pinyin_tab.bg_active_vram = hanzi_tab.bg_active_vram;
	this->pinyin_tab.bg_inactive_vram = hanzi_tab.bg_inactive_vram;
	this->pinyin_tab.owns_bg_vram = false;
	this->latin_tab.bg_vram = hanzi_tab.bg_vram;
	this->latin_tab.bg_active_vram = hanzi_tab.bg_active_vram;
	this->latin_tab.bg_inactive_vram = hanzi_tab.bg_inactive_vram;
	this->latin_tab.owns_bg_vram = false;
	
	for( TextButtonList::iterator i=this->text_buttons.begin(); i!=this->text_buttons.end(); i++ )
	{
		(*i)->init_text_layer( this->freetype_renderer );
	}
	
	// Palette für 8-Bit-Buttonbeschriftungen wie Hintergrundpalette initialisieren:
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );
}

void NewWordsViewer::show_settings()
{
	this->text_buttons.free_all();
	SettingsDialog settings_dialog( this->freetype_renderer, this->settings, "Word List Settings" );
	settings_dialog.run_until_exit();
	this->init_subscreen();
	this->restore_init_settings();
}

void NewWordsViewer::render( Screen screen )
{
	NewWord* new_word = 0;
	if( this->current_word!=this->words.end() )
	{
		new_word = *this->current_word;
	}
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		if( new_word )
		{
			new_word->render( this->freetype_renderer, this->word_screen, *this, this->library );
		}
	}
	else if( screen == SCREEN_SUB )
	{
		oamClear( &oamSub, 0, 0 );
		int oam_entry = 0;
		if( this->words.size() )
		{
			if( this->current_word != this->words.begin() )
			{
				this->left_button.render_to( oam_entry );
			}
			NewWordList::iterator test_it = this->current_word;
			if( ++test_it != this->words.end() )
			{
				this->right_button.render_to( oam_entry );
			}
		}
		this->exit_button.render_to( oam_entry );
		this->clear_button.render_to( oam_entry );
		this->hanzi_tab.render_to( oam_entry, this->hanzi_tab.x, this->hanzi_tab.y-(this->render_foreign_word ? 0 : 8) );
		this->pinyin_tab.render_to( oam_entry, this->pinyin_tab.x, this->pinyin_tab.y-(this->render_pronuciation ? 0 : 8) );
		this->latin_tab.render_to( oam_entry, this->latin_tab.x, this->latin_tab.y-(this->render_translation ? 0 : 8) );
		this->rating_bar.render_to( oam_entry );
		if( new_word )
		{
			this->library.words_db.read_word(*new_word);
			if( this->rating_easy.active || new_word->rating==RATING_EASY )
				this->rating_easy.render_to( oam_entry );
			if( this->rating_medium.active || new_word->rating==RATING_MEDIUM )
				this->rating_medium.render_to( oam_entry );
			if( this->rating_hard.active || new_word->rating==RATING_HARD )
				this->rating_hard.render_to( oam_entry );
			if( this->rating_impossible.active || new_word->rating==RATING_IMPOSSIBLE )
				this->rating_impossible.render_to( oam_entry );
		}
		this->settings_button.render_to( oam_entry );
		this->down_button.render_to( oam_entry );
		// gepufferte Bilddaten einblenden bzw. in den VRAM kopieren:
		swiWaitForVBlank();
		oamUpdate( &oamSub );
	}
}

void NewWordsViewer::run_until_exit()
{
	if( this->config ) this->config->save_position( *this->current_word );
	this->render( SCREEN_SUB );
	this->render( SCREEN_MAIN );
	touchPosition old_touch;
    touchRead( &old_touch );
	bool touched = false;
	int pixels_drawn = 0;
	int old_distance = 0;
    while( this->current_word!=this->words.end() )
    {
        if( this->config ) this->config->save_position( *this->current_word );
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
		
		NewWordList::iterator test_it = this->current_word;
		if( pressed & KEY_L && this->current_word!=this->words.begin() )
		{
			this->left_button.active = true;
			this->render( SCREEN_SUB );
			this->switch_backwards();
		}
		else if( pressed & KEY_R && ++test_it!=this->words.end() )
		{
			this->right_button.active = true;
			this->render( SCREEN_SUB );
			this->switch_forward();
		}
		
		touchPosition touch;
        touchRead( &touch );
        if( keysCurrent() & KEY_TOUCH )
        {
			if( !touched ) 
			{
				touched = true;
				pixels_drawn = 0;
				old_touch = touch;
			}
			NewWordList::iterator test_it = this->current_word;
            if( this->current_word!=this->words.begin()
				&& this->left_button.is_responsible(touch.px, touch.py)
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->left_button.active )
				{
					this->left_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( ++test_it != this->words.end()
				&& this->right_button.is_responsible(touch.px, touch.py)
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->right_button.active )
				{
					this->right_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->clear_button.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->clear_button.active )
				{
					this->clear_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->exit_button.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->exit_button.active )
				{
					this->exit_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->hanzi_tab.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->hanzi_tab.active )
				{
					this->hanzi_tab.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->pinyin_tab.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->pinyin_tab.active )
				{
					this->pinyin_tab.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->latin_tab.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->latin_tab.active )
				{
					this->latin_tab.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_easy.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->rating_easy.active )
				{
					this->rating_easy.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_medium.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->rating_medium.active )
				{
					this->rating_medium.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_hard.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->rating_hard.active )
				{
					this->rating_hard.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_impossible.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->rating_impossible.active )
				{
					this->rating_impossible.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->settings_button.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
			{
				if( !this->settings_button.active )
				{
					this->settings_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->down_button.is_responsible(touch.px, touch.py) 
				&& pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT
				&& this->current_word!=this->words.end() )
			{
				if( !this->down_button.active )
				{
					this->down_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
			else
			{
				bool changed = false;
				for( TextButtonList::iterator i=this->text_buttons.begin(); i!=this->text_buttons.end(); i++ )
				{
					changed |= (*i)->active;
					(*i)->active = false;
				}
				int x_diff = touch.px - old_touch.px;
				int y_diff = touch.py - old_touch.py;
				int distance = (int)std::sqrt( std::pow(x_diff,2) + std::pow(y_diff,2) );
				if( distance && ((old_distance && (distance <= old_distance*DrawingPad::MAX_ACCELERATION_FACTOR)) 
								|| (distance <= DrawingPad::MAX_ACCELERATION_FACTOR)) )
				{
					old_distance = distance;
					pixels_drawn += distance;
					this->drawing_pad.draw_line( touch.px, touch.py, old_touch.px, old_touch.py );
				}
				if( changed ) this->render( SCREEN_SUB );
			}
			old_touch = touch;
		}
		else if( touched && pixels_drawn < BUTTON_ACTIVATION_DRAW_LIMIT )
		{
			touched = false;
			NewWordList::iterator test_it = this->current_word;
            if( this->current_word != this->words.begin()
				&& this->left_button.is_responsible(old_touch.px, old_touch.py)
				&& this->left_button.active )
            {
				this->switch_backwards();
            }
            else if( ++test_it != this->words.end()
				&& this->right_button.is_responsible(old_touch.px, old_touch.py)
				&& this->right_button.active )
            {
				this->switch_forward();
            }
            else if( this->clear_button.is_responsible(old_touch.px, old_touch.py) 
				&& this->clear_button.active )
            {
				this->clear_button.active = false;
				this->render( SCREEN_SUB );
				this->drawing_pad.clear();
            }
            else if( this->exit_button.is_responsible(old_touch.px, old_touch.py) 
				&& this->exit_button.active )
            {
				this->exit_button.active = false;
				NewWord* word = *this->current_word;
				if( this->config ) this->config->save_position( word );
				return;
            }
            else if( this->hanzi_tab.is_responsible(old_touch.px, old_touch.py) 
				&& this->hanzi_tab.active )
            {
				this->hanzi_tab.active = false;
				this->toggle_foreign_word();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->pinyin_tab.is_responsible(old_touch.px, old_touch.py) 
				&& this->pinyin_tab.active )
            {
				this->pinyin_tab.active = false;
				this->toggle_pronunciation();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->latin_tab.is_responsible(old_touch.px, old_touch.py) 
				&& this->latin_tab.active )
            {
				this->latin_tab.active = false;
				this->toggle_translation();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_easy.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_easy.active )
            {
				this->rating_easy.active = false;
				(*this->current_word)->rating = RATING_EASY;
				this->library.words_db.add_or_write_word( **this->current_word );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_medium.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_medium.active )
            {
				this->rating_medium.active = false;
				(*this->current_word)->rating = RATING_MEDIUM;
				this->library.words_db.add_or_write_word( **this->current_word );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_hard.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_hard.active )
            {
				this->rating_hard.active = false;
				(*this->current_word)->rating = RATING_HARD;
				this->library.words_db.add_or_write_word( **this->current_word );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_impossible.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_impossible.active )
            {
				this->rating_impossible.active = false;
				(*this->current_word)->rating = RATING_IMPOSSIBLE;
				this->library.words_db.add_or_write_word( **this->current_word );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->settings_button.is_responsible(old_touch.px, old_touch.py) 
				&& this->settings_button.active )
            {
				this->settings_button.active = false;
				this->show_settings();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->down_button.is_responsible(old_touch.px, old_touch.py) 
				&& this->down_button.active
				&& this->current_word!=this->words.end() )
            {
				this->down_button.active = false;
				this->text_buttons.free_all();
				TextView::show_word_as_text( this->freetype_renderer, this->library, *this->current_word, 0 );
				this->init_subscreen();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
			else
			{
				this->drawing_pad.draw_point( old_touch.px, old_touch.py );
			}
		}
		else
		{
			swiWaitForVBlank();
			touched = false;
			pixels_drawn = 0;
			for( TextButtonList::iterator i=this->text_buttons.begin(); 
				i!=this->text_buttons.end(); i++ )
			{
				(*i)->active = false;
			}
		}
    }
}

void NewWordsViewer::switch_forward()
{
	this->right_button.active = false;
	this->current_word++;
	this->restore_init_settings_if_needed();
	if( this->clear_on_switch ) this->drawing_pad.clear();
	this->render( SCREEN_MAIN );
	this->render( SCREEN_SUB );
}

void NewWordsViewer::switch_backwards()
{
	this->left_button.active = false;
	this->current_word--;
	this->restore_init_settings_if_needed();
	if( this->clear_on_switch ) this->drawing_pad.clear();
	this->render( SCREEN_MAIN );
	this->render( SCREEN_SUB );
}
