#include "fulltext_search.h"

FulltextSearch::FulltextSearch( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer )
	: TouchKeyboard( _ui_lang, _freetype_renderer )
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
}