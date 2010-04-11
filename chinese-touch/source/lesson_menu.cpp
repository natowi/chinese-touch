#include <math.h>
#include <sstream>
#include <nds/arm9/image.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/input.h>

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
#include "center_rating_bar.h"
#include "bottom_rating_easy.h"
#include "bottom_rating_medium.h"
#include "bottom_rating_hard.h"
#include "bottom_rating_impossible.h"
#include "small_menu_button.h"
#include "small_menu_button_active.h"
#include "small_menu_button_inactive.h"
#include "bottom_right_button.h"
#include "bottom_right_button_active.h"
#include "settings_dialog.h"
#include "bottom_center_button.h"
#include "bottom_center_button_active.h"


int MenuEntry::BASE_HEIGHT = 32;
int MenuEntry::ACTIVE_HEIGHT = 52;
int MenuEntry::FONT_SIZE = 7;
int MenuEntry::ICON_X_OFFSET = 5;
int MenuEntry::TEXT_X_OFFSET = 50;
int MenuEntry::BUTTON_GAP = 3;
int MenuEntry::BUTTON_Y_OFFSET = MenuEntry::BASE_HEIGHT+2;
int MenuEntry::BUTTON_WIDTH = 32;
int MenuEntry::BUTTON_HEIGHT = 16;
int MenuEntry::SMALL_BUTTON_WIDTH = 16;

int MenuEntry::JUMP_DOWN_BUTTON_X_OFFSET = ICON_X_OFFSET + 0 * SMALL_BUTTON_WIDTH;
int MenuEntry::JUMP_UP_BUTTON_X_OFFSET = ICON_X_OFFSET + 1 * SMALL_BUTTON_WIDTH;
int MenuEntry::NEW_WORDS_BUTTON_X_OFFSET = 0 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::GRAMMAR_BUTTON_X_OFFSET = 1 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::TEXT_BUTTON_X_OFFSET = 2 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::EXERCISES_BUTTON_X_OFFSET = 3 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;

int MenuEntry::RATED_WORDS_BUTTON_X_OFFSET = 4 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::EASY_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*0;
int MenuEntry::MEDIUM_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*1;
int MenuEntry::HARD_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*2;
int MenuEntry::IMPOSSIBLE_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*3;

int MenuEntry::EXPLODE_BUTTON_X_OFFSET = ICON_X_OFFSET;

int LessonMenu::BUTTON_ACTIVATION_SCROLL_LIMIT = 5;
int LessonMenu::MAX_ACCELERATION_FACTOR = 10;

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
		// generic buttons:
		if( x>=MenuEntry::EASY_WORDS_BUTTON_X_OFFSET
			&& x<MenuEntry::EASY_WORDS_BUTTON_X_OFFSET+SMALL_BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_EASY_WORDS;
		}
		if( x>=MenuEntry::MEDIUM_WORDS_BUTTON_X_OFFSET
			&& x<MenuEntry::MEDIUM_WORDS_BUTTON_X_OFFSET+SMALL_BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS;
		}
		if( x>=MenuEntry::HARD_WORDS_BUTTON_X_OFFSET
			&& x<MenuEntry::HARD_WORDS_BUTTON_X_OFFSET+SMALL_BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_HARD_WORDS;
		}
		if( x>=MenuEntry::IMPOSSIBLE_WORDS_BUTTON_X_OFFSET
			&& x<MenuEntry::IMPOSSIBLE_WORDS_BUTTON_X_OFFSET+SMALL_BUTTON_WIDTH )
		{
			return LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS;
		}
		
		if( this->book ) // buttons only applicable to books:
		{
			if( !this->exploded 
				&& x>=MenuEntry::EXPLODE_BUTTON_X_OFFSET
				&& x<MenuEntry::EXPLODE_BUTTON_X_OFFSET+MenuEntry::BUTTON_WIDTH )
			{
				return LessonMenuChoice::CONTENT_TYPE_EXPLODE;
			}
			if( this->exploded
				&& x>=MenuEntry::EXPLODE_BUTTON_X_OFFSET
				&& x<MenuEntry::EXPLODE_BUTTON_X_OFFSET+MenuEntry::BUTTON_WIDTH )
			{
				return LessonMenuChoice::CONTENT_TYPE_IMPLODE;
			}
		}
		else if( this->lesson ) // buttons only applicable to lessons:
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
			if( x>=MenuEntry::JUMP_DOWN_BUTTON_X_OFFSET
				&& x<MenuEntry::JUMP_DOWN_BUTTON_X_OFFSET+MenuEntry::SMALL_BUTTON_WIDTH )
			{
				return LessonMenuChoice::CONTENT_TYPE_JUMP_DOWN;
			}
			if( x>=MenuEntry::JUMP_UP_BUTTON_X_OFFSET
				&& x<MenuEntry::JUMP_UP_BUTTON_X_OFFSET+MenuEntry::SMALL_BUTTON_WIDTH )
			{
				return LessonMenuChoice::CONTENT_TYPE_JUMP_UP;
			}
		}
	}
	return LessonMenuChoice::CONTENT_TYPE_NONE;
}


