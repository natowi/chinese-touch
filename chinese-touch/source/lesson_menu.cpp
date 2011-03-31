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
#include "center_rating_bar.h"
#include "center_rating_bar_disabled.h"
#include "center_rating_bar_any_extension.h"
#include "bottom_rating_easy.h"
#include "bottom_rating_easy_active.h"
#include "bottom_rating_easy_disabled.h"
#include "bottom_rating_medium.h"
#include "bottom_rating_medium_active.h"
#include "bottom_rating_medium_disabled.h"
#include "bottom_rating_hard.h"
#include "bottom_rating_hard_active.h"
#include "bottom_rating_hard_disabled.h"
#include "bottom_rating_impossible.h"
#include "bottom_rating_impossible_active.h"
#include "bottom_rating_impossible_disabled.h"
#include "bottom_rating_any.h"
#include "bottom_rating_any_active.h"
#include "small_menu_button.h"
#include "small_menu_button_active.h"
#include "small_menu_button_inactive.h"
#include "bottom_right_button.h"
#include "bottom_right_button_active.h"
#include "settings_dialog.h"
#include "bottom_center_button.h"
#include "bottom_center_button_active.h"
#include "fulltext_search.h"
#include "bottom_rating_bar.h"
#include "bottom_rating_bar_any_extension.h"
#include "tiny_lesson.h"
#include "tiny_lessons.h"
#include "tiny_dice.h"
#include "tiny_clock.h"
#include "loading.h"
#include "tiny_search.h"
#include "tiny_book.h"
#include "tiny_text.h"


int MenuEntry::BASE_HEIGHT = 32;
int MenuEntry::ACTIVE_HEIGHT = 52;
int MenuEntry::FONT_SIZE = 7;
int MenuEntry::ICON_X_OFFSET = 5;
int MenuEntry::TEXT_X_OFFSET = 50;
int MenuEntry::BUTTON_GAP = 3;
int MenuEntry::BUTTON_Y_OFFSET = MenuEntry::BASE_HEIGHT/2-1;
int MenuEntry::BUTTON_ACTIVE_Y_OFFSET = MenuEntry::BASE_HEIGHT+2;
int MenuEntry::BUTTON_WIDTH = 32;
int MenuEntry::BUTTON_HEIGHT = 16;
int MenuEntry::SMALL_BUTTON_WIDTH = 16;

int MenuEntry::JUMP_DOWN_BUTTON_X_OFFSET = ICON_X_OFFSET + 0 * SMALL_BUTTON_WIDTH;
int MenuEntry::JUMP_UP_BUTTON_X_OFFSET = ICON_X_OFFSET + 1 * SMALL_BUTTON_WIDTH;
int MenuEntry::BUTTON_X_OFFSET = MenuEntry::TEXT_X_OFFSET;

int MenuEntry::RATED_WORDS_BUTTON_X_OFFSET = 4 * (MenuEntry::BUTTON_WIDTH+BUTTON_GAP) + MenuEntry::TEXT_X_OFFSET;
int MenuEntry::LESSON_RANGE_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET - MenuEntry::BUTTON_GAP;
int MenuEntry::ORDER_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + 32;
int MenuEntry::EASY_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*0;
int MenuEntry::MEDIUM_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*1;
int MenuEntry::HARD_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*2;
int MenuEntry::IMPOSSIBLE_WORDS_BUTTON_X_OFFSET = RATED_WORDS_BUTTON_X_OFFSET + SMALL_BUTTON_WIDTH*3;

int MenuEntry::EXPLODE_BUTTON_X_OFFSET = ICON_X_OFFSET;

int LessonMenu::BUTTON_ACTIVATION_SCROLL_LIMIT = 5;
int LessonMenu::MAX_ACCELERATION_FACTOR = 10;


std::string LessonMenuChoice::get_enum_text( LessonMenuChoice::ContentType content_type )
{
	std::string text;
	switch( content_type )
	{
		case LessonMenuChoice::CONTENT_TYPE_NONE: text = "none"; break;
		case LessonMenuChoice::CONTENT_TYPE_GRAMMAR: text = "grammar"; break;
		case LessonMenuChoice::CONTENT_TYPE_TEXT: text = "text"; break;
		case LessonMenuChoice::CONTENT_TYPE_EXERCISES: text = "exercises"; break;
		case LessonMenuChoice::CONTENT_TYPE_ANY_WORDS: text = "any_words"; break;
		case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS: text = "easy_words"; break;
		case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS: text = "medium_words"; break;
		case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS: text = "hard_words"; break;
		case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS: text = "impossible_words"; break;
		case LessonMenuChoice::CONTENT_TYPE_SEARCH: text = "search"; break;
	}
	return text;
}

std::string LessonMenuChoice::get_enum_text( LessonMenuChoice::ContentRange content_range )
{
	std::string text;
	switch( content_range )
	{
		case LessonMenuChoice::CONTENT_RANGE_LESSON: text = "lesson"; break;
		case LessonMenuChoice::CONTENT_RANGE_BOOK: text = "book"; break;
	}
	return text;
}

std::string LessonMenuChoice::get_enum_text( LessonMenuChoice::ContentOrder content_order)
{
	std::string text;
	switch( content_order )
	{
		case LessonMenuChoice::CONTENT_ORDER_INDEX: text = "index"; break;
		case LessonMenuChoice::CONTENT_ORDER_LATENCY: text = "latency"; break;
		case LessonMenuChoice::CONTENT_ORDER_RANDOM: text = "random"; break;
	}
	return text;
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
		for( Library::iterator book_it = this->program.library->begin(); book_it != this->program.library->end(); book_it++ )
		{
			for( Book::iterator lesson_it = book_it->second->begin(); lesson_it != book_it->second->end(); lesson_it++ )
			{
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
					this->program.ft->render( this->info_screen, progress.str(), this->program.ft->latin_face, 14, 0, info_screen.res_y/2-30, &style );
				}
				run_count += lesson_it->second->parse_dictionary_if_needed( /*count_only=*/(run==1), /*force_update=*/this->full_sync );
			}
		}
		prev_run_count = run_count;
	}
	this->program.ft->render( this->info_screen, "\n\n\ndone", this->program.ft->latin_face, 14, 0, info_screen.res_y-30, &style );
}


