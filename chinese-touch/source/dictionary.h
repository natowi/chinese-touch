#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <set>
#include <map>
#include <string>

#include "freetype_renderer.h"

class Text;
class NewWord;


class WordOccurrence
{
public:
	Text* text;
	std::string::size_type offset;
public:
	WordOccurrence() : text(0), offset(0) {}
};

typedef std::set<NewWord*> NewWordSet;
typedef std::list<NewWord*> NewWordList;
typedef std::map<std::string,WordOccurrence> OccurrencesByString;
typedef std::map<std::string,NewWordSet> NewWordsByString;
typedef std::map<unsigned long,NewWordSet> NewWordsByULong;

class Dictionary
{
public:
	//OccurrencesByString occurrences_by_string;
	NewWordsByString new_words_by_word_string;
	NewWordsByULong new_words_by_char_code;
public:
	Dictionary() {}
	void add_new_word( NewWord* );
	void find_words_by_char_code( unsigned long char_code, NewWordList& result );
	void find_words_by_context( const std::string& text, const UCCharList& search_list, 
		UCCharList::const_iterator pos, int max_range, NewWordList& result );
};

#endif // DICTIONARY_H