void DictionarySynchronizer::run_action()
{
	RenderStyle style;
	style.center_x = true;
	int prev_run_count = 0;
	std::string prev_progress;
	for( int run=1; run<=2; run++ )
	{
		int run_count = 0;
		for( Library::iterator book_it = this->library.begin(); book_it != this->library.end(); book_it++ )
		{
			for( Book::iterator lesson_it = book_it->second->begin(); lesson_it != book_it->second->end(); lesson_it++ )
			{
				run_count += lesson_it->second->parse_dictionary_if_needed( /*count_only=*/(run==1) );
				std::stringstream progress;
				progress << ((run==1) ? "scanning" : "syncing") << "\n" 
						<< ((run==1) ? 0 : run_count) << " / "
						<< ((run==1) ? run_count : prev_run_count);
				if( prev_run_count )
				{
					progress << " (" << (run_count*100)/prev_run_count << "%)";
				}
				std::string new_progress = progress.str();
				if( new_progress != prev_progress )
				{
					prev_progress = new_progress;
					info_screen.clear();
					this->freetype_renderer.render( this->info_screen, progress.str(), this->freetype_renderer.latin_face, 14, 0, info_screen.res_y/2-30, &style );
				}
			}
		}
		prev_run_count = run_count;
	}
	this->freetype_renderer.render( this->info_screen, "\n\n\ndone", this->freetype_renderer.latin_face, 14, 0, info_screen.res_y-30, &style );
}