void IndexRebuilder::run_action()
{
	RenderStyle style;
	style.center_x = true;
	std::string prev_progress;
	int word_count = this->program.words_db->count_words();
	int max_word_id = this->program.words_db->get_max_word_id();
	int query_step = 20;
	int run_count = 0;
	this->program.words_db->clear_fulltext_patterns();
	for( int current_id = 1; current_id<=max_word_id+query_step /* hack to ensure progress being updated to 100%*/; 
			current_id+=query_step )
	{
		std::stringstream progress;
		progress << "indexing" << "\n" << run_count << " / " << word_count;
		progress << " (" << (run_count*100)/word_count << "%)";
		std::string new_progress = progress.str();
		if( new_progress != prev_progress )
		{
			prev_progress = new_progress;
			info_screen.clear();
			this->program.ft->render( this->info_screen, progress.str(), this->program.ft->latin_face, 14, 0, info_screen.res_y/2-30, &style );
		}
		// hack to ensure progress being updated to 100%:
		if( current_id>=max_word_id ) break;
		
		NewWordList* result = new NewWordList();
		std::stringstream condition;
		condition << "words.id>=" << current_id << " and words.id<" << current_id+query_step;
		this->program.words_db->query_words( *this->program.library, condition.str(), *result );
		for( NewWordList::iterator word_it=result->begin(); word_it!=result->end(); word_it++ )
		{
			StringSet* search_patterns = new StringSet();
			Lesson::get_patterns_from_text( (*word_it)->pinyin, *search_patterns );
			for( Definitions::iterator def_it=(*word_it)->definitions.begin(); def_it!=(*word_it)->definitions.end(); def_it++ )
			{
				Lesson::get_patterns_from_text( def_it->second->translation, *search_patterns );
				Lesson::get_patterns_from_text( def_it->second->comment, *search_patterns );
			}
			this->program.words_db->add_fulltext_patterns( **word_it, *search_patterns );
			delete search_patterns;
		}
		run_count += result->size();
		delete result;
	}
	this->program.ft->render( this->info_screen, "\n\n\ndone", this->program.ft->latin_face, 14, 0, info_screen.res_y-30, &style );
}


LessonMenu::LessonMenu( Program& _program, int _recursion_depth, LessonMenuChoice& _choice )
	: Mode(_program, _recursion_depth), choice(_choice), 
		info_screen(SCREEN_MAIN), menu_screen(SCREEN_SUB),
		y_offset(5), v_y(0), active_list_id(0), sub_frame_count(0), 
		book_icon(menu_screen,"",SpriteSize_32x32,MenuEntry::ICON_X_OFFSET,0,program.ft->latin_face,9), 
		lesson_icon(menu_screen,"",SpriteSize_32x32,MenuEntry::ICON_X_OFFSET,0,program.ft->latin_face,9),
		grammar_button(menu_screen,"§",SpriteSize_32x16,0,0,program.ft->latin_face,11,0,-1),
		text_button(menu_screen,"",SpriteSize_32x16,0,0,program.ft->han_face,9,1,1),
		exercises_button(menu_screen,"",SpriteSize_32x16,0,0,program.ft->han_face,9,1),
		explode_button(menu_screen,"open",SpriteSize_32x16,MenuEntry::EXPLODE_BUTTON_X_OFFSET,0,program.ft->latin_face,6,1,2),
		implode_button(menu_screen,"close",SpriteSize_32x16,MenuEntry::EXPLODE_BUTTON_X_OFFSET,0,program.ft->latin_face,6,1,2),
		lesson_range_button(menu_screen,"",SpriteSize_32x16,MenuEntry::LESSON_RANGE_BUTTON_X_OFFSET,0,program.ft->han_face,9,1),
		book_range_button(menu_screen,"",SpriteSize_32x16,MenuEntry::LESSON_RANGE_BUTTON_X_OFFSET,0,program.ft->han_face,9,1),
		order_index_button(menu_screen,"#",SpriteSize_32x16,MenuEntry::ORDER_BUTTON_X_OFFSET,0,program.ft->han_face,9,1,2),
		order_latency_button(menu_screen,"",SpriteSize_32x16,MenuEntry::ORDER_BUTTON_X_OFFSET,0,program.ft->han_face,9,1),
		order_random_button(menu_screen,"",SpriteSize_32x16,MenuEntry::ORDER_BUTTON_X_OFFSET,0,program.ft->han_face,9,1),
		rating_bar_any_extension(menu_screen,"",SpriteSize_32x16,MenuEntry::RATED_WORDS_BUTTON_X_OFFSET-16,0,program.ft->latin_face,7),
		rating_bar(menu_screen,"",SpriteSize_64x32,MenuEntry::RATED_WORDS_BUTTON_X_OFFSET,0,program.ft->latin_face,7),
		rating_any(menu_screen,"",SpriteSize_16x16,MenuEntry::EASY_WORDS_BUTTON_X_OFFSET-16,0,program.ft->latin_face,7),
		rating_easy(menu_screen,"",SpriteSize_16x16,MenuEntry::EASY_WORDS_BUTTON_X_OFFSET,0,program.ft->latin_face,7),
		rating_medium(menu_screen,"",SpriteSize_16x16,MenuEntry::MEDIUM_WORDS_BUTTON_X_OFFSET,0,program.ft->latin_face,7),
		rating_hard(menu_screen,"",SpriteSize_16x16,MenuEntry::HARD_WORDS_BUTTON_X_OFFSET,0,program.ft->latin_face,7),
		rating_impossible(menu_screen,"",SpriteSize_16x16,MenuEntry::IMPOSSIBLE_WORDS_BUTTON_X_OFFSET,0,program.ft->latin_face,7),
		global_rating_bar_any_extension(menu_screen,"",SpriteSize_32x16,menu_screen.res_x/2-40,menu_screen.res_y-16,program.ft->latin_face,7),
		global_rating_bar(menu_screen,"",SpriteSize_64x32,menu_screen.res_x/2-24,menu_screen.res_y-16,program.ft->latin_face,7,0,0),
		global_rating_any(menu_screen,"",SpriteSize_16x16,menu_screen.res_x/2-40,menu_screen.res_y-16,program.ft->latin_face,7,0,0),
		global_rating_easy(menu_screen,"",SpriteSize_16x16,menu_screen.res_x/2-24,menu_screen.res_y-16,program.ft->latin_face,7,0,0),
		global_rating_medium(menu_screen,"",SpriteSize_16x16,menu_screen.res_x/2-8,menu_screen.res_y-16,program.ft->latin_face,7,0,0),
		global_rating_hard(menu_screen,"",SpriteSize_16x16,menu_screen.res_x/2+8,menu_screen.res_y-16,program.ft->latin_face,7,0,0),
		global_rating_impossible(menu_screen,"",SpriteSize_16x16,menu_screen.res_x/2+24,menu_screen.res_y-16,program.ft->latin_face,7,0,0),
		jump_down_button(menu_screen,"下",SpriteSize_16x16,MenuEntry::JUMP_DOWN_BUTTON_X_OFFSET,0,program.ft->han_face,9,1,1),
		jump_up_button(menu_screen,"上",SpriteSize_16x16,MenuEntry::JUMP_UP_BUTTON_X_OFFSET,0,program.ft->han_face,9,1,1),
		settings_button(menu_screen,"s",SpriteSize_16x16,menu_screen.res_x-16,menu_screen.res_y-16,program.ft->latin_face,10,1,1),
		search_button(menu_screen,"",SpriteSize_32x16,40,menu_screen.res_y-16,program.ft->han_face,9,0,1),
		loading_symbol(menu_screen,"",SpriteSize_32x32,menu_screen.res_x/2-16,menu_screen.res_y/2-16,program.ft->han_face,14,0,1),
		old_y_offset(0), old_abs_y_diff(0), pixels_scrolled(0)
{
	this->init_mode();
	
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new DictionarySynchronizer("0_quick_sync", "Quick-Synchronize Word Database", "qsyn",
		this->program, this->info_screen) );
	this->settings.add_setting( new DictionarySynchronizer("1_full_sync", "Fully Synchronize Word Database (Slow!)", "fsyn",
		this->program, this->info_screen, /*full_sync=*/true) );
	this->settings.add_setting( new IndexRebuilder("2_recreate_index", "(Re-)Create Full-Text Index (Very Slow!)", "idx",
		this->program, this->info_screen) );
	
	// store list of reference buttons to be initialized:
	this->text_buttons.push_back( &this->grammar_button );
	this->text_buttons.push_back( &this->text_button );
	this->text_buttons.push_back( &this->exercises_button );
	this->text_buttons.push_back( &this->jump_down_button );
	this->text_buttons.push_back( &this->jump_up_button );
	this->text_buttons.push_back( &this->explode_button );
	this->text_buttons.push_back( &this->implode_button );
	this->text_buttons.push_back( &this->lesson_range_button );
	this->text_buttons.push_back( &this->book_range_button );
	this->text_buttons.push_back( &this->order_index_button );
	this->text_buttons.push_back( &this->order_latency_button );
	this->text_buttons.push_back( &this->order_random_button );
	this->text_buttons.push_back( &this->rating_bar_any_extension );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_any );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );
	this->text_buttons.push_back( &this->loading_symbol );
	for( TextButtonList::iterator tbi=this->text_buttons.begin();
		tbi!=this->text_buttons.end(); tbi++ )
	{
		// reference buttons are disabled and hidden
		(*tbi)->disabled = (*tbi)->hidden = true;
	}
	// global lesson menu buttons:
	this->text_buttons.push_back( &this->global_rating_bar_any_extension );
	this->text_buttons.push_back( &this->global_rating_bar );
	this->text_buttons.push_back( &this->global_rating_any );
	this->text_buttons.push_back( &this->global_rating_easy );
	this->text_buttons.push_back( &this->global_rating_medium );
	this->text_buttons.push_back( &this->global_rating_hard );
	this->text_buttons.push_back( &this->global_rating_impossible );
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->search_button );
	
	this->init_vram();
	
	this->cached_global_rating = static_cast<Rating>( round(this->program.words_db->get_avg_rating()) );
	
	// Menü zur gespeicherten Position bewegen:
	std::string config_book_name = this->program.config->get( "current_book_name", "" );
	int config_lesson_number = this->program.config->get( "current_lesson_number", 0 );
	if( config_book_name.length() )
	{
		bool found = false;
		// tatsächlichen Wert nur verändern, wenn wir auch was finden:
		int _y_offset = 0;
		for( Library::iterator book_it=this->program.library->begin();
			book_it!=this->program.library->end(); 
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
				BookEntry* entry = new BookEntry( *this, book_it->second );
				if( config_lesson_number ) entry->exploded = true;
				this->menu_list[ static_cast<void*>(entry->book) ] = entry;
				break;
			}
		}
	}
	
	// restore current range and order settings:
	this->choice.content_range = static_cast<LessonMenuChoice::ContentRange>( this->program.config->get("lesson_menu.content_range", this->choice.content_range) );
	this->choice.content_order = static_cast<LessonMenuChoice::ContentOrder>( this->program.config->get("lesson_menu.content_order", this->choice.content_order) );
}

