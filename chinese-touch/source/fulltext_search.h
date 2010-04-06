#ifndef FULLTEXT_SEARCH_H
#define FULLTEXT_SEARCH_H

#include "touch_keyboard.h"


class FulltextSearch : public TouchKeyboard
{
public:
	FulltextSearch( UILanguage& _ui_lang, FreetypeRenderer& _freetype_renderer );
	virtual std::string handle_key_pressed( const std::string& input );
	virtual void handle_text_changed( std::string& written_text );
public:
	RenderScreen word_screen;
};

#endif // FULLTEXT_SEARCH_H