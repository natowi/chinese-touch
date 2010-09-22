#include <iostream>
#include <cmath>

#include <nds.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>
#include <time.h> // word access times
#include <stdlib.h> // rand()
#include <iomanip>
#include <sys/stat.h>
#include <string.h>

#include "new_words.h"
#include "config.h"
#include "error_console.h"
#include "sprite_helper.h"
#include "words_db.h"
#include "text_view.h"

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
#include "bottom_rating_bar.h"
#include "bottom_rating_easy.h"
#include "bottom_rating_easy_active.h"
#include "bottom_rating_medium.h"
#include "bottom_rating_medium_active.h"
#include "bottom_rating_hard.h"
#include "bottom_rating_hard_active.h"
#include "bottom_rating_impossible.h"
#include "bottom_rating_impossible_active.h"
#include "settings_dialog.h"
#include "right_center_button.h"
#include "right_center_button_active.h"
#include "small_top_button.h"
#include "small_top_button_active.h"
#include "small_star.h"
#include "small_star_active.h"
#include "small_trash.h"
#include "small_trash_active.h"
#include "bottom_center_button.h"
#include "bottom_center_button_active.h"
#include "top_center_button.h"
#include "top_center_button_active.h"
#include "fulltext_search.h"
#include "tiny_search.h"
#include "tiny_bubble.h"
#include "image_png.h"
#include "tiny_text.h"
#include "bottom_left_dogear.h"
#include "bottom_left_dogear_active.h"