MenuEntry::MenuEntry( LessonMenu& _lesson_menu ) 
	: ButtonProvider( _lesson_menu.button_provider_list, *_lesson_menu.program.ft ), 
	lesson_menu( _lesson_menu ),
	text_surface( new RenderScreenBuffer(200, MenuEntry::BASE_HEIGHT) ),
	exploded(false), text_rendered(false), top(0), last_frame_rendered(0),
	rating_bar_any_extension( _lesson_menu.rating_bar_any_extension ),
	rating_bar( _lesson_menu.rating_bar ),
	rating_any( _lesson_menu.rating_any ),
	rating_easy( _lesson_menu.rating_easy ),
	rating_medium( _lesson_menu.rating_easy ),
	rating_hard( _lesson_menu.rating_hard ),
	rating_impossible( _lesson_menu.rating_impossible ),
	order_index_button( _lesson_menu.order_index_button ),
	order_latency_button( _lesson_menu.order_latency_button ),
	order_random_button( _lesson_menu.order_random_button ),
	cached_rating( RATING_NONE )
{
	this->text_buttons.push_back( &this->rating_bar_any_extension );
	this->text_buttons.push_back( &this->rating_bar );
	this->text_buttons.push_back( &this->rating_any );
	this->text_buttons.push_back( &this->rating_easy );
	this->text_buttons.push_back( &this->rating_medium );
	this->text_buttons.push_back( &this->rating_hard );
	this->text_buttons.push_back( &this->rating_impossible );
	this->text_buttons.push_back( &this->order_index_button );
	this->text_buttons.push_back( &this->order_latency_button );
	this->text_buttons.push_back( &this->order_random_button );
	
	this->init_button_vram();
}

BookEntry::BookEntry( LessonMenu& _lesson_menu, Book* _book )
	: MenuEntry(_lesson_menu), book(_book),
	book_icon( _lesson_menu.book_icon ),
	explode_button( _lesson_menu.explode_button ),
	implode_button( _lesson_menu.implode_button )
{
	this->text_buttons.push_back( &this->book_icon );
	this->text_buttons.push_back( &this->explode_button );
	this->text_buttons.push_back( &this->implode_button );
	
	this->init_button_vram();
	
	this->book_icon.disabled = true; // ignore input events on book icon
	
	this->cached_rating = static_cast<Rating>( round(lesson_menu.program.words_db->get_avg_rating(this->book)) );
}

LessonEntry::LessonEntry( LessonMenu& _lesson_menu, Lesson* _lesson )
	: MenuEntry(_lesson_menu), lesson(_lesson),
	lesson_icon( _lesson_menu.lesson_icon ),
	grammar_button( _lesson_menu.grammar_button ),
	text_button( _lesson_menu.text_button ),
	exercises_button( _lesson_menu.exercises_button ),
	jump_down_button( _lesson_menu.jump_down_button ),
	jump_up_button( _lesson_menu.jump_up_button ),
	lesson_range_button( _lesson_menu.lesson_range_button ),
	book_range_button( _lesson_menu.book_range_button )
{
	this->text_buttons.push_back( &this->lesson_icon );
	this->text_buttons.push_back( &this->grammar_button );
	this->text_buttons.push_back( &this->text_button );
	this->text_buttons.push_back( &this->exercises_button );
	this->text_buttons.push_back( &this->jump_down_button );
	this->text_buttons.push_back( &this->jump_up_button );
	this->text_buttons.push_back( &this->lesson_range_button );
	this->text_buttons.push_back( &this->book_range_button );
	
	this->init_button_vram();
	
	this->lesson_icon.disabled = true; // ignore input events on lesson icon
	
	this->cached_rating = static_cast<Rating>( round(lesson_menu.program.words_db->get_avg_rating(this->lesson)) );
}


void LessonMenu::init_mode()
{
	this->program.ft->init_screen( this->info_screen );
	this->info_screen.clear();
	this->info_screen.clear_bg();
	
	this->program.ft->init_screen( this->menu_screen );
	this->menu_screen.clear();
	// Farbindex 0 der Hintergrundpalette auf hellblau für's Highlight setzen:
	this->menu_screen.palette[0] = 31<<10|28<<5|28;
	
	Mode::init_mode();
}


void LessonMenu::init_vram()
{
	Mode::init_vram();
}

