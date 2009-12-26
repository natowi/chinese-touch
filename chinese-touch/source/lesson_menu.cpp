#include <math.h>
#include <sstream>
#include <nds/arm9/image.h>

#include "chinese-touch.h"
#include "lesson_menu.h"
#include "screen.h"
#include "error_console.h"
// compiled-in sprite data:
#include "text-x-generic.h"
#include "accessories-dictionary.h"
#include "accessories-dictionary-open.h"
#include "menu_button.h"
#include "menu_button_active.h"
#include "menu_button_inactive.h"
#include "greys256.h"
#include "sprite_helper.h"
#include "bg_dragon.h"


int MenuEntry::BASE_HEIGHT = 32;
int MenuEntry::ACTIVE_HEIGHT = 52;
int MenuEntry::FONT_SIZE = 7;
int MenuEntry::TEXT_X_OFFSET = 50;
int MenuEntry::BUTTON_GAP = 6;
int MenuEntry::BUTTON_Y_OFFSET = MenuEntry::BASE_HEIGHT+2;
int MenuEntry::BUTTON_WIDTH = 32;
int MenuEntry::BUTTON_HEIGHT = 16;
int MenuEntry::NEW_WORDS_BUTTON_X_OFFSET = 0 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::GRAMMAR_BUTTON_X_OFFSET = 1 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::TEXT_BUTTON_X_OFFSET = 2 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::EXERCISES_BUTTON_X_OFFSET = 3 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int LessonMenu::BUTTON_ACTIVATION_SCROLL_LIMIT = 5;

void MenuEntry::render_text( FreetypeRenderer& ft, const std::string& text )
{
	RenderStyle render_style;
	//render_style.linebreak = false;
	ft.render( *this->text_surface, text,
		ft.latin_face, MenuEntry::FONT_SIZE, 0, 0, &render_style );
}

MenuList::~MenuList()
{
	for( MenuList::iterator e_it=this->begin(); 
		e_it!=this->end(); e_it++ )
	{
		if( e_it->second )
		{
			delete e_it->second;
			e_it->second=0;
		}
	}
	this->clear();
}

/*! \brief returns content type code for clicked lesson contents button */
LessonMenuChoice::ContentType MenuEntry::get_content_type_by_pos( int x, int y )
{
	if( y>=this->top+MenuEntry::BUTTON_Y_OFFSET 
		&& y<this->top+MenuEntry::BUTTON_Y_OFFSET+MenuEntry::BUTTON_HEIGHT )
	{
		if( x>=MenuEntry::NEW_WORDS_BUTTON_X_OFFSET
			&& x<MenuEntry::NEW_WORDS_BUTTON_X_OFFSET+MenuEntry::BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_NEW_WORDS;
		}
		if( x>=MenuEntry::GRAMMAR_BUTTON_X_OFFSET
			&& x<MenuEntry::GRAMMAR_BUTTON_X_OFFSET+MenuEntry::BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_GRAMMAR;
		}
		if( x>=MenuEntry::TEXT_BUTTON_X_OFFSET
			&& x<MenuEntry::TEXT_BUTTON_X_OFFSET+MenuEntry::BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_TEXT;
		}
		if( x>=MenuEntry::EXERCISES_BUTTON_X_OFFSET
			&& x<MenuEntry::EXERCISES_BUTTON_X_OFFSET+MenuEntry::BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_EXERCISES;
		}
	}
	return LessonMenuChoice::CONTENT_TYPE_NONE;
}


