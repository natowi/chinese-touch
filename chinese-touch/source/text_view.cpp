#include <math.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>

#include "text_view.h"
#include "unicode.h"
#include "error_console.h"
#include "sprite_helper.h"
#include "words_db.h"

#include "greys256.h"
#include "top_left_button.h"
#include "top_left_button_active.h"
#include "top_right_button.h"
#include "top_right_button_active.h"
#include "bottom_left_button.h"
#include "bottom_left_button_active.h"
#include "bg_dragon.h"
#include "top_paper_tab.h"
#include "top_paper_tab_active.h"
#include "top_paper_tab_inactive.h"
#include "bottom_rating_bar.h"
#include "bottom_rating_easy.h"
#include "bottom_rating_medium.h"
#include "bottom_rating_hard.h"
#include "bottom_rating_impossible.h"


int TextView::LINE_HEIGHT = 16;
int TextView::BUTTON_ACTIVATION_SCROLL_LIMIT = 5;
int TextView::MAX_ACCELERATION_FACTOR = 10;

TextView::TextView( FreetypeRenderer& _ft, Config& _config, Text& _text, Dictionary& _dict )
	: freetype_renderer(_ft), config(_config), text(_text), dict(_dict), y_offset(5), v_y(0), sub_frame_count(0),
		current_new_word_list_it(this->current_new_word_list.begin()), current_highlight(0),
		current_highlight_x(0), current_highlight_y(0), context_mode(CONTEXT_WORDS_BY_CONTEXT),
		context_render_char(0),
		left_button(&oamSub,"<",SpriteSize_32x16,0,0,freetype_renderer.latin_face,10,0,0), 
		right_button(&oamSub,">",SpriteSize_32x16,text_screen.res_x-32,0,freetype_renderer.latin_face,10,2,0), 
		exit_button(&oamSub,"x",SpriteSize_16x16,0,text_screen.res_y-16,freetype_renderer.latin_face,10,-1,1),
		hanzi_tab(&oamSub,"汉字",SpriteSize_32x16,text_screen.res_x/2-16-32-8,0,freetype_renderer.han_face,9),
		pinyin_tab(&oamSub,"拼音",SpriteSize_32x16,text_screen.res_x/2-16,0,freetype_renderer.han_face,9,1,-1),
		latin_tab(&oamSub,"latin",SpriteSize_32x16,text_screen.res_x/2+16+8,0,freetype_renderer.latin_face,7,0,1),
		rating_bar(&oamSub,"",SpriteSize_64x32,text_screen.res_x/2-32,text_screen.res_y-32,freetype_renderer.latin_face,7,0,0),
		rating_easy(&oamSub,"",SpriteSize_16x16,text_screen.res_x/2-32,text_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_medium(&oamSub,"",SpriteSize_16x16,text_screen.res_x/2-16,text_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_hard(&oamSub,"",SpriteSize_16x16,text_screen.res_x/2,text_screen.res_y-16,freetype_renderer.latin_face,7,0,0),
		rating_impossible(&oamSub,"",SpriteSize_16x16,text_screen.res_x/2+16,text_screen.res_y-16,freetype_renderer.latin_face,7,0,0)
{
	this->freetype_renderer.init_screen( SCREEN_MAIN, this->word_screen );
	dmaCopy( bg_dragonBitmap, this->word_screen.bg_base_address, sizeof(bg_dragonBitmap) );
	set_16bpp_sprite_opague( this->word_screen.bg_base_address, 256, 192 );
	bgShow( this->word_screen.bg_id );
	this->word_screen.clear();
	this->freetype_renderer.init_screen( SCREEN_SUB, this->text_screen );
	this->text_screen.clear();

	// unteren Bildschirm für Spritenutzung initialisieren:
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	oamAllocReset( &oamSub );
	oamEnable( &oamSub );

	// vorgerenderte Spritegrafiken laden:
	this->left_button.init_vram( top_left_buttonBitmap, this->left_button.bg_vram );
	this->left_button.init_vram( top_left_button_activeBitmap, this->left_button.bg_active_vram );
	this->right_button.init_vram( top_right_buttonBitmap, this->right_button.bg_vram );
	this->right_button.init_vram( top_right_button_activeBitmap, this->right_button.bg_active_vram );
	this->exit_button.init_vram( bottom_left_buttonBitmap, this->exit_button.bg_vram );
	this->exit_button.init_vram( bottom_left_button_activeBitmap, this->exit_button.bg_active_vram );
	this->hanzi_tab.init_vram( top_paper_tabBitmap, this->hanzi_tab.bg_vram );
	this->hanzi_tab.init_vram( top_paper_tab_activeBitmap, this->hanzi_tab.bg_active_vram );
	this->hanzi_tab.init_vram( top_paper_tab_inactiveBitmap, this->hanzi_tab.bg_inactive_vram );
	this->rating_bar.init_vram( bottom_rating_barBitmap, this->rating_bar.bg_vram );
	this->rating_bar.bg_prio = 2; // place bar behind rating emotes
	this->rating_easy.init_vram( bottom_rating_easyBitmap, this->rating_easy.bg_vram );
	this->rating_medium.init_vram( bottom_rating_mediumBitmap, this->rating_medium.bg_vram );
	this->rating_hard.init_vram( bottom_rating_hardBitmap, this->rating_hard.bg_vram );
	this->rating_impossible.init_vram( bottom_rating_impossibleBitmap, this->rating_impossible.bg_vram );

	this->pinyin_tab.bg_vram = hanzi_tab.bg_vram;
	this->pinyin_tab.bg_active_vram = hanzi_tab.bg_active_vram;
	this->pinyin_tab.bg_inactive_vram = hanzi_tab.bg_inactive_vram;
	this->pinyin_tab.owns_bg_vram = false;
	this->latin_tab.bg_vram = hanzi_tab.bg_vram;
	this->latin_tab.bg_active_vram = hanzi_tab.bg_active_vram;
	this->latin_tab.bg_inactive_vram = hanzi_tab.bg_inactive_vram;
	this->latin_tab.owns_bg_vram = false;

	this->text_buttons.push_back( &this->left_button );
	this->text_buttons.push_back( &this->right_button );
	this->text_buttons.push_back( &this->exit_button );
	this->text_buttons.push_back( &this->hanzi_tab );
	this->text_buttons.push_back( &this->pinyin_tab );
	this->text_buttons.push_back( &this->latin_tab );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );

	for( TextButtonList::iterator i=this->text_buttons.begin(); i!=this->text_buttons.end(); i++ )
	{
		if( (*i)->owns_bg_vram )
		{
			// Alpha-Bits bei definierten Spritepixeln auf "undurchsichtig" setzen:
			if( (*i)->bg_vram ) set_16bpp_sprite_opague( (*i)->bg_vram, (*i)->width, (*i)->height, 0 );
			if( (*i)->bg_active_vram ) set_16bpp_sprite_opague( (*i)->bg_active_vram, (*i)->width, (*i)->height, 0 );
			if( (*i)->bg_inactive_vram ) set_16bpp_sprite_opague( (*i)->bg_inactive_vram, (*i)->width, (*i)->height, 0 );
		}
		if( (*i)->text.length() )
		{
			// VRAM für 8-Bit-Buttonbeschriftungs-Sprites reservieren:
			(*i)->text_vram = oamAllocateGfx( &oamSub, SpriteSize_32x16, SpriteColorFormat_256Color );
			RenderScreenBuffer button_text( (*i)->width, (*i)->height );
			RenderStyle render_style;
			render_style.center_x = true;
			this->freetype_renderer.render( button_text, (*i)->text, 
				(*i)->face, (*i)->font_size, 0, 1, &render_style );
			// Spritekonvertierung:
			// (Zwischenpufferung aus Bequemlichkeit, weil VRAM nur mit 16-bit-Wörtern beschreibbbar)
			u8 conversion_buffer[(*i)->width * (*i)->height];
			tile_8bpp_sprite( (u8*)(button_text.base_address), conversion_buffer, (*i)->width, (*i)->height );
			memcpy( (*i)->text_vram, conversion_buffer, (*i)->width * (*i)->height * 1 );
		}
	}
	
	// Palette für 8-Bit-Buttonbeschriftungen wie Hintergrundpalette initialisieren:
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );
	
	UCCharList char_list;
	if( !utf8_to_ucs4((unsigned char*)text.c_str(), char_list) )
    {
        WARN( "error in utf-8 input (non fatal)" );
    }
	unsigned int prev_size = 0;
	RenderInfo info( 0, 0, 0, 0 );
	int org_size = char_list.size();
	while( prev_size!=char_list.size() )
	{
		// break when no characters where consumed within two consecutive iterations
		// 		to prevent endless loops
		prev_size = char_list.size();
		BufferedLine* buffered_line = new BufferedLine();
		RenderStyle render_style;
		render_style.multiline = false;
		render_style.indentation_offset = info.indentation_offset;
		info = this->freetype_renderer.render( *buffered_line, char_list, 
			this->freetype_renderer.latin_face, 8, 0, 0, &render_style, &buffered_line->render_char_list );
		this->push_back( buffered_line );
		// render progress bar:
		int progress_bar_pixels = (org_size-char_list.size())*this->word_screen.res_x/org_size;
		for( int i=-4; i<=4; i++ )
			for( int j=0; j<progress_bar_pixels; j++ )
			{
				u16* ptr = this->word_screen.bg_base_address+(this->word_screen.res_y/2+i)*this->word_screen.res_x+j;
				*ptr |= 31<<10;
				*ptr &= 0xffff^((1<<4)|(1<<9));
			}
	}
	bgHide( this->word_screen.bg_id );
}

