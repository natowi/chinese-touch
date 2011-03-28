#ifndef WORDS_DB_H
#define WORDS_DB_H

#include <sqlite3.h>
#include <string>
#include <map>
#include <list>

#include "unicode.h"


class Book;
class Lesson;
class NewWord;
class Library;
class NewWordList;

std::string replace_pattern( std::string src, const std::string& pattern, const std::string& replacement );
std::string replace_patterns( std::string src, StringList& patterns, const std::string& replacement );

typedef std::list<int> IntList;
int int_list_callback( void *pv_int_list, int argc, char **argv, char **azColName );
typedef std::map<std::string,std::string> StringMap;
typedef std::list<StringMap> MapList;
int map_list_callback( void *pv_map_list, int argc, char **argv, char **azColName );

class WordsDB
{
protected:
	sqlite3* db;
public:
	bool update_needed;
	StringList update_reasons;
public:
	WordsDB() : db(0), update_needed(false) {}
	void open( const std::string& file_name, bool create_db = false );
	void create( const std::string& file_name );
	void close();
	bool update();
	int get_book_id( Book& book, bool add_missing );
	int get_lesson_id( Lesson& lesson, bool add_missing );
	void add_or_write_word( NewWord& );
	void write_word( NewWord& );
	bool read_word( NewWord& );
	void delete_word( NewWord& );
	void query_words( Library& library, const std::string& condition, NewWordList& result_list, const std::string& ordering="" );
	void query_static_words( Library& library, const std::string& condition, NewWordList& result_list, Lesson* owner_lesson, const std::string& ordering = "" );
	void query_static_fulltext( Library& library, const StringList& patterns, NewWordList& result_list, Lesson* owner_lesson, const std::string& ordering = "" );
	void add_fulltext_patterns( NewWord& word, const StringSet& patterns );
	void clear_fulltext_patterns();
	int get_file_id( const std::string& file_path );
	int get_file_mtime( const std::string& file_path );
	void set_file_mtime( const std::string& file_path, int new_mtime );
	double get_avg_rating();
	double get_avg_rating( Book* book );
	double get_avg_rating( Lesson* lesson );
	int get_max_word_id();
	int count_words();
	int count_words( Book* book );
	int count_words( Lesson* lesson );
	void expire_lesson_words( Lesson* lesson );
protected:
	double get_avg_rating( Book* book, Lesson* lesson );
	int count_words( Book* book, Lesson* lesson );
	void update_words( const std::string& set_clause, const std::string& condition );
};

#endif