LessonMenu::LessonMenu( FreetypeRenderer& _freetype_renderer, Library& _library, Config& _config )
	: freetype_renderer(_freetype_renderer), library(_library), config(_config), 
		y_offset(5), v_y(0), active_list_id(0), frame_count(0), 
		book_icon(&oamSub,"",32,32,5,0,freetype_renderer.latin_face,9), 
		lesson_icon(&oamSub,"",32,32,5,0,freetype_renderer.latin_face,9),
		new_words_button(&oamSub,"生词",MenuEntry::BUTTON_WIDTH,MenuEntry::BUTTON_HEIGHT,MenuEntry::NEW_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9),
		grammar_button(&oamSub,"语法",MenuEntry::BUTTON_WIDTH,MenuEntry::BUTTON_HEIGHT,MenuEntry::GRAMMAR_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1,1),
		text_button(&oamSub,"课文",MenuEntry::BUTTON_WIDTH,MenuEntry::BUTTON_HEIGHT,MenuEntry::TEXT_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1,1),
		exercises_button(&oamSub,"练习",MenuEntry::BUTTON_WIDTH,MenuEntry::BUTTON_HEIGHT,MenuEntry::EXERCISES_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1)
{
	this->freetype_renderer.init_screen( SCREEN_MAIN, this->info_screen );
	dmaCopy( bg_dragonBitmap, this->info_screen.bg_base_address, sizeof(bg_dragonBitmap) );
	set_16bpp_sprite_opague( this->info_screen.bg_base_address, 256, 192 );
	bgShow( this->info_screen.bg_id );
	this->info_screen.clear();
	this->freetype_renderer.init_screen( SCREEN_SUB, this->menu_screen );
	this->menu_screen.clear();
	// Farbindex 0 der Hintergrundpalette auf hellblau für's Highlight setzen:
	this->menu_screen.palette[0] = 31<<10|28<<5|28;

	// unteren Bildschirm für Spritenutzung initialisieren:
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	oamAllocReset( &oamSub );
	oamEnable( &oamSub );

	// vorgerenderte Spritegrafiken laden:
	this->book_icon.bg_vram = oamAllocateGfx( &oamSub, SpriteSize_32x32, SpriteColorFormat_Bmp );
	dmaCopy( accessories_dictionaryBitmap, this->book_icon.bg_vram, this->book_icon.width * this->book_icon.height *2 );
	this->book_icon.bg_active_vram = oamAllocateGfx( &oamSub, SpriteSize_32x32, SpriteColorFormat_Bmp );
	dmaCopy( accessories_dictionary_openBitmap, this->book_icon.bg_active_vram, this->book_icon.width * this->book_icon.height *2 );
	this->lesson_icon.bg_vram = oamAllocateGfx( &oamSub, SpriteSize_32x32, SpriteColorFormat_Bmp );
	dmaCopy( text_x_genericBitmap, this->lesson_icon.bg_vram, this->lesson_icon.width * this->lesson_icon.height *2 );
	this->new_words_button.bg_vram = oamAllocateGfx( &oamSub, SpriteSize_32x16, SpriteColorFormat_Bmp );
	dmaCopy( menu_buttonBitmap, this->new_words_button.bg_vram, this->new_words_button.width * this->new_words_button.height *2 );
	this->new_words_button.bg_active_vram = oamAllocateGfx( &oamSub, SpriteSize_32x16, SpriteColorFormat_Bmp );
	dmaCopy( menu_button_activeBitmap, this->new_words_button.bg_active_vram, this->new_words_button.width * this->new_words_button.height *2 );
	this->new_words_button.bg_inactive_vram = oamAllocateGfx( &oamSub, SpriteSize_32x16, SpriteColorFormat_Bmp );
	dmaCopy( menu_button_inactiveBitmap, this->new_words_button.bg_inactive_vram, this->new_words_button.width * this->new_words_button.height *2 );
	this->grammar_button.bg_vram = this->new_words_button.bg_vram;
	this->grammar_button.bg_active_vram = this->new_words_button.bg_active_vram;
	this->grammar_button.bg_inactive_vram = this->new_words_button.bg_inactive_vram;
	this->grammar_button.owns_bg_vram = false;
	this->text_button.bg_vram = this->new_words_button.bg_vram;
	this->text_button.bg_active_vram = this->new_words_button.bg_active_vram;
	this->text_button.bg_inactive_vram = this->new_words_button.bg_inactive_vram;
	this->text_button.owns_bg_vram = false;
	this->exercises_button.bg_vram = this->new_words_button.bg_vram;
	this->exercises_button.bg_active_vram = this->new_words_button.bg_active_vram;
	this->exercises_button.bg_inactive_vram = this->new_words_button.bg_inactive_vram;
	this->exercises_button.owns_bg_vram = false;

	// store list of interactive buttons in instance:
	this->text_buttons.push_back( &this->new_words_button );
	this->text_buttons.push_back( &this->grammar_button );
	this->text_buttons.push_back( &this->text_button );
	this->text_buttons.push_back( &this->exercises_button );
	// create temporary superset for initialization:
	TextButtonList init_button_list( this->text_buttons );
	init_button_list.push_back( &this->book_icon );
	init_button_list.push_back( &this->lesson_icon );
	for( TextButtonList::iterator i=init_button_list.begin(); i!=init_button_list.end(); i++ )
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

	// Palette für 8-Bit-Buttonbeschriftungen mit speziell vorbereiteter Palette initialisieren:
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );
	
	// Menü zur gespeicherten Position bewegen:
	std::string config_book_name = this->config.get_current_book_name();
	int config_lesson_number = this->config.get_current_lesson_number();
	if( config_book_name.length() && config_lesson_number )
	{
		bool found = false;
		// tatsächlichen Wert nur verändern, wenn wir auch was finden:
		int _y_offset = 0;
		for( Library::iterator book_it=this->library.begin();
			book_it!=this->library.end(); 
			book_it++, _y_offset-=MenuEntry::BASE_HEIGHT )
		{
			// geht davon aus, dass nur das selektierte Buch ausgeklappt ist:
			if( book_it->first == config_book_name )
			{
				for( Book::iterator lesson_it=book_it->second->begin();
					lesson_it!=book_it->second->end(); 
					lesson_it++, _y_offset-=MenuEntry::BASE_HEIGHT )
				{
					if( lesson_it->first == config_lesson_number )
					{
						_y_offset -= MenuEntry::BASE_HEIGHT;
						found = true;
						this->active_list_id = static_cast<void*>( lesson_it->second );
						this->y_offset = _y_offset;
						// Stück zurück und leichten Impuls setzen um das Vorspulen zu verdeutlichen:
						this->y_offset+=20;
						this->v_y=-5;
						break;
					}
				}
		}
			if( found )
			{
				/* Menüeintrag für das enthaltende Buch anlegen und ausklappen 
					(den Rest macht die Rendermethode...) */
				MenuEntry* entry = new MenuEntry();
				entry->book = book_it->second;
				entry->exploded = true;
				this->menu_list[ static_cast<void*>(entry->book) ] = entry;
				entry->render_text( this->freetype_renderer, entry->book->title );
				break;
			}
		}
	}
	bgHide( this->info_screen.bg_id );
}

