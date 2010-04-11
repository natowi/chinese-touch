#include <fstream>
#include <string.h>
#include <iomanip>
#include <map>

#include <nds.h>
#include <fat.h>
#include <sqlite3.h>

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


int main()
{
	defaultExceptionHandler();
	bool first_run = false;
	try
	{
		LOG( "initializing fat driver" );
		if( !fatInitDefault() )
		{
			throw ERROR( "error initializing fat driver" );
		}
		
		WordsDB words_db;
		try
		{
			words_db.open( WORDS_DB_FILE_NAME );
		}
		catch( Error& e )
		{
			WARN( e.what() );
			words_db.create( WORDS_DB_FILE_NAME );
			first_run = true;
		}
		words_db.update();
		
		LOG( "initializing library" );
		Library library( words_db );
		LOG( "scanning library..." );
		library.rescan();
		LOG( "scanning complete" );

		LOG( "initializing Freetype" );
		FreetypeRenderer* ft = new FreetypeRenderer( "ukai.ttc", "VeraSe.ttf" );

		Config config;
		LOG( "loading config" );
		config.load();
	
		UILanguage ui_language( "en" );
#if 0
		NewWordList words;
		words.insert( words.end(), 
					library.begin()->second->begin()->second->new_words.begin(), 
					library.begin()->second->begin()->second->new_words.end() );
		NewWordsViewer* new_words = new NewWordsViewer( *ft, words, &config );
		new_words->run_until_exit();
		delete new_words;
#endif
#if 0
		ErrorConsole::init_screen();
		RenderScreen r;
		ft->init_screen( SCREEN_MAIN, r );
		ft->render( r, "vvvvvvvaaaaaaaaaaaaavvvvvvvvvvvvvvvvvviiiiiiiiiiiiiiiiiiiiiiiiiiii我的中文老师很好！", ft->latin_face, 10, 0, 0 );
		//ft->render( r, "Ich schreib hier mal einen längeren Text hin um auszuprobieren wie gut das mit den horizontalen Buchstabenabständen nun eigentlich funktioniert. 我的中文老师很好！", ft->latin_face, 10, 0, 0 );
		while( true ) swiWaitForVBlank();
#endif
		
		bool sync_done = false;
		while( true )
		{
			try
			{
				LOG( "initializing lesson menu" );
				LessonMenu* lesson_menu = new LessonMenu( *ft, library, config );
				if( first_run && !sync_done )
				{
					DictionarySynchronizer* dict_sync = new DictionarySynchronizer( "","","", *ft, library, lesson_menu->menu_screen );
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
						library.words_db.query_words( library, condition.str(), words, "atime" );
						NewWordsViewer* new_words = new NewWordsViewer( *ft, words, library );
						if( lesson_menu_choice.lesson ) config.save_position( lesson_menu_choice.lesson, true );
						else config.save_position( lesson_menu_choice.book, true );
						new_words->run_until_exit();
						delete new_words;
						break;
					}
					case LessonMenuChoice::CONTENT_TYPE_NEW_WORDS:
					{
						Lesson* lesson = lesson_menu_choice.lesson;
						if( !lesson )
							throw ERROR( "LessonMenu returned no lesson" );
						lesson->parse_dictionary_if_needed();
						NewWordList words;
						std::stringstream condition;
						condition << "lesson_id=" << lesson->id;
						library.words_db.query_words( library, condition.str(), words, "file_offset" );
						NewWordsViewer* new_words = new NewWordsViewer( *ft, words, library, &config );
						new_words->run_until_exit();
						delete new_words;
						break;
					}
					case LessonMenuChoice::CONTENT_TYPE_GRAMMAR:
					{
						Lesson* lesson = lesson_menu_choice.lesson;
						if( !lesson )
							throw ERROR( "LessonMenu returned no lesson" );
						TextVector& texts = lesson_menu_choice.lesson->grammar_texts;
						if( !texts.size() )
						{
							lesson->parse_text( ".grammar", lesson->grammar_texts );
						}
						if( texts.size() )
						{
							//TextView text_view( *ft, library, *texts[0], &config );
							//text_view.run_until_exit();
						} else throw ERROR( "Keine Grammatik für diese Lektion vorhanden" );
						break;
					}
					case LessonMenuChoice::CONTENT_TYPE_TEXT:
					{
						Lesson* lesson = lesson_menu_choice.lesson;
						if( !lesson )
							throw ERROR( "LessonMenu returned no lesson" );
						TextVector& texts = lesson_menu_choice.lesson->lesson_texts;
						if( !texts.size() )
						{
							lesson->parse_text( ".text", lesson->lesson_texts );
						}
						if( texts.size() )
						{
							//TextView text_view( *ft, library, *texts[0], &config );
							//text_view.run_until_exit();
						} else throw ERROR( "Kein Text für diese Lektion vorhanden" );
						break;
					}
					case LessonMenuChoice::CONTENT_TYPE_EXERCISES:
					{
						Lesson* lesson = lesson_menu_choice.lesson;
						if( !lesson )
							throw ERROR( "LessonMenu returned no lesson" );
						TextVector& texts = lesson_menu_choice.lesson->exercises;
						if( !texts.size() )
						{
							lesson->parse_text( ".exercise", lesson->exercises );
						}
						if( texts.size() )
						{
							//TextView text_view( *ft, library, *texts[0], &config );
							//text_view.run_until_exit();
						} else throw ERROR( "Keine Übung für diese Lektion vorhanden" );
						break;
					}
					case LessonMenuChoice::CONTENT_TYPE_SEARCH:
					{
						FulltextSearch fulltext_search( ui_language, *ft, library );
						fulltext_search.run_until_exit();
						break;
					}
					default: throw ERROR( "LessonMenu returned invalid content type" );
				}
			}
			catch( Error& e )
			{
				RenderScreen error_screen;
				ft->init_screen( SCREEN_MAIN, error_screen );
				error_screen.clear();
				ft->render( error_screen, e.what(), ft->latin_face, 8, 3, 3 );
				error_stream << "caught " << e.what() << std::endl;
				for( int i=0; i<500; i++ ) swiWaitForVBlank();
			}
		}
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
