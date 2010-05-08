#include <fstream>
#include <string.h>
#include <iomanip>
#include <map>

#include <nds.h>
#include <fat.h>
#include <sqlite3.h>
#include <time.h> // srand
#include <stdlib.h> // srand

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


Program::Program()
	: first_run( false )
{
	LOG( "initializing fat driver" );
	if( !fatInitDefault() )
	{
		throw ERROR( "error initializing fat driver" );
	}

	try
	{
		this->words_db->open( WORDS_DB_FILE_NAME );
	}
	catch( Error& e )
	{
		WARN( e.what() );
		this->words_db->create( WORDS_DB_FILE_NAME );
		first_run = true;
	}
	this->words_db->update();
	
	LOG( "initializing library" );
	this->library = new Library( *words_db );
	LOG( "scanning library..." );
	this->library->rescan();
	LOG( "scanning complete" );
	
	this->ui_lang = new UILanguage( "en" );
	
	LOG( "initializing Freetype" );
	this->ft = new FreetypeRenderer( "ukai.ttc", "VeraSe.ttf" );
	
	this->config = new Config();
	LOG( "loading config" );
	this->config->load();
}

void Program::run()
{
	srand( time(0) ); // initialize random seed
	bool sync_done = false;
	while( true )
	{
		try
		{
			LOG( "initializing lesson menu" );
			LessonMenu* lesson_menu = new LessonMenu( *this, 0 );
			if( first_run && !sync_done )
			{
				DictionarySynchronizer* dict_sync = new DictionarySynchronizer( "","","", *this, lesson_menu->menu_screen );
				dict_sync->run_action();
				delete dict_sync;
				sync_done = true;
			}
			LessonMenuChoice lesson_menu_choice;
			lesson_menu->run_for_user_choice( lesson_menu_choice );
			delete lesson_menu;
			
			switch( lesson_menu_choice.content_type )
			{
				case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
				case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
				{
					if( !lesson_menu_choice.book )
						throw ERROR( "LessonMenu returned no book" );
					NewWordList words;
					std::stringstream condition;
					condition << "book_id=" << lesson_menu_choice.book->id;
					switch( lesson_menu_choice.content_type )
					{
						case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
							condition << " and rating=" << RATING_EASY;
							break;
						case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
							condition << " and rating=" << RATING_MEDIUM;
							break;
						case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
							condition << " and rating=" << RATING_HARD;
							break;
						case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
							condition << " and (rating=" << RATING_IMPOSSIBLE << " or rating=" << RATING_NONE << ")";
							break;
						default:
							break;
					}
					if( lesson_menu_choice.lesson )
						condition << " and lesson_number<=" << lesson_menu_choice.lesson->number;
					this->words_db->query_words( *this->library, condition.str(), words, "atime" );
					if( lesson_menu_choice.lesson ) config->save_position( lesson_menu_choice.lesson, true );
					else config->save_position( lesson_menu_choice.book, true );
					if( words.size() )
					{
						NewWordsViewer* new_words = new NewWordsViewer( *this, 0, words, false );
						new_words->run_until_exit();
						delete new_words;
					} else throw ERROR( "Empty word list; Assign new words from text mode!" );
					break;
				}
				case LessonMenuChoice::CONTENT_TYPE_NEW_WORDS:
				{
					Lesson* lesson = lesson_menu_choice.lesson;
					if( !lesson )
						throw ERROR( "LessonMenu returned no lesson" );
					this->config->save_position( lesson, true );
					lesson->parse_dictionary_if_needed();
					NewWordList words;
					std::stringstream condition;
					condition << "lesson_id=" << lesson->id;
					this->words_db->query_words( *this->library, condition.str(), words, "file_offset" );
					if( words.size() )
					{
						NewWordsViewer* new_words = new NewWordsViewer( *this, 0, words, true );
						new_words->run_until_exit();
						delete new_words;
					} else throw ERROR( "Empty word list; Assign new words from text mode!" );
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
			RenderScreen error_screen;
			this->ft->init_screen( SCREEN_MAIN, error_screen );
			error_screen.clear();
			this->ft->render( error_screen, e.what(), this->ft->latin_face, 8, 3, 3 );
			error_stream << "caught " << e.what() << std::endl;
			for( int i=0; i<500; i++ ) swiWaitForVBlank();
		}
	}
}


int main()
{
	defaultExceptionHandler();
	try
	{
		Program program;
		program.run();
	}
	catch( Error& e )
	{
		error_stream << "caught " << e.what() << std::endl;
		ErrorConsole::init_screen();
		ErrorConsole::dump();
	}
	catch( std::exception& e )
	{
		error_stream << "caught " << e.what() << std::endl;
		ErrorConsole::init_screen();
		ErrorConsole::dump();
	}
	return 0;
}
