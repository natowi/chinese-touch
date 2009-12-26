#ifndef UNICODE_H
#define UNICODE_H

#include <list>

class UCChar
{
public:
	unsigned long code_point;
	int source_offset;
	int source_length;
};
typedef std::list<UCChar> UCCharList;

bool utf8_to_ucs4( const unsigned char* src, UCCharList& result_list );

#endif // UNICODE_H