void NewWord::render( Program& program, RenderScreen& render_screen, RenderSettings& render_settings )
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
	
	// optimization: prevent word re-rendering if an image buffer is already available
	if( !render_settings.stroke_order_image_buffer || render_settings.stroke_order_full_update ) render_screen.clear();
	render_screen.clear_bg();

	// render hanzi centered
	RenderStyle render_style;
	render_style.center_x = true;
	int top = 5;
	int size = 32;
	// optimization: prevent word re-rendering if an image buffer is already available
	if( render_settings.render_foreign_word && (!render_settings.stroke_order_image_buffer || render_settings.stroke_order_full_update) )
	{
		RenderCharList char_list;
		RenderInfo rect = program.ft->render( render_screen, this->hanzi, program.ft->han_face, size, 0, top, &render_style, &char_list );
		top += rect.height;
		if( render_settings.highlight_char.code_point )
		{
			for( RenderCharList::iterator rci=char_list.begin(); rci!=char_list.end(); rci++ )
			{
				if( (*rci)->uc_char.source_offset == render_settings.highlight_char.source_offset )
				{
					if( render_settings.highlight_render_char ) 
						delete render_settings.highlight_render_char;
					render_settings.highlight_render_char = new RenderChar( **rci );
				}
			}
		}
	}
													// save space only for stroke order image
	else if( render_settings.render_foreign_word || !render_settings.render_stroke_order )
	{
		top += size*1.2; // white space instead of foreign word
	}
												// save space only for stroke order image:
	if( render_settings.render_foreign_word || !render_settings.render_stroke_order )
	{
		top += 10; // gap after foreign word
	}
	
	// render pinyin centered
	size = 12;
	if( this->pinyin.length()>20 ) size = 10;
	if( this->pinyin.length()>40 ) size = 8;
	// optimization: prevent pronunciation re-rendering if an image buffer is already available
	if( render_settings.render_pronuciation && (!render_settings.stroke_order_image_buffer || render_settings.stroke_order_full_update) )
	{
		RenderInfo rect = program.ft->render( render_screen, this->pinyin, program.ft->han_face, size, 0, top, &render_style );
		top += rect.height;
	}
													// save space only for stroke order image:
	else if( render_settings.render_pronuciation || !render_settings.render_stroke_order )
	{
		top += size*1.2; // white space instead of pronunciation
	}
												// save space only for stroke order image:
	if( render_settings.render_pronuciation || !render_settings.render_stroke_order )
	{
		top += 10; // gap after pronunciation
	}

	if( render_settings.render_stroke_order )
	{
		render_settings.stroke_order_full_update = false;
		if( render_settings.highlight_render_char )
		{
			for( int line=0; line < render_settings.highlight_render_char->height; line++ )
			{	
				u16* start = render_screen.bg_base_address + line*render_screen.res_x 
					+ render_settings.highlight_render_char->y*render_screen.res_x 
					+ render_settings.highlight_render_char->x;
				for( int pixel=0; pixel < render_settings.highlight_render_char->width; pixel++ )
				{
					start[pixel] = 1<<15 | 15<<10 | 24<<5 | 31<<0;
				}
			}
		}
		if( render_settings.highlight_char.code_point 
			&& (!render_settings.stroke_order_image_buffer 
				|| render_settings.highlight_char.code_point!=render_settings.stroke_order_image_char.code_point) )
		{
			std::stringstream stroke_image_name;
			stroke_image_name << program.base_dir << "/stroke-order/u" << std::hex << std::setfill('0') << std::setw(4) << render_settings.highlight_char.code_point << std::setw(0) << ".png";
			struct stat statbuf;
			if( stat(stroke_image_name.str().c_str(), &statbuf)==0 )
			{
				render_settings.stroke_order_scroll_left=0;
				render_settings.stroke_order_scroll_top=0;
				render_settings.stroke_order_image_buffer_width=0;
				render_settings.stroke_order_image_buffer_height=0;
				read_png( stroke_image_name.str(), render_settings.stroke_order_image_buffer, 
						render_settings.stroke_order_image_buffer_width, render_settings.stroke_order_image_buffer_height );
				render_settings.stroke_order_image_char = render_settings.highlight_char;
			}
		}
		if( render_settings.stroke_order_image_buffer )
		{
			int width = render_screen.res_x;
			int height = render_screen.res_y - top;
			u16* start_address = render_screen.bg_base_address + render_screen.res_x*(render_screen.res_y-height);
			
			if( render_settings.stroke_order_scroll_left > render_settings.stroke_order_image_buffer_width - render_screen.res_x ) 
				render_settings.stroke_order_scroll_left = render_settings.stroke_order_image_buffer_width - render_screen.res_x;
			if( render_settings.stroke_order_scroll_top > render_settings.stroke_order_image_buffer_height - height ) 
				render_settings.stroke_order_scroll_top = render_settings.stroke_order_image_buffer_height - height;
			if( render_settings.stroke_order_scroll_left < 0 ) render_settings.stroke_order_scroll_left = 0;
			if( render_settings.stroke_order_scroll_top < 0 ) render_settings.stroke_order_scroll_top = 0;
			
			int image_residual_width = render_settings.stroke_order_image_buffer_width - render_settings.stroke_order_scroll_left;
			if( image_residual_width < width ) width = image_residual_width;
			int image_residual_height = render_settings.stroke_order_image_buffer_height - render_settings.stroke_order_scroll_top;
			if( image_residual_height < height ) height = image_residual_height;
			
			for( int line = 0; line < height; line++ )
			{
				// copy line data from stroke order image buffer to vram:
				memcpy( start_address+line*render_screen.res_x, 
						render_settings.stroke_order_image_buffer
							+ (line+render_settings.stroke_order_scroll_top)*render_settings.stroke_order_image_buffer_width
							+ (render_settings.stroke_order_scroll_left>>1<<1),
						width*2 );
			}
		}
	}
	else
	{
		std::string lang = "de";

		if( render_settings.render_translation && this->definitions.count(lang) )
		{
			if( this->definitions[lang]->word_type.length() )
			{
				// render word type
				size = 7;
				RenderInfo rect = program.ft->render( render_screen, this->definitions[lang]->word_type, program.ft->latin_face, size, 0, top, &render_style );
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
				RenderInfo rect = program.ft->render( render_screen, text, program.ft->latin_face, size, 0, top, &render_style );
				top += rect.height+10;
			}
			if( this->definitions[lang]->comment.length() )
			{
				// render first n characters of comment
				size = 8;
				std::string text = this->definitions[lang]->comment.substr(0,char_limit);
				if( text.length()==char_limit ) text += "...";
				RenderInfo rect = program.ft->render( render_screen, text, program.ft->latin_face, size, 0, top, &render_style );
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
			program.ft->render( render_screen, text_stream.str(), program.ft->latin_face, size, 0, book_lesson_top );
		}
	}
}


RenderSettings::RenderSettings()
	:	render_foreign_word(true), render_pronuciation(true), render_translation(true), render_stroke_order(false),
		init_render_foreign_word(true), init_render_pronuciation(true), init_render_translation(true), init_render_stroke_order(false),
		restore_on_switch(false), stroke_order_scroll_left(0), stroke_order_scroll_top(0), stroke_order_image_buffer(0),
		stroke_order_image_buffer_width(0), stroke_order_image_buffer_height(0), stroke_order_full_update(true),
		highlight_render_char(0)
{
}

RenderSettings::~RenderSettings()
{
	this->free_buffers();
}

void RenderSettings::free_buffers()
{
	if( this->stroke_order_image_buffer ) delete this->stroke_order_image_buffer;
	this->stroke_order_image_buffer = 0;
	if( this->highlight_render_char ) delete this->highlight_render_char;
	this->highlight_render_char = 0;
}