TextView::~TextView()
{
	if( this->current_highlight ) delete this->current_highlight;
	for( TextView::iterator tv_it = this->begin();
		tv_it != this->end(); tv_it++ )
	{
		if( *tv_it ) delete *tv_it;
	}
}

void TextView::render( Screen screen, bool update_sprites )
{
	NewWord* new_word = 0;
	if( this->current_new_word_list_it != this->current_new_word_list.end() )
	{
		new_word = *this->current_new_word_list_it;
	}
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		if( new_word )
		{
			new_word->render( this->freetype_renderer, this->word_screen, *this );
		}
	}
	else if( screen == SCREEN_SUB )
	{
		this->sub_frame_count++;
		if( update_sprites )
		{
			oamClear( &oamSub, 0, 0 );
			int oam_entry = 0;
			if( this->current_new_word_list.size() )
			{
				if( this->current_new_word_list_it != this->current_new_word_list.begin() )
				{
					this->left_button.render_to( oam_entry );
				}
				NewWordList::iterator test_it = this->current_new_word_list_it;
				if( ++test_it != this->current_new_word_list.end() )
				{
					this->right_button.render_to( oam_entry );
				}
			}
			this->exit_button.render_to( oam_entry );
			this->hanzi_tab.render_to( oam_entry, this->hanzi_tab.x, this->hanzi_tab.y-(this->render_hanzi ? 0 : 8) );
			this->pinyin_tab.render_to( oam_entry, this->pinyin_tab.x, this->pinyin_tab.y-(this->render_pinyin ? 0 : 8) );
			this->latin_tab.render_to( oam_entry, this->latin_tab.x, this->latin_tab.y-(this->render_translation ? 0 : 8) );
			this->rating_bar.render_to( oam_entry );
			if( new_word )
			{
				if( !WordsDB::read_word(*new_word) ) WordsDB::add_or_write_word( *new_word );
				if( this->rating_easy.active || new_word->rating==RATING_EASY )
					this->rating_easy.render_to( oam_entry );
				if( this->rating_medium.active || new_word->rating==RATING_MEDIUM )
					this->rating_medium.render_to( oam_entry );
				if( this->rating_hard.active || new_word->rating==RATING_HARD )
					this->rating_hard.render_to( oam_entry );
				if( this->rating_impossible.active || new_word->rating==RATING_IMPOSSIBLE )
					this->rating_impossible.render_to( oam_entry );
			}
			
			// gepufferte Bilddaten einblenden bzw. in den VRAM kopieren:
			swiWaitForVBlank();
			oamUpdate( &oamSub );
		}
		
		int top = this->y_offset;
		this->text_screen.clear( 1 );
		if( this->current_highlight )
		{
			this->current_highlight->render_to( this->text_screen, 
				this->current_highlight_x, this->current_highlight_y+this->y_offset,
				/*replace=*/true );
		}
		for( TextView::iterator line_it = this->begin();
			line_it != this->end() && top<this->text_screen.res_y; 
			line_it++, top+=TextView::LINE_HEIGHT )
		{
			if( top > -TextView::LINE_HEIGHT )
			{
				(*line_it)->render_to( this->text_screen, 0, top );
				(*line_it)->top = top;
				(*line_it)->last_frame_rendered = this->sub_frame_count;
			}
		}
	}
}