void LessonMenu::init_button_vram()
{
	// vorgerenderte Spritegrafiken laden:
	this->book_icon.init_vram( accessories_dictionaryBitmap, this->book_icon.bg_vram );
	this->book_icon.init_vram( accessories_dictionary_openBitmap, this->book_icon.bg_active_vram );
	this->lesson_icon.init_vram( text_x_genericBitmap, this->lesson_icon.bg_vram );
	this->grammar_button.init_vram( menu_buttonBitmap, this->grammar_button.bg_vram );
	this->grammar_button.init_vram( menu_button_activeBitmap, this->grammar_button.bg_active_vram );
	this->grammar_button.init_vram( menu_button_inactiveBitmap, this->grammar_button.bg_inactive_vram );
	this->text_button.bg_vram = this->grammar_button.bg_vram;
	this->text_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->text_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->text_button.owns_bg_vram = false;
	this->text_button.init_vram( tiny_bookBitmap, this->text_button.fg_vram );
	this->exercises_button.bg_vram = this->grammar_button.bg_vram;
	this->exercises_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->exercises_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->exercises_button.init_vram( tiny_textBitmap, this->exercises_button.fg_vram );
	this->exercises_button.owns_bg_vram = false;

	this->jump_down_button.init_vram( small_menu_buttonBitmap, this->jump_down_button.bg_vram );
	this->jump_down_button.init_vram( small_menu_button_activeBitmap, this->jump_down_button.bg_active_vram );
	this->jump_down_button.init_vram( small_menu_button_inactiveBitmap, this->jump_down_button.bg_inactive_vram );
	this->jump_up_button.bg_vram = this->jump_down_button.bg_vram;
	this->jump_up_button.bg_active_vram = this->jump_down_button.bg_active_vram;
	this->jump_up_button.bg_inactive_vram = this->jump_down_button.bg_inactive_vram;
	this->jump_up_button.owns_bg_vram = false;
	
	this->explode_button.bg_vram = this->grammar_button.bg_vram;
	this->explode_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->explode_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->explode_button.owns_bg_vram = false;
	this->implode_button.bg_vram = this->grammar_button.bg_vram;
	this->implode_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->implode_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->implode_button.owns_bg_vram = false;

	this->lesson_range_button.init_vram( tiny_lessonBitmap, this->lesson_range_button.fg_vram );
	this->lesson_range_button.bg_vram = this->grammar_button.bg_vram;
	this->lesson_range_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->lesson_range_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->lesson_range_button.owns_bg_vram = false;
	this->book_range_button.init_vram( tiny_lessonsBitmap, this->book_range_button.fg_vram );
	this->book_range_button.bg_vram = this->grammar_button.bg_vram;
	this->book_range_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->book_range_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->book_range_button.owns_bg_vram = false;

	this->order_index_button.bg_vram = this->grammar_button.bg_vram;
	this->order_index_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->order_index_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->order_index_button.owns_bg_vram = false;
	this->order_latency_button.init_vram( tiny_clockBitmap, this->order_latency_button.fg_vram );
	this->order_latency_button.bg_vram = this->grammar_button.bg_vram;
	this->order_latency_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->order_latency_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->order_latency_button.owns_bg_vram = false;
	this->order_random_button.init_vram( tiny_diceBitmap, this->order_random_button.fg_vram );
	this->order_random_button.bg_vram = this->grammar_button.bg_vram;
	this->order_random_button.bg_active_vram = this->grammar_button.bg_active_vram;
	this->order_random_button.bg_inactive_vram = this->grammar_button.bg_inactive_vram;
	this->order_random_button.owns_bg_vram = false;
	
	this->rating_bar_any_extension.init_vram( center_rating_bar_any_extensionBitmap, this->rating_bar_any_extension.bg_vram );
	this->rating_bar_any_extension.bg_prio = 2; // place bar behind rating emotes
	this->rating_bar.init_vram( center_rating_barBitmap, this->rating_bar.bg_vram );
	this->rating_bar.init_vram( center_rating_bar_disabledBitmap, this->rating_bar.bg_inactive_vram );
	this->rating_bar.bg_prio = 2; // place bar behind rating emotes
	//this->rating_any.init_vram( bottom_rating_anyBitmap, this->rating_any.bg_vram );
	this->rating_any.init_vram( bottom_rating_any_activeBitmap, this->rating_any.bg_active_vram );
	this->rating_easy.init_vram( bottom_rating_easyBitmap, this->rating_easy.bg_vram );
	this->rating_easy.init_vram( bottom_rating_easy_activeBitmap, this->rating_easy.bg_active_vram );
	this->rating_easy.init_vram( bottom_rating_easy_disabledBitmap, this->rating_easy.bg_inactive_vram );
	this->rating_medium.init_vram( bottom_rating_mediumBitmap, this->rating_medium.bg_vram );
	this->rating_medium.init_vram( bottom_rating_medium_activeBitmap, this->rating_medium.bg_active_vram );
	this->rating_medium.init_vram( bottom_rating_medium_disabledBitmap, this->rating_medium.bg_inactive_vram );
	this->rating_hard.init_vram( bottom_rating_hardBitmap, this->rating_hard.bg_vram );
	this->rating_hard.init_vram( bottom_rating_hard_activeBitmap, this->rating_hard.bg_active_vram );
	this->rating_hard.init_vram( bottom_rating_hard_disabledBitmap, this->rating_hard.bg_inactive_vram );
	this->rating_impossible.init_vram( bottom_rating_impossibleBitmap, this->rating_impossible.bg_vram );
	this->rating_impossible.init_vram( bottom_rating_impossible_activeBitmap, this->rating_impossible.bg_active_vram );
	this->rating_impossible.init_vram( bottom_rating_impossible_disabledBitmap, this->rating_impossible.bg_inactive_vram );
	
	this->global_rating_bar_any_extension.init_vram( bottom_rating_bar_any_extensionBitmap, this->global_rating_bar_any_extension.bg_vram );
	this->global_rating_bar_any_extension.bg_prio = 2; // place bar behind rating emotes
	this->global_rating_bar.init_vram( bottom_rating_barBitmap, this->global_rating_bar.bg_vram );
	this->global_rating_bar.bg_prio = 2; // place bar behind rating emotes
	this->global_rating_any.bg_vram = this->rating_any.bg_vram;
	this->global_rating_any.bg_active_vram = this->rating_any.bg_active_vram;
	this->global_rating_any.owns_bg_vram = false;
	this->global_rating_easy.bg_vram = this->rating_easy.bg_vram;
	this->global_rating_easy.bg_active_vram = this->rating_easy.bg_active_vram;
	this->global_rating_easy.owns_bg_vram = false;
	this->global_rating_medium.bg_vram = this->rating_medium.bg_vram;
	this->global_rating_medium.bg_active_vram = this->rating_medium.bg_active_vram;
	this->global_rating_medium.owns_bg_vram = false;
	this->global_rating_hard.bg_vram = this->rating_hard.bg_vram;
	this->global_rating_hard.bg_active_vram = this->rating_hard.bg_active_vram;
	this->global_rating_hard.owns_bg_vram = false;
	this->global_rating_impossible.bg_vram = this->rating_impossible.bg_vram;
	this->global_rating_impossible.bg_active_vram = this->rating_impossible.bg_active_vram;
	this->global_rating_impossible.owns_bg_vram = false;
	
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	
	this->search_button.init_vram( bottom_center_buttonBitmap, this->search_button.bg_vram );
	this->search_button.init_vram( bottom_center_button_activeBitmap, this->search_button.bg_active_vram );
	this->search_button.init_vram( tiny_searchBitmap, this->search_button.fg_vram );
	
	this->loading_symbol.init_vram( loadingBitmap, this->loading_symbol.bg_vram );
	
	ButtonProvider::init_button_vram();
}

