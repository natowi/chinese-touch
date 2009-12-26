#include "unicode.h"

bool utf8_to_ucs4( const unsigned char* src, UCCharList& result_list )
{
    if( !src ) return false;
    const unsigned char* first = src;
    while( *first )
    {
        int len = 0;
        unsigned long result = 0;
        if( (*first >> 7) == 0 )
        {
            len = 1;
            result = *first;
        }
        else if( (*first >> 5) == 6 )
        {
            len = 2;
            result = *first & 31;
        }
        else if( (*first >> 4) == 14 )
        {
            len = 3;
            result = *first & 15;
        }
        else if( (*first >> 3) == 30 )
        {
            len = 4;
            result = *first & 7;
        }
        else return false;
        const unsigned char* next;
        for( next = first + 1;
            *next && ((*next >> 6) == 2) && (next-first<len); 
            next++ )
        {
            result = result << 6;
            result |= *next & 63;
        }
		UCChar uc_char;
		uc_char.code_point = result;
		uc_char.source_offset = first-src;
		uc_char.source_length = next-first;
        result_list.push_back( uc_char );
        first = next;
    }
    return true;
}