WordListBrowser::WordListBrowser( ButtonProviderList& provider_list, 
								  FreetypeRenderer& _freetype_renderer, 
								  NewWordList& _words, 
								  RenderScreen& _button_screen,
								  Library& _library ) 
	: 	ButtonProvider(provider_list, _freetype_renderer),
		words(_words), current_word(words.begin()),
		button_screen(_button_screen), library(_library),
		left_button(_button_screen,"<",SpriteSize_32x16,0,0,button_ft.latin_face,10,0,0), 
		right_button(_button_screen,">",SpriteSize_32x16,button_screen.res_x-32,0,button_ft.latin_face,10,2,0), 
		as_text_tab(_button_screen,"ˇ",SpriteSize_32x16,button_screen.res_x/2-16-64-8,/*dynamic*/ 0,button_ft.han_face,16,1,-2),
		foreign_word_tab(_button_screen,"汉字",SpriteSize_32x16,button_screen.res_x/2-16-32,/*dynamic*/ 0,button_ft.han_face,9),
		pronunciation_tab(_button_screen,"",SpriteSize_32x16,button_screen.res_x/2-16,/*dynamic*/ 0,button_ft.han_face,9,1,-1),
		translation_tab(_button_screen,"latin",SpriteSize_32x16,button_screen.res_x/2+16,/*dynamic*/ 0,button_ft.latin_face,7,0,1),
		stroke_order_tab(_button_screen,"",SpriteSize_32x16,button_screen.res_x/2+16+32,/*dynamic*/ 0,button_ft.latin_face,6,0,1),
		rating_bar(_button_screen,"",SpriteSize_64x32,button_screen.res_x/2-32,/*dynamic*/ 0,button_ft.latin_face,7,0,0),
		rating_easy(_button_screen,"",SpriteSize_16x16,button_screen.res_x/2-32,/*dynamic*/ 0,button_ft.latin_face,7,0,0),
		rating_medium(_button_screen,"",SpriteSize_16x16,button_screen.res_x/2-16,/*dynamic*/ 0,button_ft.latin_face,7,0,0),
		rating_hard(_button_screen,"",SpriteSize_16x16,button_screen.res_x/2,/*dynamic*/ 0,button_ft.latin_face,7,0,0),
		rating_impossible(_button_screen,"",SpriteSize_16x16,button_screen.res_x/2+16,/*dynamic*/ 0,button_ft.latin_face,7,0,0),
		add_button(_button_screen,"",SpriteSize_16x16,button_screen.res_x/2+48,button_screen.res_y-16,button_ft.han_face,9,0,0),
		remove_button(_button_screen,"",SpriteSize_16x16,button_screen.res_x/2+64,button_screen.res_y-16,button_ft.han_face,9,0,0),
		search_button(_button_screen,"",SpriteSize_32x16,40,button_screen.res_y-16,button_ft.han_face,9,0,1),
		exit_button(_button_screen,"x",SpriteSize_16x16,0,button_screen.res_y-16,button_ft.latin_face,10,-1,1),
		dogear(_button_screen,"",SpriteSize_32x32,0,button_screen.res_y-32,button_ft.latin_face,7,0,0),
		current_char( current_char_list.begin() )
{
	this->text_buttons.push_back( &this->left_button );
	this->text_buttons.push_back( &this->right_button );
	this->text_buttons.push_back( &this->foreign_word_tab );
	this->text_buttons.push_back( &this->pronunciation_tab );
	this->text_buttons.push_back( &this->translation_tab );
	this->text_buttons.push_back( &this->stroke_order_tab );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );
	this->text_buttons.push_back( &this->as_text_tab );
	this->text_buttons.push_back( &this->add_button );
	this->text_buttons.push_back( &this->remove_button );
	this->text_buttons.push_back( &this->search_button );
	this->text_buttons.push_back( &this->exit_button );
	this->text_buttons.push_back( &this->dogear );
	
	this->add_button.hidden = this->add_button.disabled = true;
	this->remove_button.hidden = this->remove_button.disabled = true;
	
	// The dog-ear is hidden by default (can be explicitly enabled by caller)
	this->dogear.hidden = this->dogear.disabled = true;
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
	this->as_text_tab.init_vram( top_center_buttonBitmap, this->as_text_tab.bg_vram );
	this->as_text_tab.init_vram( top_center_button_activeBitmap, this->as_text_tab.bg_active_vram );
	this->rating_bar.init_vram( bottom_rating_barBitmap, this->rating_bar.bg_vram );
	this->rating_bar.bg_prio = 2; // place bar behind rating emotes
	this->rating_easy.init_vram( bottom_rating_easyBitmap, this->rating_easy.bg_vram );
	this->rating_easy.init_vram( bottom_rating_easy_activeBitmap, this->rating_easy.bg_active_vram );
	this->rating_medium.init_vram( bottom_rating_mediumBitmap, this->rating_medium.bg_vram );
	this->rating_medium.init_vram( bottom_rating_medium_activeBitmap, this->rating_medium.bg_active_vram );
	this->rating_hard.init_vram( bottom_rating_hardBitmap, this->rating_hard.bg_vram );
	this->rating_hard.init_vram( bottom_rating_hard_activeBitmap, this->rating_hard.bg_active_vram );
	this->rating_impossible.init_vram( bottom_rating_impossibleBitmap, this->rating_impossible.bg_vram );
	this->rating_impossible.init_vram( bottom_rating_impossible_activeBitmap, this->rating_impossible.bg_active_vram );
	this->add_button.init_vram( small_starBitmap, this->add_button.bg_vram );
	this->add_button.init_vram( small_star_activeBitmap, this->add_button.bg_active_vram );
	this->remove_button.init_vram( small_trashBitmap, this->remove_button.bg_vram );
	this->remove_button.init_vram( small_trash_activeBitmap, this->remove_button.bg_active_vram );
	this->search_button.init_vram( bottom_center_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( bottom_center_button_activeBitmap, this->search_button.bg_active_vram );
	this->search_button.init_vram( tiny_searchBitmap, this->search_button.fg_vram );
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	this->dogear.init_vram( bottom_left_dogearBitmap, this->dogear.bg_vram );
	this->dogear.init_vram( bottom_left_dogear_activeBitmap, this->dogear.bg_active_vram );

	this->pronunciation_tab.bg_vram = this->foreign_word_tab.bg_vram;
	this->pronunciation_tab.bg_active_vram = this->foreign_word_tab.bg_active_vram;
	this->pronunciation_tab.bg_inactive_vram = this->foreign_word_tab.bg_inactive_vram;
	this->pronunciation_tab.init_vram( tiny_bubbleBitmap, this->pronunciation_tab.fg_vram );
	this->pronunciation_tab.owns_bg_vram = false;
	this->translation_tab.bg_vram = this->foreign_word_tab.bg_vram;
	this->translation_tab.bg_active_vram = this->foreign_word_tab.bg_active_vram;
	this->translation_tab.bg_inactive_vram = this->foreign_word_tab.bg_inactive_vram;
	this->translation_tab.owns_bg_vram = false;
	this->stroke_order_tab.bg_vram = this->foreign_word_tab.bg_vram;
	this->stroke_order_tab.bg_active_vram = this->foreign_word_tab.bg_active_vram;
	this->stroke_order_tab.bg_inactive_vram = this->foreign_word_tab.bg_inactive_vram;
	this->stroke_order_tab.init_vram( tiny_textBitmap, this->stroke_order_tab.fg_vram );
	this->stroke_order_tab.owns_bg_vram = false;
	
	ButtonProvider::init_button_vram();
}