LessonMenu::~LessonMenu()
{
}

void LessonMenu::render( Screen screen )
{
	this->frame_count++;
	if( screen == SCREEN_MAIN )
	{
		this->info_screen.clear();
		std::string author, title, publisher, description, stats_text;
		if( this->menu_list.count(this->active_list_id) )
		{
			MenuEntry* entry = this->menu_list[ this->active_list_id ];
			if( entry->book )
			{
				author = entry->book->author;
				title = entry->book->title;
				publisher = entry->book->publisher;
				description = entry->book->description;
				int word_count = 0;
				for( Book::iterator lesson_it = entry->book->begin();
					lesson_it != entry->book->end(); lesson_it++ )
				{
					Lesson* lesson = lesson_it->second;
					word_count += lesson->new_words.size();
				}
				std::stringstream stats_stream;
				stats_stream << "contains " << entry->book->size() << " lessons and " << word_count << " words";
				stats_text = stats_stream.str();
			}
			else if( entry->lesson )
			{
				title = entry->lesson->title;
				description = entry->lesson->description;
				std::stringstream stats_stream;
				stats_stream << "contains " << entry->lesson->new_words.size() << " words";
				stats_text = stats_stream.str();
			}
		}
		else
		{
			author = "Chinese Touch";
			title = "汉语接触";
			publisher = "an extensible language learning tool (v1.0)";
			description = "Lesson Menu: Please select a book or lesson on the touchscreen!";
			std::stringstream stats_stream;
			int lesson_count = 0;
			for( Library::iterator book_it = this->library.begin();
				book_it != this->library.end(); book_it++ )
			{
				lesson_count += book_it->second->size();
			}
			stats_stream << "" << lesson_count << " lessons from " << this->library.size() << " books loaded." ;
			stats_text = stats_stream.str();
		}
		int top = 0;
		RenderInfo rect(0,0,0,0);
		top += 3;
		if( author.length() )
		{
			RenderStyle render_style;
			render_style.center_x = true;
			rect = this->freetype_renderer.render( this->info_screen, author, 
				this->freetype_renderer.latin_face, 10, 0, top, &render_style );
			top += rect.height+15;
		}
		if( title.length() )
		{
			RenderStyle render_style;
			render_style.center_x = true;
			rect = this->freetype_renderer.render( this->info_screen, title, 
				this->freetype_renderer.latin_face, 14, 0, top, &render_style );
			top += rect.height+15;
		} else top += 24+15;
		if( publisher.length() )
		{
			RenderStyle render_style;
			render_style.center_x = true;
			rect = this->freetype_renderer.render( this->info_screen, publisher, 
				this->freetype_renderer.latin_face, 7, 0, top, &render_style );
			top += rect.height+15;
		}
		if( description.length() )
		{
			RenderStyle render_style;
			rect = this->freetype_renderer.render( this->info_screen, description, 
				this->freetype_renderer.latin_face, 8, 3, top, &render_style );
			top += rect.height+15;
		}
		if( stats_text.length() )
		{
			RenderStyle render_style;
			rect = this->freetype_renderer.render( this->info_screen, stats_text, 
				this->freetype_renderer.latin_face, 7, 3, top, &render_style );
			top += rect.height+15;
		}
	}
	else if( screen == SCREEN_SUB )
	{
		oamClear( &oamSub, 0, 0 );
		int top = this->y_offset;
		int oam_entry = 0;
		for( Library::iterator book_it = this->library.begin(); 
			book_it != this->library.end() && top < this->menu_screen.res_y; 
			book_it++ )
		{
			void* book_id = static_cast<void*>( book_it->second );
			MenuEntry* book_entry = 0;
			// book_entry holen, wenn bereits verfügbar, was bei initial ausgeklappten Büchern
			// der Fall sein sollte, wenn der Konstruktor sie bereits vorgerendert hat:
			if( this->menu_list.count( book_id ) )
			{
				book_entry = this->menu_list[ book_id ];
			}
			if( top > -MenuEntry::BASE_HEIGHT )
			{
				if( book_entry && book_entry->exploded )
				{
					oamSet( this->book_icon.oam, oam_entry++, this->book_icon.x, top, /*prio=*/0, /*alpha=*/15, 
							SpriteSize_32x32, SpriteColorFormat_Bmp, this->book_icon.bg_active_vram, 0, 0, 0, 0, 0, 0 );
				}
				else
				{
					oamSet( this->book_icon.oam, oam_entry++, this->book_icon.x, top, /*prio=*/0, /*alpha=*/15, 
							SpriteSize_32x32, SpriteColorFormat_Bmp, this->book_icon.bg_vram, 0, 0, 0, 0, 0, 0 );
				}
				// book_entry anlegen, falls nicht schon früher geschehen,
				// da wir ihn nun wirklich brauchen:
				if( !book_entry )
				{
					book_entry = new MenuEntry();
					book_entry->book = book_it->second;
					this->menu_list[ book_id ] = book_entry;
					book_entry->render_text( this->freetype_renderer, book_it->second->title );
				}
				book_entry->top = top;
				book_entry->last_frame_rendered = this->frame_count;
			}
			top += MenuEntry::BASE_HEIGHT;
			// Inhalte von Büchern, die nicht gerendert wurden, werden ignoriert:
			if( book_entry && book_entry->exploded )
			{
				for( Book::iterator lesson_it = book_it->second->begin();
					lesson_it != book_it->second->end() && top < this->menu_screen.res_y; 
					lesson_it++ )
				{
					void* lesson_id = static_cast<void*>( lesson_it->second );
					MenuEntry* lesson_entry = 0;
					if( top > -MenuEntry::ACTIVE_HEIGHT )
					{
						oamSet( this->lesson_icon.oam, oam_entry++, this->lesson_icon.x, top, /*prio=*/0, /*alpha=*/15, 
								SpriteSize_32x32, SpriteColorFormat_Bmp, this->lesson_icon.bg_vram, 0, 0, 0, 0, 0, 0 );
						if( this->menu_list.count( lesson_id ) )
						{
							lesson_entry = this->menu_list[ lesson_id ];
						}
						else
						{
							lesson_entry = new MenuEntry();
							lesson_entry->lesson = lesson_it->second;
							this->menu_list[ lesson_id ] = lesson_entry;
							lesson_entry->render_text( this->freetype_renderer, lesson_it->second->title );
						}
						lesson_entry->top = top;
						lesson_entry->last_frame_rendered = this->frame_count;
					}
					if( lesson_entry && lesson_entry->lesson && (lesson_id == this->active_list_id) )
					{
						this->new_words_button.inactive = lesson_entry->lesson->new_words.empty();
						this->grammar_button.inactive = lesson_entry->lesson->grammar_texts.empty();
						this->text_button.inactive = lesson_entry->lesson->lesson_texts.empty();
						this->exercises_button.inactive = lesson_entry->lesson->exercises.empty();
						if( top > -MenuEntry::ACTIVE_HEIGHT )
						{
							for( TextButtonList::iterator i = this->text_buttons.begin();
								i != this->text_buttons.end(); i++ )
							{
								oamSet( (*i)->oam, oam_entry++,
										(*i)->x, top+MenuEntry::BUTTON_Y_OFFSET, 	// position
										/*prio=*/1, /*alpha=*/1, SpriteSize_32x16, SpriteColorFormat_Bmp, 
										(*i)->inactive ? (*i)->bg_inactive_vram :
											( (*i)->active ? (*i)->bg_active_vram-64 : (*i)->bg_vram ),
										0, 0, 0, 0, 0, 0 );
								oamSet( (*i)->oam, oam_entry++,
										(*i)->x+(*i)->text_x_offset, top+MenuEntry::BUTTON_Y_OFFSET+(*i)->text_y_offset, 	// position
										/*prio=*/0, /*alpha=*/0, SpriteSize_32x16, SpriteColorFormat_256Color, 
										(*i)->text_vram, 0, 0, 0, 0, 0, 0 );
							}
						}
						top += MenuEntry::ACTIVE_HEIGHT;
					}
					else top += MenuEntry::BASE_HEIGHT;
				}
			}
		}
		// gepufferte Bilddaten einblenden bzw. in den VRAM kopieren:
		swiWaitForVBlank();
		oamUpdate( &oamSub );
		this->menu_screen.clear( 1 );
		for( MenuList::iterator entry_it = this->menu_list.begin();
			entry_it != this->menu_list.end(); entry_it++ )
		{
			MenuEntry* entry = entry_it->second;
			if( entry->last_frame_rendered == this->frame_count )
			{
				if( entry_it->first == this->active_list_id )
				{
					int highlight_height=2;
					if( entry->book ) highlight_height = MenuEntry::BASE_HEIGHT;
					else if( entry->lesson ) highlight_height = MenuEntry::ACTIVE_HEIGHT;
					int render_top = entry->top;
					if( render_top<0 )
					{
						highlight_height += render_top;
						render_top = 0;
					}
					if( highlight_height >= 2 )
					{
						memset( this->menu_screen.base_address+this->menu_screen.res_x*(render_top+1)/2, 
								0, 
								this->menu_screen.res_x*(highlight_height-2) );
					}
					if( entry->top == render_top )
					{
						memset( this->menu_screen.base_address+this->menu_screen.res_x*(render_top)/2, 
								64, 
								this->menu_screen.res_x );
					}
					if( highlight_height >= 1 )
					{
						memset( this->menu_screen.base_address+this->menu_screen.res_x*(render_top+highlight_height-1)/2, 
								64, 
								this->menu_screen.res_x );
					}
				}
				entry->text_surface->render_to( this->menu_screen, MenuEntry::TEXT_X_OFFSET, entry->top );
			}
		}
	}
}

