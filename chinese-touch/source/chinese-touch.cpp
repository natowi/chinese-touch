#include <fstream>
#include <string.h>
#include <iomanip>
#include <map>

#include <nds.h>
#include <fat.h>

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

		Dictionary dictionary;

		LOG( "initializing library" );
		Library library( global_fat_initialized, dictionary );
		LOG( "scanning library..." );
		library.rescan();
		LOG( "scanning complete" );
		if( !library.size() )
		{
			WARN( "empty library" );
			std::string book_name = "dummy book";
			library[ book_name ] = new Book( book_name, &library);
		}
		Book& book = *library.begin()->second;
		if( !book.size() )
		{
			WARN( "empty book \"" <<  book.name << "\"" );
			int lesson_number = 1;
			book[ lesson_number ] = new Lesson( lesson_number, &book );
			book[ lesson_number ]->new_words.push_back( new NewWord("foo","bar",book[lesson_number]) );
			book[ lesson_number ]->new_words.push_back( new NewWord("testi","test",book[lesson_number]) );
			book[ lesson_number ]->new_words.push_back( new NewWord("abc","xyz",book[lesson_number]) );
		}

		LOG( "initializing Freetype" );
		FreetypeRenderer* ft = new FreetypeRenderer( "ukai.ttc", "VeraSe.ttf" );

		Config config;
		LOG( "loading config" );
		config.load();

#if 0
		NewWordsViewer* new_words = new NewWordsViewer( *ft, *(book.begin()->second), config );
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
				/* Testlauf des Lektionsauswahlmenüs: */
				LOG( "initializing lesson menu" );
				LessonMenu* lesson_menu = new LessonMenu( *ft, library, config );
				LessonMenuChoice lesson_menu_choice;
				lesson_menu->run_for_user_choice( lesson_menu_choice );
				delete lesson_menu;

				/* Testlauf des Vokabeltrainers: */
				if( lesson_menu_choice.lesson )
				{
					switch( lesson_menu_choice.content_type )
					{
						case LessonMenuChoice::CONTENT_TYPE_NEW_WORDS:
						{
							NewWordsViewer* new_words = new NewWordsViewer( *ft, *lesson_menu_choice.lesson, config );
							new_words->run_until_exit();
							delete new_words;
							break;
						}
						case LessonMenuChoice::CONTENT_TYPE_GRAMMAR:
						{
							TextVector& texts = lesson_menu_choice.lesson->grammar_texts;
							if( texts.size() )
							{
								TextView text_view( *ft, config, *texts[0], dictionary );
								text_view.run_until_exit();
							} else throw ERROR( "Keine Grammatik für diese Lektion vorhanden" );
							break;
						}
						case LessonMenuChoice::CONTENT_TYPE_TEXT:
						{
							TextVector& texts = lesson_menu_choice.lesson->lesson_texts;
							if( texts.size() )
							{
								TextView text_view( *ft, config, *texts[0], dictionary );
								text_view.run_until_exit();
							} else throw ERROR( "Kein Text für diese Lektion vorhanden" );
							break;
						}
						case LessonMenuChoice::CONTENT_TYPE_EXERCISES:
						{
							TextVector& texts = lesson_menu_choice.lesson->exercises;
							if( texts.size() )
							{
								TextView text_view( *ft, config, *texts[0], dictionary );
								text_view.run_until_exit();
							} else throw ERROR( "Keine Übung für diese Lektion vorhanden" );
							break;
						}
						default: throw ERROR( "LessonMenu returned invalid content type!" );
					}
				}
				else throw ERROR( "LessonMenu returned no lesson!" );
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