void WordListBrowser::free_button_vram()
{
	 // also free stroke order image buffer if allocated:
	this->free_buffers();
	ButtonProvider::free_button_vram();
}


void WordListBrowser::render_buttons( OamState* oam_state, int& oam_entry )
{
	if( this->render_stroke_order )
	{
		// hide left button when at the beginning of the character list:
		if( this->current_char == this->current_char_list.begin() ) this->left_button.hidden = this->left_button.disabled = true;
		else this->left_button.hidden = this->left_button.disabled = false;
		// hide right button when at the end of the character list:
		UCCharList::iterator test_it = this->current_char;
		if( this->current_char != this->current_char_list.end() ) test_it++;
		if( test_it == this->current_char_list.end() ) this->right_button.hidden = this->right_button.disabled = true;
		else this->right_button.hidden = this->right_button.disabled = false;
	}
	else
	{
		// hide left button when at the beginning of the word list:
		if( this->current_word == this->words.begin() ) this->left_button.hidden = this->left_button.disabled = true;
		else this->left_button.hidden = this->left_button.disabled = false;
		// hide right button when at the end of the word list:
		NewWordList::iterator test_it = this->current_word;
		if( this->current_word != this->words.end() ) test_it++;
		if( test_it == this->words.end() ) this->right_button.hidden = this->right_button.disabled = true;
		else this->right_button.hidden = this->right_button.disabled = false;
	}
	
	NewWord* word = 0;
	if( this->current_word != this->words.end() ) word = *this->current_word;
	
	// vertically translate several edge sensors depending on the current list/display state:
	this->left_button.y =  (this->render_stroke_order ? 32 : 0);
	this->right_button.y =  (this->render_stroke_order ? 32 : 0);
	this->as_text_tab.y = (word ? 0 : -12) + (this->render_stroke_order ? 32 : 0);
	this->foreign_word_tab.y = (word ? ( this->render_foreign_word ? 0 : -8 ) : -12) + (this->render_stroke_order ? 32 : 0);
	this->pronunciation_tab.y = (word ? ( this->render_pronuciation ? 0 : -8 ) : -12) + (this->render_stroke_order ? 32 : 0);
	this->translation_tab.y = (word ? ( this->render_translation ? 0 : -8 ) : -12) + (this->render_stroke_order ? 32 : 0);
	this->stroke_order_tab.y = (word ? ( this->render_stroke_order ? 0 : -8 ) : -12) + (this->render_stroke_order ? 32 : 0);
	
	this->rating_bar.y = word ? this->button_screen.res_y-16 : this->button_screen.res_y-16+12;
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
		/* make remove-button available if word is orphan with no lesson reference 
			or is from a static dictionary and loosely associated with a lesson: */
		this->remove_button.hidden = this->remove_button.disabled = 
			!( (word->id && !word->lesson)
				|| (word->lesson && ((word->lesson->number && (word->file_id==0))
									|| ((word->lesson->number==0) && (word->rating!=RATING_NONE)))) );
	}
	
	// let ButtonProvider do the actual rendering
	ButtonProvider::render_buttons(oam_state, oam_entry);
}