void LessonMenu::run_for_user_choice( LessonMenuChoice& choice )
{
	this->render( SCREEN_MAIN );
	this->render( SCREEN_SUB );
	touchPosition old_touch;
    touchRead( &old_touch );
	bool touched = false;
	int pixels_scrolled = 0;
	int old_y_offset = this->y_offset;
	bool button_activated_previously = false;
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
        touchPosition touch;
        touchRead( &touch );
        int area = touch.px * touch.z2 / touch.z1 - touch.px;
        if( keysCurrent() & KEY_TOUCH )
        {
			if( !touched ) 
			{
				touched = true;
				pixels_scrolled = 0;
				old_y_offset = this->y_offset;
				old_touch = touch;
			}
			bool button_activated = false;
			MenuList::iterator entry_it = this->get_entry_by_pos( touch.px, touch.py );
			if( entry_it!=this->menu_list.end() )
			{
				void* entry_id = entry_it->first;
				MenuEntry* entry = entry_it->second;
				if( entry_id == this->active_list_id )
				{
					if( entry->lesson )
					{
						button_activated = this->activate_button_by_content_type( 
								entry->get_content_type_by_pos(old_touch.px, old_touch.py) );
					}
				}
			}
			bool changed = false;
			if( button_activated && !button_activated_previously )
			{
				changed = true;
			}
			else if( !button_activated )
			{
				if( button_activated_previously )
				{
					for( TextButtonList::iterator i=this->text_buttons.begin(); 
						i!=this->text_buttons.end(); i++ )
					{
						if( (*i)->active )
						{
							(*i)->active = false;
							changed = true;
						}
					}
				}
				int y_diff = touch.py - old_touch.py;
				if( y_diff )
				{
					pixels_scrolled += abs(y_diff);
					this->y_offset += y_diff;
					this->v_y = y_diff;
					changed = true;
				}
			}
			if( changed )
			{
				this->render( SCREEN_SUB );
			}
			old_touch = touch;
			button_activated_previously = button_activated;
		}
		else if( touched && pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
		{
			touched = false;
			MenuList::iterator entry_it = this->get_entry_by_pos( old_touch.px, old_touch.py );
			if( entry_it!=this->menu_list.end() )
			{
				void* entry_id = entry_it->first;
				MenuEntry* entry = entry_it->second;
				if( entry->book )
				{
					entry->exploded = !entry->exploded;
					this->render( SCREEN_SUB );
				}
				if( entry_id == this->active_list_id )
				{
					if( entry->lesson )
					{
						choice.book = entry->lesson->book;
						choice.lesson = entry->lesson;
						choice.content_type = entry->get_content_type_by_pos( old_touch.px, old_touch.py );
						if( this->get_activation_by_content_type(choice.content_type) )
						{
							return;
						}
					}
				}
				else
				{
					this->active_list_id = entry_id;
					this->render( SCREEN_SUB );
					this->render( SCREEN_MAIN );
				}
			}
		}
        else
        {
			swiWaitForVBlank();
			touched = false;
			pixels_scrolled = 0;
			bool changed = false;
			for( TextButtonList::iterator i=this->text_buttons.begin(); 
				i!=this->text_buttons.end(); i++ )
			{
				if( (*i)->active )
				{
					(*i)->active = false;
					changed = true;
				}
			}
			if( this->v_y )
			{
				int resistance = this->v_y / 4;
				if( !resistance ) resistance = this->v_y / 2;
				if( !resistance ) resistance = this->v_y;
				this->v_y -= resistance;
				this->y_offset += this->v_y;
				changed = true;
			}
			if( changed ) this->render( SCREEN_SUB );
        }
	}
}

