#include <fstream>
#include <string.h>
#include <iomanip>
#include <map>

#include <nds.h>
#include <fat.h>
#include <sqlite3.h>
#include <time.h> // srand
#include <stdlib.h> // srand
#include <nds/arm9/background.h>

#include "chinese-touch.h"
#include "freetype_renderer.h"
#include "unicode.h"
#include "lesson.h"
#include "drawing_pad.h"
#include "config.h"
#include "error_console.h"
#include "lesson_menu.h"
#include "new_words.h"
#include "text_view.h"
#include "words_db.h"
#include "ui_language.h"
#include "fulltext_search.h"
#include "bg_dragon.h"
#include "sprite_helper.h"


Program::Program( int argc, char* argv[] )
	: ui_lang(0), ft(0), words_db(0), library(0),
		config(0), error_console(0), 
		name("chinese-touch"), version("1.5"),
		fs_type("unknown"), image_path(""), base_dir("/"+name),
		words_db_name("words.db")
{
	if( argc>=1 ) 
	{
		std::string image_url = argv[0];
		size_t fs_type_end = image_url.find(':');
		if( fs_type_end != std::string::npos )
		{
			this->fs_type = image_url.substr( 0, fs_type_end );
		}
		if( (fs_type_end+1)<image_url.length() )
		{
			this->image_path = image_url.substr( fs_type_end+1 );
		}
		size_t last_slash = this->image_path.rfind( '/' );
		if( last_slash != std::string::npos )
		{
			std::string _base_dir = image_path.substr( 0, last_slash );
			if( _base_dir.length() )
			{
				this->base_dir = _base_dir;
			}
			else WARN( "Base dir \"" << _base_dir << "\" unsupported! Desmume? Falling back to: "
						<< std::endl << this->base_dir << std::endl );
		}
		LOG( "fs_type: " << this->fs_type );
		LOG( "image_path: " << this->image_path );
		LOG( "base_dir: " << this->base_dir );
	}
	else
	{
		WARN( "Your cards loader software does not support ARGV! Thus the program must reside in:" 
			<< std::endl << this->base_dir << std::endl );
	}
	
	this->error_console = new ErrorConsole(*this);
}

void Program::initialize()
{
	LOG( "initializing fat driver" );
	if( !fatInitDefault() )
	{
		throw ERROR( "error initializing fat driver" );
	}
	
	LOG( "initializing Freetype" );
	this->ft = new FreetypeRenderer( *this, "ukai.ttc", "VeraSe.ttf", "togoshi-mincho.ttf" );
	
	RenderScreen loading_screen( SCREEN_MAIN );
	this->ft->init_screen( loading_screen );
	loading_screen.clear();
	dmaCopy( bg_dragonBitmap, loading_screen.bg_base_address, sizeof(bg_dragonBitmap) );
	set_16bpp_sprite_opague( loading_screen.bg_base_address, 256, 192 );
	bgShow( loading_screen.bg_id );
	
	int size = 9, x = 5, y = 5;
	RenderInfo render_info(0,0,0,0);
	this->words_db = new WordsDB();
	try
	{
		render_info = this->ft->render( loading_screen, "opening "+this->words_db_name, this->ft->han_face, size, x, y+=render_info.height );
		this->words_db->open( this->base_dir+"/"+this->words_db_name );
	}
	catch( Error& e )
	{
		WARN( e.what() );
		render_info = this->ft->render( loading_screen, "creating "+this->words_db_name, this->ft->han_face, size, x, y+=render_info.height );
		this->words_db->create( this->base_dir+"/"+this->words_db_name );
	}
	this->words_db->update();
	
	LOG( "initializing library" );
	render_info = this->ft->render( loading_screen, "initializing library", this->ft->han_face, size, x, y+=render_info.height );
	this->library = new Library( *this );
	LOG( "scanning library..." );
	render_info = this->ft->render( loading_screen, "scanning library...", this->ft->han_face, size, x, y+=render_info.height );
	this->library->rescan();
	LOG( "scanning complete" );
	render_info = this->ft->render( loading_screen, "scanning complete", this->ft->han_face, size, x, y+=render_info.height );
	
	this->ui_lang = new UILanguage( "en" );
	
	this->config = new Config( *this );
	LOG( "loading config" );
	render_info = this->ft->render( loading_screen, "loading config", this->ft->han_face, size, x, y+=render_info.height );
	this->config->load();
}