void MenuEntry::init_button_vram()
{
	this->rating_bar_any_extension = this->lesson_menu.rating_bar_any_extension;
	this->rating_bar = this->lesson_menu.rating_bar;
	this->rating_any = this->lesson_menu.rating_any;
	this->rating_easy = this->lesson_menu.rating_easy;
	this->rating_medium = this->lesson_menu.rating_medium;
	this->rating_hard = this->lesson_menu.rating_hard;
	this->rating_impossible = this->lesson_menu.rating_impossible;
	this->order_index_button = this->lesson_menu.order_index_button;
	this->order_index_button.bg_prio = this->order_index_button.text_prio = this->order_index_button.fg_prio = 2;
	this->order_latency_button = this->lesson_menu.order_latency_button;
	this->order_latency_button.bg_prio = this->order_latency_button.text_prio = this->order_latency_button.fg_prio = 2;
	this->order_random_button = this->lesson_menu.order_random_button;
	this->order_random_button.bg_prio = this->order_random_button.text_prio = this->order_random_button.fg_prio = 2;
	
	for( TextButtonList::iterator tbi=this->text_buttons.begin();
		tbi!=this->text_buttons.end(); tbi++ )
	{
		(*tbi)->owns_bg_vram = (*tbi)->owns_text_vram = (*tbi)->owns_fg_vram
		= (*tbi)->disabled = (*tbi)->hidden = false;
	}
	
	// just initializes owned text vram, but we do not own any:
	//ButtonProvider::init_button_vram();
}

void BookEntry::init_button_vram()
{
	this->book_icon = this->lesson_menu.book_icon;
	this->explode_button = this->lesson_menu.explode_button;
	this->implode_button = this->lesson_menu.implode_button;
	
	MenuEntry::init_button_vram();
}

void LessonEntry::init_button_vram()
{
	this->lesson_icon = this->lesson_menu.lesson_icon;
	this->grammar_button = this->lesson_menu.grammar_button;
	this->grammar_button.bg_prio = this->grammar_button.text_prio = 2;
	this->text_button = this->lesson_menu.text_button;
	this->text_button.bg_prio = this->text_button.text_prio = 2;
	this->exercises_button = this->lesson_menu.exercises_button;
	this->exercises_button.bg_prio = this->exercises_button.text_prio = 2;
	this->jump_down_button = this->lesson_menu.jump_down_button;
	this->jump_up_button = this->lesson_menu.jump_up_button;
	this->lesson_range_button = this->lesson_menu.lesson_range_button;
	this->lesson_range_button.bg_prio = this->lesson_range_button.text_prio = this->lesson_range_button.fg_prio = 2;
	this->book_range_button = this->lesson_menu.book_range_button;
	this->book_range_button.bg_prio = this->book_range_button.text_prio = this->book_range_button.fg_prio = 2;
	
	MenuEntry::init_button_vram();
}


