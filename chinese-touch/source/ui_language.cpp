#include "ui_language.h"
#include "unicode.h"

UILanguage::UILanguage( const std::string& language )
{
	// TODO open required ui language database from /chinese-touch/ui_lang/
	// TODO load language specific keyboard characters from database
	std::string keyboard_characters_utf8 = "<abcdefghijklmnopqrstuvwxyzü¯´ˇ` ";
	utf8_to_utf8_char_list( (const unsigned char*)keyboard_characters_utf8.c_str(), this->keyboard_characters );
}


std::string UILanguage::get_text_by_file_and_english( const char* file, const char* english )
{
	// TODO: query interface text from database
	return english;
}

std::string UILanguage::get_text_by_english( const char* english )
{
	return this->get_text_by_file_and_english( "any", english );
}