void TextView::run_until_exit()
{
	this->config.save_position( this->text.lesson );
	this->render( SCREEN_SUB );
	this->render( SCREEN_MAIN );
	touchPosition old_touch;
    touchRead( &old_touch );
	bool touched = false;
	int pixels_scrolled = 0;
	int old_y_offset = this->y_offset;
	int old_abs_y_diff = 0;
	while( true )
	{
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
		
		NewWordList::iterator test_it = this->current_new_word_list_it;
		test_it++;
		if( pressed & KEY_L && this->current_new_word_list_it != this->current_new_word_list.begin() )
		{
			this->left_button.active = true;
			this->render( SCREEN_SUB );
			this->left_button.active = false;
			this->current_new_word_list_it--;
			this->render( SCREEN_MAIN );
			this->render( SCREEN_SUB );
		}
		else if( pressed & KEY_R && test_it != this->current_new_word_list.end() )
		{
			this->right_button.active = true;
			this->render( SCREEN_SUB );
			this->right_button.active = false;
			this->current_new_word_list_it++;
			this->render( SCREEN_MAIN );
			this->render( SCREEN_SUB );
		}

		touchPosition touch;
        touchRead( &touch );
        if( keysCurrent() & KEY_TOUCH )
        {
			if( !touched ) 
			{
				touched = true;
				pixels_scrolled = 0;
				old_y_offset = this->y_offset;
				old_touch = touch;
			}
			NewWordList::iterator end_it = this->current_new_word_list_it;
			if( end_it != this->current_new_word_list.end() ) end_it++;
            if( this->exit_button.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
            {
				if( !this->exit_button.active )
				{
					this->exit_button.active = true;
					this->render( SCREEN_SUB );
				}
            }
            else if( this->current_new_word_list_it != this->current_new_word_list.begin()
				&& this->left_button.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
            {
				if( !this->left_button.active )
				{
					this->left_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( end_it != this->current_new_word_list.end() 
				&& this->right_button.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
            {
				if( !this->right_button.active  )
				{
					this->right_button.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->hanzi_tab.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
			{
				if( !this->hanzi_tab.active )
				{
					this->hanzi_tab.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->pinyin_tab.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
			{
				if( !this->pinyin_tab.active )
				{
					this->pinyin_tab.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->latin_tab.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
			{
				if( !this->latin_tab.active )
				{
					this->latin_tab.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_easy.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
			{
				if( !this->rating_easy.active )
				{
					this->rating_easy.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_medium.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
			{
				if( !this->rating_medium.active )
				{
					this->rating_medium.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_hard.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
			{
				if( !this->rating_hard.active )
				{
					this->rating_hard.active = true;
					this->render( SCREEN_SUB );
				}
			}
            else if( this->rating_impossible.is_responsible(touch.px, touch.py) 
				&& pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
			{
				if( !this->rating_impossible.active )
				{
					this->rating_impossible.active = true;
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
				int y_diff = touch.py - old_touch.py;
				int abs_y_diff = abs( y_diff );
				if( abs_y_diff && ((old_abs_y_diff && (abs_y_diff <= old_abs_y_diff*TextView::MAX_ACCELERATION_FACTOR)) 
								|| (abs_y_diff <= TextView::MAX_ACCELERATION_FACTOR)) )
				{
					old_abs_y_diff = abs_y_diff;
					pixels_scrolled += abs(y_diff);
					changed = true;
					this->y_offset += y_diff;
					this->v_y = y_diff;
				}
				if( changed ) this->render( SCREEN_SUB, false );
			}
			old_touch = touch;
		}
		else if( touched && pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
		{
			touched = false;
            if( this->exit_button.active && this->exit_button.is_responsible(old_touch.px, old_touch.py) )
            {
				this->exit_button.active = false;
				this->config.save_position( this->text.lesson );
				return;
            }
            else if( this->left_button.active && this->left_button.is_responsible(old_touch.px, old_touch.py) )
            {
				this->left_button.active = false;
				if( this->current_new_word_list_it != this->current_new_word_list.begin() )
				{
					this->current_new_word_list_it--;
					this->render( SCREEN_MAIN );
					this->render( SCREEN_SUB );
				}
			}
            else if( this->right_button.active && this->right_button.is_responsible(old_touch.px, old_touch.py) )
            {
				this->right_button.active = false;
				if( this->current_new_word_list_it != this->current_new_word_list.end() )
				{
					if( ++this->current_new_word_list_it == this->current_new_word_list.end() )
					{
						this->current_new_word_list_it--;
					}
					else
					{
						this->render( SCREEN_MAIN );
						this->render( SCREEN_SUB );
					}
				}
			}
            else if( this->hanzi_tab.is_responsible(old_touch.px, old_touch.py) 
				&& this->hanzi_tab.active )
            {
				this->hanzi_tab.active = false;
				this->toggle_hanzi();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->pinyin_tab.is_responsible(old_touch.px, old_touch.py) 
				&& this->pinyin_tab.active )
            {
				this->pinyin_tab.active = false;
				this->toggle_pinyin();
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
				&& this->rating_easy.active
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
            {
				this->rating_easy.active = false;
				(*this->current_new_word_list_it)->rating = RATING_EASY;
				WordsDB::add_or_write_word( **this->current_new_word_list_it );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_medium.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_medium.active
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
            {
				this->rating_medium.active = false;
				(*this->current_new_word_list_it)->rating = RATING_MEDIUM;
				WordsDB::add_or_write_word( **this->current_new_word_list_it );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_hard.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_hard.active
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
            {
				this->rating_hard.active = false;
				(*this->current_new_word_list_it)->rating = RATING_HARD;
				WordsDB::add_or_write_word( **this->current_new_word_list_it );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
            else if( this->rating_impossible.is_responsible(old_touch.px, old_touch.py) 
				&& this->rating_impossible.active
				&& this->current_new_word_list_it != this->current_new_word_list.end() )
            {
				this->rating_impossible.active = false;
				(*this->current_new_word_list_it)->rating = RATING_IMPOSSIBLE;
				WordsDB::add_or_write_word( **this->current_new_word_list_it );
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
            }
			else
			{
				// Dictionary nach relevanten Treffern durchsuchen:
				bool found = false;
				TextView::iterator prev_line_it = this->end();
				for( TextView::iterator line_it = this->begin();
					!found && line_it != this->end() && (*line_it)->top<this->text_screen.res_y; 
					prev_line_it=line_it++ )
				{
					BufferedLine* line = *line_it;
					if( line->last_frame_rendered==this->sub_frame_count
						&& old_touch.py > line->top  
						&& old_touch.py < line->top+TextView::LINE_HEIGHT  )
					{
						// create and fill tempory search list containing characters of 3 successive lines
						// for multi line context matches:
						UCCharList search_char_list;
						UCCharList::iterator search_char_it = search_char_list.begin();
						typedef std::list<TextView::iterator> SearchLineIterators;
						SearchLineIterators search_line_its;
						if( prev_line_it!=this->end() ) search_line_its.push_back( prev_line_it );
						search_line_its.push_back( line_it );
						TextView::iterator next_line_it = line_it; next_line_it++;
						if( next_line_it!=this->end() ) search_line_its.push_back( next_line_it );
						for( SearchLineIterators::iterator sli = search_line_its.begin();
							sli != search_line_its.end();
							sli++ )
						{
							for( RenderCharList::iterator r_it = (*(*sli))->render_char_list.begin();
								r_it != (*(*sli))->render_char_list.end();
								r_it++ )
							{
								search_char_list.push_back( (*r_it)->uc_char );
								// initiate search character position to first character of current line:
								if( r_it == line->render_char_list.begin() )
								{
									search_char_it = search_char_list.end(); search_char_it--;
								}
							}
						}
						// find character clicked on:
						for( RenderCharList::iterator char_it = line->render_char_list.begin();
							!found && char_it != line->render_char_list.end();
							char_it++, search_char_it++ )
						{
							RenderChar* curr_char = *char_it;
							if( old_touch.px > curr_char->x && old_touch.px < curr_char->x+curr_char->width )
							{
								if( this->context_render_char != curr_char 
									|| this->context_mode != CONTEXT_WORDS_BY_CONTEXT )
								{
									// first click (odd count) on a character: find words in current context:
									this->dict.find_words_by_context( this->text, search_char_list, search_char_it, 6, this->current_new_word_list );
									this->context_mode = CONTEXT_WORDS_BY_CONTEXT;
									// Farbindex 0 der Hintergrundpalette auf orange für's Highlight setzen:
									this->text_screen.palette[0] = 16<<10|24<<5|31;
								}
								else
								{
									// second click (even count) on a character: find words containing selected character:
									this->dict.find_words_by_char_code( curr_char->uc_char.code_point, this->current_new_word_list );
									this->context_mode = CONTEXT_WORDS_BY_CHARCODE;
									// Farbindex 0 der Hintergrundpalette auf blau für's Highlight setzen:
									this->text_screen.palette[0] = 31<<10|24<<5|24;
								}
								this->context_render_char = curr_char;
								this->current_new_word_list_it = this->current_new_word_list.begin();
								// (re-)define and render highlight for selected character in text view:
								if( this->current_highlight ) delete this->current_highlight;
								this->current_highlight = new RenderScreenBuffer( curr_char->width, TextView::LINE_HEIGHT );
								this->current_highlight->clear();
								this->current_highlight_x = curr_char->x;
								this->current_highlight_y = line->top-this->y_offset+curr_char->y-1;
								this->render( SCREEN_SUB );
								// render first search result on main screen:
								this->render( SCREEN_MAIN );
								found = true;
								break;
							}
						}
					}
				}
			}
		}
		else
		{
			swiWaitForVBlank();
			touched = false;
			pixels_scrolled = 0;
			for( TextButtonList::iterator i=this->text_buttons.begin(); 
				i!=this->text_buttons.end(); i++ )
			{
				(*i)->active = false;
			}
			if( this->v_y )
			{
				int resistance = this->v_y / 4;
				if( !resistance ) resistance = this->v_y / 2;
				if( !resistance ) resistance = this->v_y;
				this->v_y -= resistance;
				this->y_offset += this->v_y;
				this->render( SCREEN_SUB, false );
			}
		}
	}
}