LessonMenu::~LessonMenu()
{
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


void LessonMenu::render( Screen screen )
{
	this->sub_frame_count++;
	if( screen == SCREEN_MAIN )
	{
		this->info_screen.clear();
		std::string author, title, publisher, description, stats_text;
		if( this->menu_list.count(this->active_list_id) )
		{
			MenuEntry* entry = this->menu_list[ this->active_list_id ];
			BookEntry* book_entry = dynamic_cast<BookEntry*>( entry );
			LessonEntry* lesson_entry = dynamic_cast<LessonEntry*>( entry );
			if( book_entry )
			{
				author = book_entry->book->author;
				title = book_entry->book->title;
				publisher = book_entry->book->publisher;
				description = book_entry->book->description;
				int word_count = 0;
				// FIXME: query SQL database:
#if 0
				for( Book::iterator lesson_it = book_entry->book->begin();
					lesson_it != book_entry->book->end(); lesson_it++ )
				{
					Lesson* lesson = lesson_it->second;
					word_count += lesson->new_words.size();
				}
#endif
				std::stringstream stats_stream;
				stats_stream << "contains " << book_entry->book->size() << " lessons";
				// FIXME: query SQL database: " and " << word_count << " words";
				stats_text = stats_stream.str();
			}
			else if( lesson_entry )
			{
				title = lesson_entry->lesson->title;
				description = lesson_entry->lesson->description;
				// FIXME: query SQL database:
#if 0
				std::stringstream stats_stream;
				stats_stream << "contains " << lesson_entry->lesson->new_words.size() << " words";
				stats_text = stats_stream.str();
#endif
			}
		}
		else
		{
			author = "Chinese Touch";
			title = "汉语接触";
			publisher = "an extensible language learning tool (v" + this->program.version + ")";
			publisher += "\n-\nsyslock@gmail.com\nhttp://code.google.com/p/chinese-touch";
			description = "Lesson Menu: Please select a book or lesson on the touchscreen!";
			std::stringstream stats_stream;
			int lesson_count = 0;
			for( Library::iterator book_it = this->program.library->begin();
				book_it != this->program.library->end(); book_it++ )
			{
				lesson_count += book_it->second->size();
			}
			stats_stream << "" << lesson_count << " lessons from " << this->program.library->size() << " books available." ;
			stats_text = stats_stream.str();
		}
		int top = 0;
		RenderInfo rect(0,0,0,0);
		top += 3;
		if( author.length() )
		{
			RenderStyle render_style;
			render_style.center_x = true;
			rect = this->program.ft->render( this->info_screen, author, 
				this->program.ft->latin_face, 10, 0, top, &render_style );
			top += rect.height+15;
		}
		if( title.length() )
		{
			RenderStyle render_style;
			render_style.center_x = true;
			rect = this->program.ft->render( this->info_screen, title, 
				this->program.ft->latin_face, 14, 0, top, &render_style );
			top += rect.height+15;
		} else top += 24+15;
		if( publisher.length() )
		{
			RenderStyle render_style;
			render_style.center_x = true;
			rect = this->program.ft->render( this->info_screen, publisher, 
				this->program.ft->latin_face, 7, 0, top, &render_style );
			top += rect.height+15;
		}
		if( description.length() )
		{
			RenderStyle render_style;
			rect = this->program.ft->render( this->info_screen, description, 
				this->program.ft->latin_face, 8, 3, top, &render_style );
			top += rect.height+15;
		}
		if( stats_text.length() )
		{
			RenderStyle render_style;
			rect = this->program.ft->render( this->info_screen, stats_text, 
				this->program.ft->latin_face, 7, 3, top, &render_style );
			top += rect.height+15;
		}
		
		Mode::render( screen );
	}
	else if( screen == SCREEN_SUB )
	{
		int top = this->y_offset;
		
		for( Library::iterator book_it = this->program.library->begin(); 
			book_it != this->program.library->end() && top < this->menu_screen.res_y; 
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
				// book_entry anlegen, falls nicht schon früher geschehen,
				// da wir ihn nun wirklich brauchen:
				if( !book_entry )
				{
					book_entry = new BookEntry( *this, book_it->second );
					this->menu_list[ book_id ] = book_entry;
				}
				book_entry->top = top;
				book_entry->last_frame_rendered = this->sub_frame_count;
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
					LessonEntry* lesson_entry = 0;
					if( this->menu_list.count( lesson_id ) )
					{
						lesson_entry = dynamic_cast<LessonEntry*>( this->menu_list[lesson_id] );
					}
					if( top > -MenuEntry::ACTIVE_HEIGHT )
					{
						if( !lesson_entry )
						{
							lesson_entry = new LessonEntry( *this, lesson_it->second );
							this->menu_list[ lesson_id ] = lesson_entry;
						}
						lesson_entry->top = top;
						lesson_entry->last_frame_rendered = this->sub_frame_count;
					}
					if( lesson_entry && lesson_entry->lesson && (lesson_id == this->active_list_id) )
					{
						top += MenuEntry::ACTIVE_HEIGHT;
					}
					else top += MenuEntry::BASE_HEIGHT;
				}
			}
		}
		
		// hide all but currently active rating:
		this->global_rating_any.hidden = !( this->global_rating_any.active );
		this->global_rating_easy.hidden = !( this->global_rating_easy.active || this->cached_global_rating==RATING_EASY );
		this->global_rating_medium.hidden = !( this->global_rating_medium.active || this->cached_global_rating==RATING_MEDIUM );
		this->global_rating_hard.hidden = !( this->global_rating_hard.active || this->cached_global_rating==RATING_HARD );
		this->global_rating_impossible.hidden = !( this->global_rating_impossible.active || this->cached_global_rating==RATING_IMPOSSIBLE );
		
		// Basisimplementierung schon hier aufrufen, um Verwackeln zwischen Sprites und Hintergrund zu vermeiden?
		Mode::render( screen );
		
		// copy buffered background data to vram:
		this->menu_screen.clear( 1 );
		for( MenuList::iterator entry_it = this->menu_list.begin();
			entry_it != this->menu_list.end(); entry_it++ )
		{
			MenuEntry* entry = entry_it->second;
			if( entry->last_frame_rendered == this->sub_frame_count )
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

void MenuEntry::render_buttons( OamState* oam_state, int& oam_entry )
{
	// ignore buttons on stale menu entries:
	if( this->last_frame_rendered!=this->lesson_menu.sub_frame_count )
		return;
	
	// vertical button offset is different on selected entry:
	bool active = this->get_entry_id()==this->lesson_menu.active_list_id;
	int y;
	if( active ) y = this->top + MenuEntry::BUTTON_ACTIVE_Y_OFFSET;
	else y = this->top + MenuEntry::BUTTON_Y_OFFSET;
	
	// show any rating selector on selected menu entries only:
	this->rating_bar_any_extension.hidden = this->rating_bar_any_extension.disabled
	= this->rating_any.hidden = this->rating_any.disabled
	= this->order_index_button.hidden = this->order_index_button.disabled
	= this->order_latency_button.hidden = this->order_latency_button.disabled
	= this->order_random_button.hidden = this->order_random_button.disabled
	= !active;
	
	this->rating_bar.y 
	= this->rating_easy.y 
	= this->rating_medium.y
	= this->rating_hard.y
	= this->rating_impossible.y
	= this->rating_bar_any_extension.y
	= this->rating_any.y
	= y;
	
	this->order_index_button.y
	= this->order_latency_button.y
	= this->order_random_button.y
	= this->top + MenuEntry::BUTTON_Y_OFFSET;
	
	// hide all but currently active rating:
	this->rating_easy.hidden = !( this->rating_easy.active || this->cached_rating==RATING_EASY );
	this->rating_medium.hidden = !( this->rating_medium.active || this->cached_rating==RATING_MEDIUM );
	this->rating_hard.hidden = !( this->rating_hard.active || this->cached_rating==RATING_HARD );
	this->rating_impossible.hidden = !( this->rating_impossible.active || this->cached_rating==RATING_IMPOSSIBLE );
	
	// rating bar is disabled for user input (and rendered as such), when not selected:
	this->rating_bar.disabled 
	= this->rating_easy.disabled 
	= this->rating_medium.disabled
	= this->rating_hard.disabled
	= this->rating_impossible.disabled
	= !active;
	
	if( active )
	{
		this->order_index_button.hidden = this->order_index_button.disabled 
		= !(this->lesson_menu.choice.content_order==LessonMenuChoice::CONTENT_ORDER_INDEX);
		this->order_latency_button.hidden = this->order_latency_button.disabled 
		= !(this->lesson_menu.choice.content_order==LessonMenuChoice::CONTENT_ORDER_LATENCY);
		this->order_random_button.hidden = this->order_random_button.disabled 
		= !(this->lesson_menu.choice.content_order==LessonMenuChoice::CONTENT_ORDER_RANDOM);
	}
	
	ButtonProvider::render_buttons(oam_state, oam_entry);
}

void BookEntry::render_buttons(OamState* oam_state, int& oam_entry)
{
	// only render MOST buttons on selected entry:
	bool active = this->get_entry_id()==this->lesson_menu.active_list_id;
	this->explode_button.hidden = this->explode_button.disabled
	= this->implode_button.hidden = this->implode_button.disabled
	= !active;
	
	this->book_icon.y = this->top;
	if( this->exploded ) 
	{
		// open book symbol
		this->book_icon.bg_vram = this->lesson_menu.book_icon.bg_active_vram;
	}
	else
	{
		// closed book symbol
		this->book_icon.bg_vram = this->lesson_menu.book_icon.bg_vram;
	}
	
	if( active )
	{
		int y = this->top + MenuEntry::BUTTON_ACTIVE_Y_OFFSET;
		this->explode_button.y = this->implode_button.y = y;
		this->explode_button.hidden = this->exploded;
		this->explode_button.disabled = this->explode_button.hidden || !this->book->size();
		this->implode_button.hidden = this->implode_button.disabled = !this->exploded;
	}
	
	MenuEntry::render_buttons(oam_state, oam_entry);
}

void LessonEntry::render_buttons(OamState* oam_state, int& oam_entry)
{
	// hide/disable different text catogories by default:
	this->grammar_button.hidden = this->grammar_button.disabled
	= this->text_button.hidden = this->text_button.disabled
	= this->exercises_button.hidden = this->exercises_button.disabled
	= true;
	// render most buttons on selected lesson entry only:
	bool active = this->get_entry_id()==this->lesson_menu.active_list_id;
	this->jump_down_button.hidden = this->jump_down_button.disabled
	= this->jump_up_button.hidden = this->jump_up_button.disabled
	= this->lesson_range_button.hidden = this->lesson_range_button.disabled
	= this->book_range_button.hidden = this->book_range_button.disabled
	= !active;
	
	this->lesson_icon.y = this->top;
	
	if( active )
	{
		// range button is rendered above normal buttons;
		this->lesson_range_button.y
		= this->book_range_button.y
		= this->top + MenuEntry::BUTTON_Y_OFFSET;
		
		int y = this->top + MenuEntry::BUTTON_ACTIVE_Y_OFFSET;
		this->jump_down_button.y 
		= this->jump_up_button.y
		= this->grammar_button.y
		= this->text_button.y
		= this->exercises_button.y
		= y;
		
		int x = MenuEntry::BUTTON_X_OFFSET;
		if( this->lesson->grammar_texts_available )
		{
			this->grammar_button.hidden = this->grammar_button.disabled = false;
			this->grammar_button.x = x;
			x += MenuEntry::BUTTON_WIDTH + MenuEntry::BUTTON_GAP;
		}
		if( this->lesson->lesson_texts_available )
		{
			this->text_button.hidden = this->text_button.disabled = false;
			this->text_button.x = x;
			x += MenuEntry::BUTTON_WIDTH + MenuEntry::BUTTON_GAP;
		}
		if( this->lesson->exercises_available )
		{
			this->exercises_button.hidden = this->exercises_button.disabled = false;
			this->exercises_button.x = x;
			x += MenuEntry::BUTTON_WIDTH + MenuEntry::BUTTON_GAP;
		}
		
		this->lesson_range_button.hidden = this->lesson_range_button.disabled 
		= !(this->lesson_menu.choice.content_range==LessonMenuChoice::CONTENT_RANGE_LESSON);
		this->book_range_button.hidden = this->book_range_button.disabled 
		= !(this->lesson_menu.choice.content_range==LessonMenuChoice::CONTENT_RANGE_BOOK);
	}
	
	MenuEntry::render_buttons(oam_state, oam_entry);
}

void MenuEntry::render_text( FreetypeRenderer& ft, const std::string& text )
{
	RenderStyle render_style;
	//render_style.linebreak = false;
	ft.render( *this->text_surface, text,
		ft.latin_face, MenuEntry::FONT_SIZE, 0, 0, &render_style );
	this->text_rendered = true;
}


ButtonAction LessonMenu::handle_button_pressed(TextButton* text_button)
{
	if( text_button == &this->settings_button )
	{
		this->show_settings();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->search_button )
	{
		this->choice.content_type = LessonMenuChoice::CONTENT_TYPE_SEARCH;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->global_rating_any )
	{
		this->choice.book = 0;
		this->choice.lesson = 0;
		this->choice.content_order = LessonMenuChoice::CONTENT_ORDER_LATENCY;
		this->choice.content_type = LessonMenuChoice::CONTENT_TYPE_ANY_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->global_rating_easy )
	{
		this->choice.book = 0;
		this->choice.lesson = 0;
		this->choice.content_order = LessonMenuChoice::CONTENT_ORDER_LATENCY;
		this->choice.content_type = LessonMenuChoice::CONTENT_TYPE_EASY_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->global_rating_medium )
	{
		this->choice.book = 0;
		this->choice.lesson = 0;
		this->choice.content_order = LessonMenuChoice::CONTENT_ORDER_LATENCY;
		this->choice.content_type = LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->global_rating_hard )
	{
		this->choice.book = 0;
		this->choice.lesson = 0;
		this->choice.content_order = LessonMenuChoice::CONTENT_ORDER_LATENCY;
		this->choice.content_type = LessonMenuChoice::CONTENT_TYPE_HARD_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->global_rating_impossible )
	{
		this->choice.book = 0;
		this->choice.lesson = 0;
		this->choice.content_order = LessonMenuChoice::CONTENT_ORDER_LATENCY;
		this->choice.content_type = LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	
	return ButtonProvider::handle_button_pressed(text_button);
}

ButtonAction MenuEntry::handle_button_pressed(TextButton* text_button)
{
	// ignore buttons on stale menu entries:
	if( this->last_frame_rendered!=this->lesson_menu.sub_frame_count )
		return BUTTON_ACTION_UNHANDLED;
	
	/* TODO: query word list availability from database for each rating and deny access to empty word 
		lists with a short warning */
	
	if( text_button == &this->rating_any )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_ANY_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->rating_easy )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_EASY_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->rating_medium )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->rating_hard )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_HARD_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->rating_impossible )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->order_index_button )
	{
		this->lesson_menu.choice.content_order = LessonMenuChoice::CONTENT_ORDER_LATENCY; // change to latency ordering
		this->lesson_menu.program.config->set( "lesson_menu.content_order", this->lesson_menu.choice.content_order );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->order_latency_button )
	{
		this->lesson_menu.choice.content_order = LessonMenuChoice::CONTENT_ORDER_RANDOM; // change to random ordering
		this->lesson_menu.program.config->set( "lesson_menu.content_order", this->lesson_menu.choice.content_order );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->order_random_button )
	{
		this->lesson_menu.choice.content_order = LessonMenuChoice::CONTENT_ORDER_INDEX; // change back to index ordering
		this->lesson_menu.program.config->set( "lesson_menu.content_order", this->lesson_menu.choice.content_order );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return ButtonProvider::handle_button_pressed(text_button);
}

