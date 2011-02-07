#ifndef UI_LANGUAGE_H
#define UI_LANGUAGE_H

#include <string>
#include "unicode.h"

#define I_TEXT( ui, english ) ui->get_text_by_english( english )
#define I_TEXT_F( ui, english ) ui->get_text_by_english( __FILE__, english )

class UILanguage
{
public:
	UILanguage( const std::string& language );
	std::string get_text_by_file_and_english( const char* file, const char* english );
	std::string get_text_by_english( const char* english );
};

#endif // UI_LANGUAGE_H