LessonMenu::LessonMenu( FreetypeRenderer& _freetype_renderer, Library& _library, Config& _config )
	: freetype_renderer(_freetype_renderer), library(_library), config(_config), 
		y_offset(5), v_y(0), active_list_id(0), frame_count(0), 
		book_icon(&oamSub,"",SpriteSize_32x32,MenuEntry::ICON_X_OFFSET,0,freetype_renderer.latin_face,9), 
		lesson_icon(&oamSub,"",SpriteSize_32x32,MenuEntry::ICON_X_OFFSET,0,freetype_renderer.latin_face,9),
		new_words_button(&oamSub,"生词",SpriteSize_32x16,MenuEntry::NEW_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1),
		grammar_button(&oamSub,"语法",SpriteSize_32x16,MenuEntry::GRAMMAR_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1,1),
		text_button(&oamSub,"课文",SpriteSize_32x16,MenuEntry::TEXT_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1,1),
		exercises_button(&oamSub,"练习",SpriteSize_32x16,MenuEntry::EXERCISES_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1),
		explode_button(&oamSub,"open",SpriteSize_32x16,MenuEntry::EXPLODE_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,6,1,2),
		implode_button(&oamSub,"close",SpriteSize_32x16,MenuEntry::EXPLODE_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,6,1,2),
		rating_bar(&oamSub,"",SpriteSize_64x32,MenuEntry::RATED_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,7),
		rating_easy(&oamSub,"",SpriteSize_16x16,MenuEntry::EASY_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,7),
		rating_medium(&oamSub,"",SpriteSize_16x16,MenuEntry::MEDIUM_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,7),
		rating_hard(&oamSub,"",SpriteSize_16x16,MenuEntry::HARD_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,7),
		rating_impossible(&oamSub,"",SpriteSize_16x16,MenuEntry::IMPOSSIBLE_WORDS_BUTTON_X_OFFSET,0,freetype_renderer.latin_face,7),
		jump_down_button(&oamSub,"下",SpriteSize_16x16,MenuEntry::JUMP_DOWN_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1,1),
		jump_up_button(&oamSub,"上",SpriteSize_16x16,MenuEntry::JUMP_UP_BUTTON_X_OFFSET,0,freetype_renderer.han_face,9,1,1),
		settings_button(&oamSub,"s",SpriteSize_16x16,menu_screen.res_x-16,menu_screen.res_y-16,freetype_renderer.latin_face,10,1,1),
		search_button(&oamSub,"找",SpriteSize_32x16,menu_screen.res_x-16-80/2-16,menu_screen.res_y-16,freetype_renderer.han_face,8,1,1)
{
	this->freetype_renderer.init_screen( SCREEN_MAIN, this->info_screen );
	//ErrorConsole::init_screen( SCREEN_MAIN );
	dmaCopy( bg_dragonBitmap, this->info_screen.bg_base_address, sizeof(bg_dragonBitmap) );
	set_16bpp_sprite_opague( this->info_screen.bg_base_address, 256, 192 );
	bgShow( this->info_screen.bg_id );
	this->info_screen.clear();
	
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new DictionarySynchronizer("0_synchronize_dictionary", "Synchronize Word Database", "sync",
		this->freetype_renderer, this->library, this->info_screen) );
	
	// store list of interactive buttons in instance:
	this->lesson_buttons.push_back( &this->new_words_button );
	this->lesson_buttons.push_back( &this->grammar_button );
	this->lesson_buttons.push_back( &this->text_button );
	this->lesson_buttons.push_back( &this->exercises_button );
	this->lesson_buttons.push_back( &this->jump_down_button );
	this->lesson_buttons.push_back( &this->jump_up_button );

	this->book_buttons.push_back( &this->explode_button );
	this->book_buttons.push_back( &this->implode_button );
	
	this->text_buttons.insert( this->text_buttons.end(), this->lesson_buttons.begin(), this->lesson_buttons.end() );
	this->text_buttons.insert( this->text_buttons.end(), this->book_buttons.begin(), this->book_buttons.end() );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->search_button );

	this->init_button_list.insert( this->init_button_list.end(), this->text_buttons.begin(), this->text_buttons.end() );
	init_button_list.push_back( &this->book_icon );
	init_button_list.push_back( &this->lesson_icon );
	this->init_subscreen();
	
	// Menü zur gespeicherten Position bewegen:
	std::string config_book_name = this->config.get_current_book_name();
	int config_lesson_number = this->config.get_current_lesson_number();
	if( config_book_name.length() )
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
				if( config_lesson_number )
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
							break;
						}
					}
				} else
				{
					found = true;
					this->active_list_id = static_cast<void*>( book_it->second );
				}
			}
			if( found )
			{
				this->y_offset = _y_offset;
				// Stück zurück und leichten Impuls setzen um das Vorspulen zu verdeutlichen:
				this->y_offset+=20;
				this->v_y=-5;
				/* Menüeintrag für das enthaltende Buch anlegen und ausklappen 
					(den Rest macht die Rendermethode...) */
				MenuEntry* entry = new MenuEntry();
				entry->book = book_it->second;
				if( config_lesson_number ) entry->exploded = true;
				this->menu_list[ static_cast<void*>(entry->book) ] = entry;
				entry->render_text( this->freetype_renderer, entry->book->title );
				break;
			}
		}
	}
	bgHide( this->info_screen.bg_id );
}

