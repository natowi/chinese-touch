#ifndef WORDS_DB_H
#define WORDS_DB_H

#include <sqlite3.h>
#include <string>

#include "lesson.h"


class WordsDB
{
private:
	static sqlite3* db;
public:
	static void open( bool create_db=false );
	static void create();
	static void close();
	static void update();
	static int get_book_id( Book& book, bool add_missing );
	static int get_lesson_id( Lesson& lesson, bool add_missing );
	static void add_or_write_word( NewWord& );
	static void write_word( NewWord& );
	static bool read_word( NewWord& );
	static void query_words( Library& library, const std::string& condition, NewWordList& result_list, const std::string& ordering="" );
	static int get_file_id( const std::string& file_path );
	static int get_file_mtime( const std::string& file_path );
	static void set_file_mtime( const std::string& file_path, int new_mtime );
};

#endif