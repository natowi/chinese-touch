#ifndef UNICODE_H
#define UNICODE_H

#include <list>
#include <string>

class UCChar
{
public:
	unsigned long code_point;
	int source_offset;
	int source_length;
};
typedef std::list<UCChar> UCCharList;

typedef std::list<std::string> StringList;

bool utf8_to_ucs4( const unsigned char* src, UCCharList& result_list );
bool utf8_to_utf8_char_list( const unsigned char* src, StringList& result_list );

#endif // UNICODE_H