void LessonMenu::init_subscreen()
{
	this->freetype_renderer.init_screen( SCREEN_SUB, this->menu_screen );
	this->menu_screen.clear();
	// Farbindex 0 der Hintergrundpalette auf hellblau für's Highlight setzen:
	this->menu_screen.palette[0] = 31<<10|28<<5|28;

	// unteren Bildschirm für Spritenutzung initialisieren:
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	//oamAllocReset( &oamSub );
	oamEnable( &oamSub );

	// vorgerenderte Spritegrafiken laden:
	this->book_icon.init_vram( accessories_dictionaryBitmap, this->book_icon.bg_vram );
	this->book_icon.init_vram( accessories_dictionary_openBitmap, this->book_icon.bg_active_vram );
	this->lesson_icon.init_vram( text_x_genericBitmap, this->lesson_icon.bg_vram );
	this->new_words_button.init_vram( menu_buttonBitmap, this->new_words_button.bg_vram );
	this->new_words_button.init_vram( menu_button_activeBitmap, this->new_words_button.bg_active_vram );
	this->new_words_button.init_vram( menu_button_inactiveBitmap, this->new_words_button.bg_inactive_vram );
	
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

	this->jump_down_button.init_vram( small_menu_buttonBitmap, this->jump_down_button.bg_vram );
	this->jump_down_button.init_vram( small_menu_button_activeBitmap, this->jump_down_button.bg_active_vram );
	this->jump_down_button.init_vram( small_menu_button_inactiveBitmap, this->jump_down_button.bg_inactive_vram );
	this->jump_up_button.bg_vram = this->jump_down_button.bg_vram;
	this->jump_up_button.bg_active_vram = this->jump_down_button.bg_active_vram;
	this->jump_up_button.bg_inactive_vram = this->jump_down_button.bg_inactive_vram;
	this->jump_up_button.owns_bg_vram = false;
	
	this->explode_button.bg_vram = this->new_words_button.bg_vram;
	this->explode_button.bg_active_vram = this->new_words_button.bg_active_vram;
	this->explode_button.bg_inactive_vram = this->new_words_button.bg_inactive_vram;
	this->explode_button.owns_bg_vram = false;
	this->implode_button.bg_vram = this->new_words_button.bg_vram;
	this->implode_button.bg_active_vram = this->new_words_button.bg_active_vram;
	this->implode_button.bg_inactive_vram = this->new_words_button.bg_inactive_vram;
	this->implode_button.owns_bg_vram = false;

	this->rating_bar.init_vram( center_rating_barBitmap, this->rating_bar.bg_vram );
	this->rating_bar.bg_prio = 2; // place bar behind rating emotes
	this->rating_easy.init_vram( bottom_rating_easyBitmap, this->rating_easy.bg_vram );
	this->rating_medium.init_vram( bottom_rating_mediumBitmap, this->rating_medium.bg_vram );
	this->rating_hard.init_vram( bottom_rating_hardBitmap, this->rating_hard.bg_vram );
	this->rating_impossible.init_vram( bottom_rating_impossibleBitmap, this->rating_impossible.bg_vram );
	
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	
	this->search_button.init_vram( bottom_center_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( bottom_center_button_activeBitmap, this->search_button.bg_active_vram );
	
	for( TextButtonList::iterator i=this->init_button_list.begin(); i!=this->init_button_list.end(); i++ )
	{
		(*i)->init_text_layer( this->freetype_renderer );
	}

	// Palette für 8-Bit-Buttonbeschriftungen mit speziell vorbereiteter Palette initialisieren:
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );
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
				// FIXME: query SQL database:
#if 0
				for( Book::iterator lesson_it = entry->book->begin();
					lesson_it != entry->book->end(); lesson_it++ )
				{
					Lesson* lesson = lesson_it->second;
					word_count += lesson->new_words.size();
				}
#endif
				std::stringstream stats_stream;
				stats_stream << "contains " << entry->book->size() << " lessons";
				// FIXME: query SQL database: " and " << word_count << " words";
				stats_text = stats_stream.str();
			}
			else if( entry->lesson )
			{
				title = entry->lesson->title;
				description = entry->lesson->description;
				// FIXME: query SQL database:
#if 0
				std::stringstream stats_stream;
				stats_stream << "contains " << entry->lesson->new_words.size() << " words";
				stats_text = stats_stream.str();
#endif
			}
		}
		else
		{
			author = "Chinese Touch";
			title = "汉语接触";
			publisher = "an extensible language learning tool (v" + std::string(PROGRAM_VERSION) + ")";
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
		
		this->settings_button.render_to( oam_entry );
		this->search_button.render_to( oam_entry );
		
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
			if( top > -MenuEntry::ACTIVE_HEIGHT )
			{
				if( book_entry && book_entry->exploded ) this->book_icon.active = true;
				else this->book_icon.active = false;
				this->book_icon.render_to( oam_entry, this->book_icon.x, top );
				
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
				if( book_id == this->active_list_id )
				{
					int y = top+MenuEntry::BUTTON_Y_OFFSET;
					if( !book_entry->exploded ) this->explode_button.render_to( oam_entry, this->explode_button.x, y );
					else this->implode_button.render_to( oam_entry, this->implode_button.x, y );
					this->rating_bar.render_to( oam_entry, this->rating_bar.x, y );
					if( this->rating_easy.active ) this->rating_easy.render_to( oam_entry, this->rating_easy.x, y );
					if( this->rating_medium.active ) this->rating_medium.render_to( oam_entry, this->rating_medium.x, y );
					if( this->rating_hard.active ) this->rating_hard.render_to( oam_entry, this->rating_hard.x, y );
					if( this->rating_impossible.active ) this->rating_impossible.render_to( oam_entry, this->rating_impossible.x, y );
				}
			}
			if( book_id == this->active_list_id )
			{
				top += MenuEntry::ACTIVE_HEIGHT;
			}
			else top += MenuEntry::BASE_HEIGHT;
			// Inhalte von Büchern, die nicht gerendert wurden, werden ignoriert:
			if( book_entry && book_entry->exploded )
			{
				for( Book::iterator lesson_it = book_it->second->begin();
					lesson_it != book_it->second->end() && top < this->menu_screen.res_y; 
					lesson_it++ )
				{
					void* lesson_id = static_cast<void*>( lesson_it->second );
					MenuEntry* lesson_entry = 0;
					if( this->menu_list.count( lesson_id ) )
					{
						lesson_entry = this->menu_list[ lesson_id ];
					}
					if( top > -MenuEntry::ACTIVE_HEIGHT )
					{
						this->lesson_icon.render_to( oam_entry, this->lesson_icon.x, top );
						
						if( !lesson_entry )
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
						this->new_words_button.disabled = !lesson_entry->lesson->new_words_available;
						this->grammar_button.disabled = !lesson_entry->lesson->grammar_texts_available;
						this->text_button.disabled = !lesson_entry->lesson->lesson_texts_available;
						this->exercises_button.disabled = !lesson_entry->lesson->exercises_available;
						if( top > -MenuEntry::ACTIVE_HEIGHT )
						{
							int y = top+MenuEntry::BUTTON_Y_OFFSET;
							for( TextButtonList::iterator i = this->lesson_buttons.begin();
								i != this->lesson_buttons.end(); i++ )
							{
								(*i)->render_to( oam_entry, (*i)->x, y );
							}
							this->rating_bar.render_to( oam_entry, this->rating_bar.x, y );
							if( this->rating_easy.active ) this->rating_easy.render_to( oam_entry, this->rating_easy.x, y );
							if( this->rating_medium.active ) this->rating_medium.render_to( oam_entry, this->rating_medium.x, y );
							if( this->rating_hard.active ) this->rating_hard.render_to( oam_entry, this->rating_hard.x, y );
							if( this->rating_impossible.active ) this->rating_impossible.render_to( oam_entry, this->rating_impossible.x, y );
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
					highlight_height = MenuEntry::ACTIVE_HEIGHT;
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
	TextButton* activated_button = 0;
	TextButton* prev_activated_button = 0;
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
		touchPosition touch;
		touchRead( &touch );
		if( keysCurrent() & KEY_TOUCH )
		{
			bool changed = false;
			if( !touched ) 
			{
				//std::cout << "touched "; 
				touched = true;
				pixels_scrolled = 0;
				old_y_offset = this->y_offset;
				old_touch = touch;
			}
			if( this->settings_button.is_responsible(touch.px, touch.py) )
			{
				if( !this->settings_button.active )
				{
					this->settings_button.active = true;
					activated_button = &this->settings_button;
					changed = true;
				}
			}
			else if( this->search_button.is_responsible(touch.px, touch.py) )
			{
				if( !this->search_button.active )
				{
					this->search_button.active = true;
					activated_button = &this->search_button;
					changed = true;
				}
			}
			else
			{
				MenuList::iterator entry_it = this->get_entry_by_pos( touch.px, touch.py );
				if( entry_it!=this->menu_list.end() )
				{
					void* entry_id = entry_it->first;
					MenuEntry* entry = entry_it->second;
					if( entry_id == this->active_list_id )
					{
						LessonMenuChoice::ContentType content_type = entry->get_content_type_by_pos(touch.px, touch.py);
						prev_activated_button = activated_button;
						activated_button = this->get_button_by_content_type( content_type );
						if( activated_button!=prev_activated_button )
						{
							//std::cout << "button released/changed ";
							for( TextButtonList::iterator i=this->text_buttons.begin(); 
								i!=this->text_buttons.end(); i++ )
							{
								if( (*i)->active )
								{
									(*i)->active = false;
									changed = true;
								}
							}
							changed = true;
						}
						if( !activated_button->active && this->activate_button_by_content_type(content_type) )
						{
							//std::cout << "button activated ";
							changed = true;
						}
					} else activated_button = 0;
				} else activated_button = 0;
			}
			if( !activated_button )
			{
				int y_diff = touch.py - old_touch.py;
				int abs_y_diff = abs( y_diff );
				if( abs_y_diff && ((old_abs_y_diff && (abs_y_diff <= old_abs_y_diff*LessonMenu::MAX_ACCELERATION_FACTOR)) 
								|| (abs_y_diff <= LessonMenu::MAX_ACCELERATION_FACTOR)) )
				{
					//std::cout << ".";
					old_abs_y_diff = abs_y_diff;
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
		}
		else if( touched && pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
		{
			touched = false;
			if( this->settings_button.active )
			{
				this->settings_button.active = false;
				this->show_settings();
				this->render( SCREEN_MAIN );
				this->render( SCREEN_SUB );
			}
			else if( this->search_button.active )
			{
				this->search_button.active = false;
				choice.content_type = LessonMenuChoice::CONTENT_TYPE_SEARCH;
				choice.book = 0;
				choice.lesson = 0;
				return;
			}
			else
			{
				MenuList::iterator entry_it = this->get_entry_by_pos( old_touch.px, old_touch.py );
				if( entry_it!=this->menu_list.end() )
				{
					void* entry_id = entry_it->first;
					MenuEntry* entry = entry_it->second;
					if( entry_id == this->active_list_id )
					{
						if( entry->book )
						{
							//std::cout << "book ";
							choice.book = entry->book;
							choice.lesson = 0;
							choice.content_type = entry->get_content_type_by_pos( old_touch.px, old_touch.py );
							//std::cout << choice.content_type << " ";
							if( this->get_activation_by_content_type(choice.content_type) )
							{
								//std::cout << "button active ";
								switch( choice.content_type )
								{
									case LessonMenuChoice::CONTENT_TYPE_EXPLODE:
										//std::cout << "explode ";
										entry->exploded = true;
										this->render( SCREEN_SUB );
										break;
									case LessonMenuChoice::CONTENT_TYPE_IMPLODE:
										//std::cout << "implode ";
										entry->exploded = false;
										this->render( SCREEN_SUB );
										break;
									case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
										return;
									default:
										//std::cout << "unknown menu event ";
										break;
								}
							}
						}
						else if( entry->lesson )
						{
							//std::cout << "lesson ";
							choice.book = entry->lesson->book;
							choice.lesson = entry->lesson;
							choice.content_type = entry->get_content_type_by_pos( old_touch.px, old_touch.py );
							//std::cout << choice.content_type << " ";
							if( this->get_activation_by_content_type(choice.content_type) )
							{
								switch( choice.content_type )
								{
									case LessonMenuChoice::CONTENT_TYPE_JUMP_DOWN:
										for( Book::iterator i=choice.book->find(choice.lesson->number); i!=choice.book->end(); i++ )
										{
											void* list_id = static_cast<void*>( i->second );
											MenuList::iterator test_it = this->menu_list.find( list_id );
											if( test_it!=this->menu_list.end() )
											{
												this->active_list_id = list_id;
												test_it->second->top;
												this->y_offset = this->y_offset-test_it->second->top + this->menu_screen.res_y/2;
												this->render( SCREEN_SUB );
											}
										}
										break;
									case LessonMenuChoice::CONTENT_TYPE_JUMP_UP:
										for( Book::iterator i=choice.book->find(choice.lesson->number); ; i-- )
										{
											void* list_id = static_cast<void*>( i->second );
											MenuList::iterator test_it = this->menu_list.find( list_id );
											if( test_it!=this->menu_list.end() )
											{
												this->active_list_id = list_id;
												test_it->second->top;
												this->y_offset = this->y_offset-test_it->second->top + this->menu_screen.res_y/2;
												this->render( SCREEN_SUB );
											}
											if( i==choice.book->begin() ) break;
										}
										break;
									case LessonMenuChoice::CONTENT_TYPE_NEW_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_GRAMMAR:
									case LessonMenuChoice::CONTENT_TYPE_TEXT:
									case LessonMenuChoice::CONTENT_TYPE_EXERCISES:
									case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
									case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
										return;
									default:
										break;
								}
							}
						}
					}
					else
					{
						//std::cout << "entry activated ";
						this->active_list_id = entry_id;
						this->render( SCREEN_SUB );
						this->render( SCREEN_MAIN );
					}
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
			if( entry_it->first==this->active_list_id )
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
		case LessonMenuChoice::CONTENT_TYPE_EXPLODE:
		{
			button = &this->explode_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_JUMP_DOWN:
		{
			button = &this->jump_down_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_JUMP_UP:
		{
			button = &this->jump_up_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_IMPLODE:
		{
			button = &this->implode_button;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
		{
			button = &this->rating_easy;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
		{
			button = &this->rating_medium;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
		{
			button = &this->rating_hard;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
		{
			button = &this->rating_impossible;
			break;
		}
		case LessonMenuChoice::CONTENT_TYPE_SEARCH:
		{
			button = &this->search_button;
			break;
		}
	}
	return button;
}

bool LessonMenu::activate_button_by_content_type( LessonMenuChoice::ContentType content_type )
{
	TextButton* button = this->get_button_by_content_type( content_type );
	if( button && !button->disabled )
	{
		button->active = true;
		return true;
	}
	return false;
}

bool LessonMenu::get_activation_by_content_type( LessonMenuChoice::ContentType content_type )
{
	TextButton* button = this->get_button_by_content_type( content_type );
	return button && !button->disabled && button->active;
}

void LessonMenu::show_settings()
{
	this->init_button_list.free_all();
	SettingsDialog* settings_dialog = new SettingsDialog( this->freetype_renderer, this->settings, "Program Settings" );
	settings_dialog->run_until_exit();
	delete settings_dialog;
	this->init_subscreen();
}