void Program::run()
{
	srand( time(0) ); // initialize random seed
	bool sync_done = false;
	
	for( Library::iterator lib_it = this->library->begin(); lib_it != this->library->end(); lib_it++ )
	{
		Book* book = lib_it->second;
		if( book->title=="Testfälle" )
		{
			if( book->begin()==book->end() ) break;
			Lesson* lesson = book->begin()->second;
			TextVector& texts = lesson->lesson_texts;
			if( !texts.size() )
			{
				lesson->parse_text( ".text", lesson->lesson_texts );
			}
			if( texts.size() )
			{
				TextView *text_view = new TextView( *this, 0, *texts[0] );
				text_view->run_until_exit();
				delete text_view;
			} else throw ERROR( "No lesson text available" );
			break;
		}
	}
	
	while( true )
	{
		try
		{
			LOG( "initializing lesson menu" );
			LessonMenuChoice lesson_menu_choice;
			LessonMenu* lesson_menu = new LessonMenu( *this, 0, lesson_menu_choice );
			if( this->words_db->update_needed && !sync_done )
			{
				std::string msg;
				for( StringList::iterator ri = this->words_db->update_reasons.begin(); ri != this->words_db->update_reasons.end(); ri++ )
				{
					msg += "- "+ *ri + "\n";
				}
				// remove DictionarySynchronizers from lesson menu settings, as we are running a 
				// synchronizer now and want to prevent users from executing it/them again accidentally:
				for( Settings::iterator si=lesson_menu->settings.begin(); si!=lesson_menu->settings.end(); si++ )
				{
					DictionarySynchronizer* ds = dynamic_cast<DictionarySynchronizer*>( si->second );
					if( ds )
					{
						delete ds;
						lesson_menu->settings.erase( si );
					}
				}
				// create a temporary Synchronizer and run it:
				// 		TODO: let WordsDB determine wether we need to do a full sync here 
				// 		(e.g. newly added dictionary fields need to be filled)
				DictionarySynchronizer* synchronizer = new DictionarySynchronizer( "", "", "", *this, lesson_menu->menu_screen );
				std::string msg2 = "Synchronizing *.dict files with words.db";
				this->ft->render( lesson_menu->info_screen, msg+msg2, this->ft->latin_face, 7, 2, 2 );
				synchronizer->run_action();
				delete synchronizer;
				lesson_menu->info_screen.clear();
				msg2 = "It is recommended to (re-)create the index needed for full-text search on your custom "
						"word entries now. As this might take a couple of minutes you may skip this now and come "
						"back later by tapping the 'S' button in the lesson menu.";
				this->ft->render( lesson_menu->info_screen, msg+msg2, this->ft->latin_face, 7, 2, 2 );
				lesson_menu->show_settings();
				sync_done = true;
			}
			lesson_menu->run_until_exit();
			delete lesson_menu;
			
			switch( lesson_menu_choice.content_type )
			{
				case LessonMenuChoice::CONTENT_TYPE_ANY_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
				{
					NewWordList words;
					std::stringstream condition;
					if( lesson_menu_choice.book ) condition << "book_id=" << lesson_menu_choice.book->id << " and";
					switch( lesson_menu_choice.content_type )
					{
						case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
							condition << " rating=" << RATING_EASY;
							break;
						case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
							condition << " rating=" << RATING_MEDIUM;
							break;
						case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
							condition << " rating=" << RATING_HARD;
							break;
						case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
							condition << " (rating=" << RATING_IMPOSSIBLE << " or rating=" << RATING_NONE << ")";
							break;
						default:
							condition << " rating>=" << RATING_NONE;
							break;
					}
					std::stringstream ordering;
					switch( lesson_menu_choice.content_order )
					{
						case LessonMenuChoice::CONTENT_ORDER_INDEX:
							ordering << " book_id, lesson_number, file_offset";
							break;
						case LessonMenuChoice::CONTENT_ORDER_LATENCY:
							ordering << " atime";
							break;
						case LessonMenuChoice::CONTENT_ORDER_RANDOM:
							// done from cpp later...
							break;
					}
					bool position_saving = false;
					if( lesson_menu_choice.lesson )
					{
						lesson_menu_choice.lesson->parse_dictionary_if_needed();
						if( lesson_menu_choice.content_range==LessonMenuChoice::CONTENT_RANGE_LESSON )
						{
							condition << " and lesson_number=" << lesson_menu_choice.lesson->number;
							// let word list browser do position saving, when single lesson range word list 
							// with offset ordering was requested:
							if( lesson_menu_choice.content_order == LessonMenuChoice::CONTENT_ORDER_INDEX )
								position_saving = true;
						}
						else if( lesson_menu_choice.content_range==LessonMenuChoice::CONTENT_RANGE_BOOK )
						{
							condition << " and lesson_number<=" << lesson_menu_choice.lesson->number;
						}
					}
					this->words_db->query_words( *this->library, condition.str(), words, ordering.str() );
					if( lesson_menu_choice.lesson ) config->save_position( lesson_menu_choice.lesson, true );
					else if( lesson_menu_choice.book ) config->save_position( lesson_menu_choice.book, true );
					NewWordsViewer* new_words = new NewWordsViewer( *this, 0, words, position_saving, 
										(lesson_menu_choice.content_order == LessonMenuChoice::CONTENT_ORDER_RANDOM),
										true /*show settings*/ );
					new_words->run_until_exit();
					delete new_words;
					break;
				}
				case LessonMenuChoice::CONTENT_TYPE_GRAMMAR:
				{
					Lesson* lesson = lesson_menu_choice.lesson;
					if( !lesson )
						throw ERROR( "LessonMenu returned no lesson" );
					this->config->save_position( lesson, true );
					TextVector& texts = lesson_menu_choice.lesson->grammar_texts;
					if( !texts.size() )
					{
						lesson->parse_text( ".grammar", lesson->grammar_texts );
					}
					if( texts.size() )
					{
						TextView *text_view = new TextView( *this, 0, *texts[0] );
						text_view->run_until_exit();
						delete text_view;
					} else throw ERROR( "No grammar description available for this lesson" );
					break;
				}
				case LessonMenuChoice::CONTENT_TYPE_TEXT:
				{
					Lesson* lesson = lesson_menu_choice.lesson;
					if( !lesson )
						throw ERROR( "LessonMenu returned no lesson" );
					this->config->save_position( lesson, true );
					TextVector& texts = lesson_menu_choice.lesson->lesson_texts;
					if( !texts.size() )
					{
						lesson->parse_text( ".text", lesson->lesson_texts );
					}
					if( texts.size() )
					{
						TextView *text_view = new TextView( *this, 0, *texts[0] );
						text_view->run_until_exit();
						delete text_view;
					} else throw ERROR( "No lesson text available" );
					break;
				}
				case LessonMenuChoice::CONTENT_TYPE_EXERCISES:
				{
					Lesson* lesson = lesson_menu_choice.lesson;
					if( !lesson )
						throw ERROR( "LessonMenu returned no lesson" );
					this->config->save_position( lesson, true );
					TextVector& texts = lesson_menu_choice.lesson->exercises;
					if( !texts.size() )
					{
						lesson->parse_text( ".exercise", lesson->exercises );
					}
					if( texts.size() )
					{
						TextView *text_view = new TextView( *this, 0, *texts[0] );
						text_view->run_until_exit();
						delete text_view;
					} else throw ERROR( "No exercises available for this lesson" );
					break;
				}
				case LessonMenuChoice::CONTENT_TYPE_SEARCH:
				{
					FulltextSearch *fulltext_search = new FulltextSearch( *this, 0, 0 );
					fulltext_search->run_until_exit();
					delete fulltext_search;
					break;
				}
				default: throw ERROR( "LessonMenu returned invalid content type" );
			}
		}
		catch( Error& e )
		{
			RenderScreen error_screen( SCREEN_MAIN );
			this->ft->init_screen( error_screen );
			error_screen.clear();
			this->ft->render( error_screen, e.what(), this->ft->latin_face, 8, 3, 3 );
			error_stream << "caught " << e.what() << std::endl;
			for( int i=0; i<500; i++ ) swiWaitForVBlank();
		}
	}
}


int main( int argc, char* argv[] )
{
	defaultExceptionHandler();
	Program* program = 0;
	try
	{
		program = new Program( argc, argv );
		program->initialize();
		program->run();
	}
	catch( Error& e )
	{
		CRITICAL( e.what() );
		if( program && program->error_console )
		{
			program->error_console->init_screen();
			program->error_console->dump();
		}
	}
	catch( std::exception& e )
	{
		CRITICAL( e.what() );
		if( program && program->error_console )
		{
			program->error_console->init_screen();
			program->error_console->dump();
		}
	}
	if( program ) delete program;
	return 0;
}
