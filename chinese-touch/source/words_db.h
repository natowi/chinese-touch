#ifndef WORDS_DB_H
#define WORDS_DB_H

#include <sqlite3.h>
#include <string>

#include "lesson.h"
#include "dictionary.h"


class WordsDB
{
private:
	static sqlite3* db;
public:
	static void open( bool create_db=false );
	static void create();
	static void close();
	static int get_book_id( Book& book, bool add_missing );
	static int get_lesson_id( Lesson& lesson, bool add_missing );
	static void find_words( const std::string& word, NewWordList& result_list, int book_id=0, int lesson_id=0 );
	static void find_words_by_char_code( unsigned long char_code, NewWordList& result_list, int book_id=0, int lesson_id=0 );
	static void find_words_by_context( const std::string& text, const UCCharList& search_list, 
		UCCharList::const_iterator pos, int max_range, NewWordList& result, int book_id=0, int lesson_id=0 );
	static void add_or_write_word( NewWord& );
	static void write_word( NewWord& );
	static bool read_word( NewWord& );
	static void get_words_from_book_by_rating( NewWordList& word_list, Book* book, Rating selected_rating, int max_lesson_number=0 );
};

#endif