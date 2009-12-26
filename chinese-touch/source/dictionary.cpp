#include "dictionary.h"
#include "lesson.h"
#include "error_console.h"


void Dictionary::add_new_word( NewWord* new_word )
{
	this->new_words_by_word_string[ new_word->hanzi ].insert( new_word );
	UCCharList char_list;
	utf8_to_ucs4( (const unsigned char*)new_word->hanzi.c_str(), char_list );
	for( UCCharList::iterator char_it = char_list.begin(); char_it != char_list.end(); char_it++ )
	{
		this->new_words_by_char_code[ char_it->code_point ].insert( new_word );
	}
}

void Dictionary::find_words_by_char_code( unsigned long char_code, NewWordSet& result )
{
	result.clear();
	if( this->new_words_by_char_code.count(char_code) )
	{
		result = this->new_words_by_char_code[ char_code ];
	}
	return;
}

void Dictionary::find_words_by_context( const std::string& text, const UCCharList& search_list, 
		UCCharList::const_iterator pos, int max_range, NewWordSet& result )
{
	result.clear();
	typedef std::list<UCCharList::const_iterator> PosList;
	PosList starts, ends;
	UCCharList::const_iterator start = pos;
	UCCharList::const_iterator end = pos; end++;
	bool first_start_inserted = false;
	bool final_end_inserted = false;
	for( int range=0; 
		range<max_range && start!=search_list.end(); 
		range++ )
	{
		if( start!=search_list.begin() )
		{
			starts.push_back( start );
			start--;
		}
		else if( !first_start_inserted )
		{
			starts.push_back( start );
			first_start_inserted = true;
		}
		if( end!=search_list.end() )
		{
			ends.push_back( end );
			end++;
		}
		else if( !final_end_inserted )
		{
			ends.push_back( end );
			final_end_inserted = true;
		}
	}
	for( PosList::iterator start_it = starts.begin();
		start_it != starts.end();
		start_it++ )
	{
		for( PosList::iterator end_it = ends.begin();
			end_it != ends.end();
			end_it++ )
		{
			std::string pattern;
			for( UCCharList::const_iterator char_it = *start_it; char_it != *end_it; char_it++ )
			{
				pattern += text.substr( char_it->source_offset, char_it->source_length );
			}
			WARN( "pattern: \"" << pattern << "\"" );
			NewWordsByString::iterator word_it=this->new_words_by_word_string.find(pattern);
			if( word_it	!= this->new_words_by_word_string.end() )
			{
				result.insert( word_it->second.begin(), word_it->second.end() );
			}
		}
	}
}