ButtonAction WordListBrowser::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->left_button && this->switch_backwards() )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	else if( text_button == &this->right_button && this->switch_forward() )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	else if( text_button == &this->foreign_word_tab )
	{
		this->toggle_foreign_word();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	else if( text_button == &this->pronunciation_tab )
	{
		this->toggle_pronunciation();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	else if( text_button == &this->translation_tab )
	{
		this->toggle_translation();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	else if( text_button == &this->rating_easy 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_EASY;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	else if( text_button == &this->rating_medium 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_MEDIUM;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	else if( text_button == &this->rating_hard 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_HARD;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	else if( text_button == &this->rating_impossible 
		&& this->current_word != this->words.end() )
	{
		(*this->current_word)->rating = RATING_IMPOSSIBLE;
		this->library.words_db.add_or_write_word( **this->current_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	else if( text_button == &this->remove_button
		&& this->current_word != this->words.end() )
	{
		this->library.words_db.delete_word( **this->current_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	else if( text_button == &this->exit_button )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	else if( text_button == &this->dogear )
	{
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	
	return ButtonProvider::handle_button_pressed(text_button);
}

bool WordListBrowser::switch_forward()
{
	this->free_buffers();
	if( this->render_stroke_order && this->current_char != this->current_char_list.end() )
	{
		UCCharList::iterator test_it = this->current_char;
		if( ++test_it != this->current_char_list.end() )
		{
			this->current_char++;
			// this->restore_init_settings_if_needed();
			return true;
		}
	}
	else if( this->current_word != this->words.end() )
	{
		NewWordList::iterator test_it = this->current_word;
		if( ++test_it != this->words.end() )
		{
			this->current_word++;
			this->restore_init_settings_if_needed();
			return true;
		}
	}
	return false;
}

bool WordListBrowser::switch_backwards()
{
	this->free_buffers();
	if( this->render_stroke_order && this->current_char != this->current_char_list.begin() )
	{
		this->current_char--;
		// this->restore_init_settings_if_needed();
		return true;
	}
	else if( this->current_word != this->words.begin() )
	{
		this->current_word--;
		this->restore_init_settings_if_needed();
		return true;
	}
	return false;
}

ButtonAction WordListBrowser::handle_console_button_event( int pressed, int held, int released )
{
	// delegate console L and R buttons to touch screen left/right button handlers:
	if( pressed & KEY_L )
	{
		return this->handle_button_pressed( &this->left_button );
	}
	else if( pressed & KEY_R )
	{
		return this->handle_button_pressed( &this->right_button );
	}
	
	return ButtonProvider::handle_console_button_event( pressed, held, released );
}

void WordListBrowser::toggle_foreign_word() 
{ 
	this->render_foreign_word = !this->render_foreign_word;
	this->stroke_order_full_update = true; // need to refresh word rendering
}
void WordListBrowser::toggle_pronunciation() 
{ 
	this->render_pronuciation = !this->render_pronuciation; 
	this->stroke_order_full_update = true; // need to refresh word rendering
}
void WordListBrowser::toggle_translation() 
{ 
	// replace stroke order image with translation fields:
	if( !this->render_translation && this->render_stroke_order ) this->toggle_stroke_order();
	this->render_translation = !this->render_translation;
	this->stroke_order_full_update = true; // need to refresh word rendering
}
void WordListBrowser::toggle_stroke_order() 
{ 
	// replace pronunciation and translation fields with stroke order image:
	if( !this->render_stroke_order && this->render_pronuciation ) 
	{
		this->toggle_pronunciation();
	}
	if( !this->render_stroke_order && this->render_translation )
	{
		this->toggle_translation();
	}
	this->render_stroke_order = !this->render_stroke_order; 
	if( this->stroke_order_image_buffer )
	{
		this->free_buffers();
	}
	if( this->render_stroke_order )
	{
		this->current_char_list.clear();
		if( this->current_word != this->words.end() )
		{
			utf8_to_ucs4( (const unsigned char*)(*this->current_word)->hanzi.c_str(), this->current_char_list );
		}
		this->current_char = this->current_char_list.begin();
	}
	this->stroke_order_full_update = true; // need to refresh word rendering
}

void WordListBrowser::restore_init_settings()
{
	this->render_foreign_word = this->init_render_foreign_word;
	this->render_pronuciation = this->init_render_pronuciation;
	this->render_translation = this->init_render_translation;
}

void WordListBrowser::restore_init_settings_if_needed()
{ 
	if( this->restore_on_switch ) 
		this->restore_init_settings();
}

bool random_sort_predicate( NewWord* left, NewWord* right )
{
	return rand() > RAND_MAX/2;
}

void WordListBrowser::randomize_list()
{
	this->words.sort( random_sort_predicate );
	this->current_word = this->words.begin();
}


// --------------------------------------------------------------------------------------------------------


int NewWordsViewer::BUTTON_ACTIVATION_DRAW_LIMIT = 5;

NewWordsViewer::NewWordsViewer( Program& _program, int _recursion_depth, NewWordList& _words, bool _save_position, bool _randomize_list, bool _show_settings )
	: Mode(_program, _recursion_depth), save_position(_save_position), word_screen(SCREEN_MAIN), drawing_screen(SCREEN_SUB),
		word_browser(button_provider_list, *_program.ft, _words, drawing_screen, *_program.library),
		drawing_pad(drawing_screen),
		clear_button(drawing_screen,"C\nL\nE\nA\nR",SpriteSize_32x64,drawing_screen.res_x-16,drawing_screen.res_y/2-32,_program.ft->latin_face,9,-7,3),
		settings_button(drawing_screen,"s",SpriteSize_16x16,drawing_screen.res_x-16,drawing_screen.res_y-16,_program.ft->latin_face,10,1,1),
		scroll_field_overlay_0(drawing_screen,"",SpriteSize_64x32,64*0,0,_program.ft->latin_face,9,0,16),
		scroll_field_overlay_1(drawing_screen,"Scroll",SpriteSize_64x32,64*1,0,_program.ft->latin_face,9,0,8),
		scroll_field_overlay_2(drawing_screen,"here!",SpriteSize_64x32,64*2,0,_program.ft->latin_face,9,0,8),
		scroll_field_overlay_3(drawing_screen,"",SpriteSize_64x32,64*3,0,_program.ft->latin_face,9,0,16),
		pixels_drawn(0), clear_on_switch(true), randomize_list(_randomize_list), scrolling(false)
{
	// disable child mode buttons when recursion limit is reached:
	if( this->recursion_depth>=Mode::MAX_RECURSION_DEPTH )
	{
		this->word_browser.as_text_tab.hidden = this->word_browser.as_text_tab.disabled = true;
		this->word_browser.search_button.hidden = this->word_browser.search_button.disabled = true;
	}
	
	// In this mode we want to restore visibility settings on switch by default:
	// FIXME: make default configurable somewhere
	this->word_browser.restore_on_switch = true;
	
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new BooleanSetting("0_show_foreign_word","Show Foreign Word",this->word_browser.init_render_foreign_word) );
	this->settings.add_setting( new BooleanSetting("1_show_pronunciation","Show Pronunciation",this->word_browser.init_render_pronuciation) );
	this->settings.add_setting( new BooleanSetting("2_show_translation","Show Translation",this->word_browser.init_render_translation) );
	this->settings.add_setting( new BooleanSetting("3_restore_state","Restore Above Settings on Switch",this->word_browser.restore_on_switch) );
	this->settings.add_setting( new BooleanSetting("4_clear_screen","Clear Writing Screen on Switch",this->clear_on_switch) );
	this->settings.add_setting( new BooleanSetting("5_randomize_list","Randomize List Now",this->randomize_list) );
	
	this->text_buttons.push_back( &this->clear_button );
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->scroll_field_overlay_0 );
	this->text_buttons.push_back( &this->scroll_field_overlay_1 );
	this->text_buttons.push_back( &this->scroll_field_overlay_2 );
	this->text_buttons.push_back( &this->scroll_field_overlay_3 );
	
	this->scroll_field_overlay_0.disabled = this->scroll_field_overlay_0.hidden
	= this->scroll_field_overlay_1.disabled = this->scroll_field_overlay_1.hidden
	= this->scroll_field_overlay_2.disabled = this->scroll_field_overlay_2.hidden
	= this->scroll_field_overlay_3.disabled = this->scroll_field_overlay_3.hidden
	= true;
	
	// initialize word list on stored word index (only used when calling new words from a lesson):
	if( this->save_position )
	{
		int word_id = this->program.config->get_current_word_id();
		for( this->word_browser.current_word = this->word_browser.words.begin(); 
			this->word_browser.current_word != this->word_browser.words.end(); 
			this->word_browser.current_word++ )
		{
			if( (*this->word_browser.current_word)->id == word_id ) break;
		}
		if( this->word_browser.current_word == this->word_browser.words.end() ) 
			this->word_browser.current_word = this->word_browser.words.begin();
	}
	
	if( _show_settings )
	{
		this->show_settings();
		// show_settings() calls init_mode() and init_vram(), so no need to do this again
	}
	else
	{
		this->init_mode();
		this->init_vram();
	}
}

void NewWordsViewer::init_mode()
{
	this->Mode::button_ft.init_screen( this->word_screen );
	this->word_screen.clear();
	this->word_screen.clear_bg();
	
	this->Mode::button_ft.init_screen( this->drawing_screen );
	this->drawing_screen.clear();

	Mode::init_mode();
}

void NewWordsViewer::init_vram()
{
	Mode::init_vram();
}

void NewWordsViewer::init_button_vram()
{
	this->clear_button.init_vram( right_center_buttonBitmap, this->clear_button.bg_vram );
	this->clear_button.init_vram( right_center_button_activeBitmap, this->clear_button.bg_active_vram );
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	
	int overlay_buffer_size = scroll_field_overlay_0.width*scroll_field_overlay_0.height;
	u16* overlay_buffer = new u16[ overlay_buffer_size ];
	for( u16* pos=overlay_buffer; pos<overlay_buffer+overlay_buffer_size; pos++ )
	{
		int grit_size = 8;
		int offset = pos - overlay_buffer;
		if( (offset % grit_size) && ((offset / scroll_field_overlay_0.width) % grit_size - grit_size+1) )
		{
			*pos = 1<<15 | 31<<10 | 24<<5 | 20<<0;
		}
		else *pos = 1<<15 | 24<<10 | 20<<5 | 16<<0;
	}
	this->scroll_field_overlay_0.init_vram( overlay_buffer, this->scroll_field_overlay_0.bg_vram );
	this->scroll_field_overlay_1.bg_vram = this->scroll_field_overlay_0.bg_vram;
	this->scroll_field_overlay_1.owns_bg_vram = false;
	this->scroll_field_overlay_2.bg_vram = this->scroll_field_overlay_0.bg_vram;
	this->scroll_field_overlay_2.owns_bg_vram = false;
	this->scroll_field_overlay_3.bg_vram = this->scroll_field_overlay_0.bg_vram;
	this->scroll_field_overlay_3.owns_bg_vram = false;
	delete overlay_buffer;
	this->scroll_field_overlay_0.bg_prio = this->scroll_field_overlay_0.text_prio
	=this->scroll_field_overlay_1.bg_prio = this->scroll_field_overlay_1.text_prio
	=this->scroll_field_overlay_2.bg_prio = this->scroll_field_overlay_2.text_prio
	=this->scroll_field_overlay_3.bg_prio = this->scroll_field_overlay_3.text_prio
	= 0;
	
	ButtonProvider::init_button_vram();
}

NewWordsViewer::~NewWordsViewer()
{
	this->word_screen.clear();
	this->word_screen.clear_bg();
}

void NewWordsViewer::show_settings()
{
	this->free_vram();
	SettingsDialog settings_dialog( this->program, this->recursion_depth, this->settings, "Word List Settings" );
	settings_dialog.run_until_exit();
	// TODO: store all but randomize_list initial settings now
	// TODO: store randomize_list setting now, but only if this is the initial settings screen
	this->word_browser.restore_init_settings();
	if( this->randomize_list )
	{
		this->word_browser.randomize_list();
		// disable randomization to prevent unvoluntary execution,
		// when the user calls the settings menu next time:
		this->randomize_list = false;
	}
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
		if( new_word )
		{
			if( this->save_position ) this->program.config->save_word_position( new_word );
			if( this->word_browser.render_stroke_order && this->word_browser.current_char!=this->word_browser.current_char_list.end() )
			{
				this->word_browser.highlight_char = *this->word_browser.current_char;
			}
			else
			{
				this->word_browser.highlight_char.init();
			}
			new_word->render( this->program, this->word_screen, this->word_browser );
		}
	}
	if( screen == SCREEN_SUB )
	{
		this->scroll_field_overlay_0.hidden
		= this->scroll_field_overlay_1.hidden
		= this->scroll_field_overlay_2.hidden
		= this->scroll_field_overlay_3.hidden
		= !this->word_browser.render_stroke_order;
	}
	
	Mode::render( screen );
}

ButtonAction NewWordsViewer::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->clear_button )
	{
		this->drawing_pad.clear();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.exit_button )
	{
		if( this->save_position && (this->word_browser.current_word != this->word_browser.words.end()) )
		{
			this->program.config->save_word_position( *this->word_browser.current_word );
		}
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->settings_button )
	{
		this->show_settings();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.as_text_tab
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		TextView::show_word_as_text( this->program, *this->word_browser.current_word, (*this->word_browser.current_word)->lesson, this->recursion_depth );
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.search_button )
	{
		this->free_vram();
		FulltextSearch *fulltext_search = new FulltextSearch( this->program, this->recursion_depth, 
				(this->word_browser.current_word!=this->word_browser.words.end() ? 
					(*this->word_browser.current_word)->lesson : 0) );
		// explicitly replace exit button with dog-ear to show user, that she is in a sub mode
		fulltext_search->word_browser.exit_button.hidden = fulltext_search->word_browser.exit_button.disabled = true;
		fulltext_search->word_browser.dogear.hidden = fulltext_search->word_browser.dogear.disabled = false;
		fulltext_search->run_until_exit();
		delete fulltext_search;
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( this->clear_on_switch && !this->word_browser.render_stroke_order
		&& (text_button == &this->word_browser.left_button
			|| text_button == &this->word_browser.right_button) )
	{
		this->drawing_pad.clear();
		return BUTTON_ACTION_PRESSED;
	}
	if( text_button == &this->word_browser.stroke_order_tab )
	{
		this->word_browser.toggle_stroke_order();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}

ButtonAction NewWordsViewer::handle_touch_begin( touchPosition touch )
{
	this->old_touch = touch;
	this->old_distance = 0;
	this->pixels_drawn = 0;
	this->scrolling = false;
	ButtonAction action=BUTTON_ACTION_UNHANDLED;
	if( this->word_browser.render_stroke_order && touch.py<32 )
	{
		this->scrolling = true;
		action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
	}
	else
	{
		// check for touch screen button activation:
		action = Mode::handle_touch_begin( touch );
	}
	// only draw a pixel or start scrolling if no button was activated by the touch:
	if( action == BUTTON_ACTION_UNHANDLED )
	{
		this->drawing_pad.draw_point( touch.px, touch.py );
		this->pixels_drawn += 1;
		action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return action;
}

ButtonAction NewWordsViewer::handle_touch_drag( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	int x_diff = touch.px - this->old_touch.px;
	int y_diff = touch.py - this->old_touch.py;
	int distance = (int)std::sqrt( std::pow(x_diff,2) + std::pow(y_diff,2) );
	if( this->scrolling )
	{
		this->word_browser.stroke_order_scroll_left -= x_diff*2;
		this->word_browser.stroke_order_scroll_top -= y_diff*2;
		action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN;
	}
	else if( this->pixels_drawn < NewWordsViewer::BUTTON_ACTIVATION_DRAW_LIMIT )
	{
		// check for touch screen button activation, but only if the users
		// is not already writing to the screen:
		action |= Mode::handle_touch_drag( touch );
	}
	// if we are not hovering a touch screen button, draw a line or scroll:
	if( (action == BUTTON_ACTION_UNHANDLED)
		&& (distance 
			&& ((this->old_distance && (distance <= this->old_distance*DrawingPad::MAX_ACCELERATION_FACTOR)) 
				|| (distance <= DrawingPad::MAX_ACCELERATION_FACTOR))) )
	{
		this->pixels_drawn += distance;
		this->drawing_pad.draw_line( touch.px, touch.py, this->old_touch.px, this->old_touch.py );
		action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
	}
	this->old_distance = distance;
	this->old_touch = touch;
	
	return action;
}

ButtonAction NewWordsViewer::handle_touch_end( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// check for touch screen button activation, but only if the users
	// is not already writing to the screen:
	if( !this->scrolling && this->pixels_drawn < NewWordsViewer::BUTTON_ACTIVATION_DRAW_LIMIT )
	{
		action |= Mode::handle_touch_end( touch );
	}
	if( action == BUTTON_ACTION_UNHANDLED )
		action |= this->handle_touch_drag( touch );
	this->scrolling = false;
	
	return action;
}