MenuList::iterator LessonMenu::get_entry_by_pos( int x, int y )
{
	LOG( "get_entry_by_pos(" << x << "," << y << ")" );
	for( MenuList::iterator entry_it = this->menu_list.begin();
		entry_it != this->menu_list.end(); 
		entry_it++ )
	{
		MenuEntry* entry = entry_it->second;
		if( entry->last_frame_rendered==this->frame_count )
		{
			int next_top;
			if( entry->lesson && entry_it->first==this->active_list_id )
				next_top = entry->top + MenuEntry::ACTIVE_HEIGHT;
			else 
				next_top = entry->top + MenuEntry::BASE_HEIGHT;
			if( y >= entry->top && y < next_top )
			{
				return entry_it;
			}
		}
	}
	return this->menu_list.end();
}

TextButton* LessonMenu::get_button_by_content_type( LessonMenuChoice::ContentType content_type )
{
	TextButton* button = 0;
	switch( content_type )
	{
		case LessonMenuChoice::CONTENT_TYPE_NONE:
		{
			return 0;
		}
		case LessonMenuChoice::CONTENT_TYPE_NEW_WORDS:
		{
			button = &this->new_words_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_GRAMMAR:
		{
			button = &this->grammar_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_TEXT:
		{
			button = &this->text_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_EXERCISES:
		{
			button = &this->exercises_button;
			break;
		}
	}
	return button;
}

bool LessonMenu::activate_button_by_content_type( LessonMenuChoice::ContentType content_type )
{
	TextButton* button = this->get_button_by_content_type( content_type );
	if( button && !button->inactive )
	{
		button->active = true;
		return true;
	}
	return false;
}

bool LessonMenu::get_activation_by_content_type( LessonMenuChoice::ContentType content_type )
{
	TextButton* button = this->get_button_by_content_type( content_type );
	return button && !button->inactive && button->active;
}