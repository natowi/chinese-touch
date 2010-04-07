#include "fulltext_search.h"
#include "error_console.h"


FulltextSearch::FulltextSearch( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer, Library& _library )
	: TouchKeyboard( _ui_lang, _freetype_renderer ), library(_library)
{
	this->freetype_renderer.init_screen( SCREEN_MAIN, this->word_screen );
	this->word_screen.clear();
}

std::string FulltextSearch::handle_key_pressed( const std::string& input )
{
	return input;
}

void FulltextSearch::handle_text_changed( std::string& written_text )
{
	// query available static book databases
	NewWordList result;
	std::string sql_cond = "pattern='"+written_text+"'";
	for( Library::iterator book_it = this->library.begin(); book_it != this->library.end(); book_it++ )
	{
		if( book_it->second 
			&& book_it->second->dictionary_lesson
			&& book_it->second->static_words_db_path.length() )
		{
			WordsDB* static_db = new WordsDB();
			try
			{
				static_db->open( book_it->second->static_words_db_path );
				try
				{
					static_db->query_static_fulltext( this->library, sql_cond, result, book_it->second->dictionary_lesson );
				}
				catch( Error& e )
				{
					WARN( e.what() );
				}
				static_db->close();
			}
			catch( Error& e )
			{
				WARN( e.what() );
			}
			delete static_db;
		}
	}
	result.sort( hanzi_min_length_sort_predicate );
	if( result.size() )
	{
		NewWordRenderSettings render_settings;
		(*result.begin())->render( this->freetype_renderer, this->word_screen, render_settings, this->library );
	}
}