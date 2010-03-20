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


int main()
{
	defaultExceptionHandler();
	try
	{
		LOG( "initializing fat driver" );
		global_fat_initialized = fatInitDefault();
		if( !global_fat_initialized )
		{
			throw ERROR( "error initializing fat driver" );
		}
		
		
		try
		{
			WordsDB::open();
		}
		catch( Error& e )
		{
			WARN( e.full_msg );
			WordsDB::create();
		}
		WordsDB::update();
		
		Dictionary dictionary;

		LOG( "initializing library" );
		Library library( global_fat_initialized, dictionary );
		LOG( "scanning library..." );
		library.rescan();
		LOG( "scanning complete" );

		LOG( "initializing Freetype" );
		FreetypeRenderer* ft = new FreetypeRenderer( "ukai.ttc", "VeraSe.ttf" );

		Config config;
		LOG( "loading config" );
		config.load();

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
		
		while( true )
		{
			try
			{
				LOG( "initializing lesson menu" );
				LessonMenu* lesson_menu = new LessonMenu( *ft, library, config );
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
						Rating selected_rating = RATING_NONE;
						switch( lesson_menu_choice.content_type )
						{
							case LessonMenuChoice::CONTENT_TYPE_EASY_WORDS:
								selected_rating = RATING_EASY;
								break;
							case LessonMenuChoice::CONTENT_TYPE_MEDIUM_WORDS:
								selected_rating = RATING_MEDIUM;
								break;
							case LessonMenuChoice::CONTENT_TYPE_HARD_WORDS:
								selected_rating = RATING_HARD;
								break;
							case LessonMenuChoice::CONTENT_TYPE_IMPOSSIBLE_WORDS:
								selected_rating = RATING_IMPOSSIBLE;
								break;
							default:
								break;
						}
						NewWordList words;
						WordsDB::get_words_from_book_by_rating( words, lesson_menu_choice.book, selected_rating, 
													  lesson_menu_choice.lesson ? lesson_menu_choice.lesson->number : 0 );
						NewWordsViewer* new_words = new NewWordsViewer( *ft, words );
						if( lesson_menu_choice.lesson ) config.save_position( lesson_menu_choice.lesson, true );
						else config.save_position( lesson_menu_choice.book, true );
						new_words->run_until_exit();
						delete new_words;
						for( NewWordList::iterator i=words.begin(); i!=words.end(); i++ )
							if( *i ) delete *i;
						break;
					}
					case LessonMenuChoice::CONTENT_TYPE_NEW_WORDS:
					{
						Lesson* lesson = lesson_menu_choice.lesson;
						if( !lesson )
							throw ERROR( "LessonMenu returned no lesson" );
						lesson->parse_dictionary_if_needed();
						NewWordList words;
						WordsDB::get_words_from_book_by_rating( words, lesson_menu_choice.book, RATING_ANY, 
							lesson->number, lesson->number, false, true );
						NewWordsViewer* new_words = new NewWordsViewer( *ft, words, &config );
						new_words->run_until_exit();
						delete new_words;
						for( NewWordList::iterator i=words.begin(); i!=words.end(); i++ )
							if( *i ) delete *i;
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
							TextView text_view( *ft, config, *texts[0], dictionary );
							text_view.run_until_exit();
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
							TextView text_view( *ft, config, *texts[0], dictionary );
							text_view.run_until_exit();
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
							TextView text_view( *ft, config, *texts[0], dictionary );
							text_view.run_until_exit();
						} else throw ERROR( "Keine Übung für diese Lektion vorhanden" );
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