ButtonAction BookEntry::handle_button_pressed(TextButton* text_button)
{
	// ignore buttons on stale menu entries:
	if( this->last_frame_rendered!=this->lesson_menu.sub_frame_count )
		return BUTTON_ACTION_UNHANDLED;
	
	this->lesson_menu.choice.book = this->book;
	this->lesson_menu.choice.lesson = 0;
	
	if( text_button == &this->explode_button )
	{
		this->exploded = true;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->implode_button )
	{
		this->exploded = false;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return MenuEntry::handle_button_pressed(text_button);
}

ButtonAction LessonEntry::handle_button_pressed(TextButton* text_button)
{
	// ignore buttons on stale menu entries:
	if( this->last_frame_rendered!=this->lesson_menu.sub_frame_count )
		return BUTTON_ACTION_UNHANDLED;
	
	this->lesson_menu.choice.lesson = this->lesson;
	this->lesson_menu.choice.book = this->lesson->book;
	
	if( text_button == &this->jump_down_button )
	{
		// restore current range and order settings:
		this->lesson_menu.choice.content_range = static_cast<LessonMenuChoice::ContentRange>( this->lesson_menu.program.config->get("lesson_menu.content_range", this->lesson_menu.choice.content_range) );
		this->lesson_menu.choice.content_order = static_cast<LessonMenuChoice::ContentOrder>( this->lesson_menu.program.config->get("lesson_menu.content_order", this->lesson_menu.choice.content_order) );
		for( Book::iterator i=this->lesson->book->find(this->lesson->number); 
			i!=this->lesson->book->end(); i++ )
		{
			void* list_id = static_cast<void*>( i->second );
			MenuList::iterator test_it = this->lesson_menu.menu_list.find( list_id );
			if( test_it!=this->lesson_menu.menu_list.end() )
			{
				this->lesson_menu.active_list_id = list_id;
				test_it->second->top;
				this->lesson_menu.y_offset = this->lesson_menu.y_offset-test_it->second->top + this->lesson_menu.menu_screen.res_y/2;
				this->lesson_menu.render( SCREEN_SUB );
			}
		}
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->jump_up_button )
	{
		// restore current range and order settings:
		this->lesson_menu.choice.content_range = static_cast<LessonMenuChoice::ContentRange>( this->lesson_menu.program.config->get("lesson_menu.content_range", this->lesson_menu.choice.content_range) );
		this->lesson_menu.choice.content_order = static_cast<LessonMenuChoice::ContentOrder>( this->lesson_menu.program.config->get("lesson_menu.content_order", this->lesson_menu.choice.content_order) );
		for( Book::iterator i=this->lesson->book->find(this->lesson->number); ; i-- )
		{
			void* list_id = static_cast<void*>( i->second );
			MenuList::iterator test_it = this->lesson_menu.menu_list.find( list_id );
			if( test_it!=this->lesson_menu.menu_list.end() )
			{
				this->lesson_menu.active_list_id = list_id;
				test_it->second->top;
				this->lesson_menu.y_offset = this->lesson_menu.y_offset-test_it->second->top + this->lesson_menu.menu_screen.res_y/2;
				this->lesson_menu.render( SCREEN_SUB );
			}
			if( i==this->lesson->book->begin() ) break;
		}
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->grammar_button )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_GRAMMAR;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->text_button )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_TEXT;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->exercises_button )
	{
		this->lesson_menu.choice.content_type = LessonMenuChoice::CONTENT_TYPE_EXERCISES;
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_EXIT_MODE;
	}
	if( text_button == &this->lesson_range_button )
	{
		this->lesson_menu.choice.content_range = LessonMenuChoice::CONTENT_RANGE_BOOK; // change to book/multi-lesson scope
		this->lesson_menu.program.config->set( "lesson_menu.content_range", this->lesson_menu.choice.content_range );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->book_range_button )
	{
		this->lesson_menu.choice.content_range = LessonMenuChoice::CONTENT_RANGE_LESSON; // change to back single lesson scope
		this->lesson_menu.program.config->set( "lesson_menu.content_range", this->lesson_menu.choice.content_range );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return MenuEntry::handle_button_pressed(text_button);
}

ButtonAction LessonMenu::handle_touch_begin(touchPosition touch)
{
	this->old_touch = touch;
	this->pixels_scrolled = 0;
	this->old_y_offset = this->y_offset;
	this->loading_symbol.hidden = true;
	
	return Mode::handle_touch_begin(touch);
}

ButtonAction LessonMenu::handle_touch_drag(touchPosition touch)
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// check for touch screen button activation, but only when the user 
	// is not already scrolling the menu:
	if( this->pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
		action |= Mode::handle_touch_drag( touch );
	// do menu scrolling, but only if we are not currently hovering a 
	// touch screen button:
	int y_diff = touch.py - old_touch.py;
	this->old_touch = touch;
	if( action == BUTTON_ACTION_UNHANDLED )
	{
		int abs_y_diff = abs( y_diff );
		if( abs_y_diff && ((old_abs_y_diff && (abs_y_diff <= old_abs_y_diff*LessonMenu::MAX_ACCELERATION_FACTOR)) 
						|| (abs_y_diff <= LessonMenu::MAX_ACCELERATION_FACTOR)) )
		{
			old_abs_y_diff = abs_y_diff;
			pixels_scrolled += abs(y_diff);
			this->y_offset += y_diff;
			this->v_y = y_diff;
			// FIXME: force render() not to updates sprites, to make scrolling effect faster
			this->render( SCREEN_SUB );
			return BUTTON_ACTION_CHANGED;
		}
	}
	
	return action;
}

ButtonAction LessonMenu::handle_touch_end(touchPosition touch)
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// ignore any touch screen buttons and menu entries currently pointed on,
	// when the user was scrolling text, so she does not press a button 
	// by accident:
	if( this->pixels_scrolled >= BUTTON_ACTIVATION_SCROLL_LIMIT )
		return BUTTON_ACTION_UNHANDLED;
	// else priority check normal touch screen buttons for a pressed event:
	else action |= Mode::handle_touch_end( touch );
	// if no other button was activated, fall back to menu entry tap detection:
	if( action == BUTTON_ACTION_UNHANDLED )
	{
		// HACK: render sub screen to guarantee entry->last_frame_rendered to be up-to-date
		this->render( SCREEN_SUB );
		bool found = false;
		for( MenuList::iterator entry_it = this->menu_list.begin();
			!found && entry_it != this->menu_list.end() && entry_it->second->top<this->menu_screen.res_y; 
			entry_it++ )
		{
			void* entry_id = entry_it->first;
			MenuEntry* entry = entry_it->second;
			if( entry->last_frame_rendered==this->sub_frame_count
				&& old_touch.py > entry->top  
				&& ((entry_id==this->active_list_id 
					&& old_touch.py < entry->top+MenuEntry::ACTIVE_HEIGHT) 
					|| old_touch.py < entry->top+MenuEntry::BASE_HEIGHT) )
			{
				this->active_list_id = entry_id;
				// restore current range and order settings:
				this->choice.content_range = static_cast<LessonMenuChoice::ContentRange>( this->program.config->get("lesson_menu.content_range", this->choice.content_range) );
				this->choice.content_order = static_cast<LessonMenuChoice::ContentOrder>( this->program.config->get("lesson_menu.content_order", this->choice.content_order) );
				this->render( SCREEN_SUB );
				this->render( SCREEN_MAIN );
			}
		}
	}
	
	return action;
}

bool update_priority_sort_pred( MenuEntry* left, MenuEntry* right )
{
	int focus = left->lesson_menu.menu_screen.res_y/3;
	return abs(left->top-focus) < abs(right->top-focus);
}

ButtonAction LessonMenu::handle_idle_cycles()
{
	this->pixels_scrolled = 0;
	/* check if there is still momentum from a previous text scrolling input 
		and animate the text based on simple physics */
	if( this->v_y )
	{
		int resistance = this->v_y / 4;
		if( !resistance ) resistance = this->v_y / 2;
		if( !resistance ) resistance = this->v_y;
		this->v_y -= resistance;
		this->y_offset += this->v_y;
		// TODO: force render() not to update static sprites, to make scrolling effect faster
		this->render( SCREEN_SUB );
		return BUTTON_ACTION_CHANGED;
	}
	else
	{
		std::list<MenuEntry*> update_list;
		// find one menu entry near the center of the screen, with a missing text buffer and render it
		for( MenuList::iterator entry_it = this->menu_list.begin(); entry_it != this->menu_list.end(); entry_it++ )
		{
			MenuEntry* entry = entry_it->second;
			//                      render visible entries only for now as the ui becomes too unresponsive otherwise
			if( !entry->text_rendered && entry->last_frame_rendered==this->sub_frame_count ) 
			{
				update_list.push_back( entry );
			}
		}
		if( update_list.size() )
		{
			if( this->loading_symbol.hidden )
			{
				this->loading_symbol.hidden = false;
				this->render( SCREEN_SUB );
			}
			else
			{
				this->loading_symbol.bg_rotation = (this->loading_symbol.bg_rotation > 0) ?  (this->loading_symbol.bg_rotation - 1) : 31;
			}
			update_list.sort( update_priority_sort_pred );
			MenuEntry* entry = *update_list.begin();
			entry->render_text( *this->program.ft, entry->get_title() );
			if( entry->last_frame_rendered==this->sub_frame_count )
			{
				this->render( SCREEN_SUB );
			}
		}
		else
		{
			if( !this->loading_symbol.hidden )
			{
				this->loading_symbol.hidden = true;
				this->render( SCREEN_SUB );
			}
		}
	}
	
	return Mode::handle_idle_cycles();
}


void LessonMenu::show_settings()
{
	this->free_vram();
	SettingsDialog* settings_dialog = new SettingsDialog( this->program, this->recursion_depth, this->settings, "Program Settings" );
	settings_dialog->run_until_exit();
	delete settings_dialog;
	this->init_mode();
	this->init_vram();
}
