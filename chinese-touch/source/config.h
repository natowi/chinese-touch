#ifndef CONFIG_H
#define CONFIG_H

#include <sqlite3.h>
#include "lesson.h"


class Config
{
public:
    Config();
	void open( const std::string& file_name, bool create_db = false );
	void create( const std::string& file_name );
	void close();
	// generic setters and getters:
	void set( const std::string& name, int number );
	void set( const std::string& name, const std::string& text );
	void set( const std::string& name, const StringList& list );
	int get( const std::string& name, int default_value = 0 );
	std::string get( const std::string& name, const std::string& default_value = "" );
	void get( const std::string& name, StringList& list, const StringList& default_value = StringList() );
	// FIXME: some more complex setters, that should be moved to other classes:
	void save_word_position( NewWord* word );
	void save_position( NewWord* word );
	void save_position( Lesson* lesson );
	void save_position( Book* book );
protected:
protected:
	sqlite3* db;
};

#endif //CONFIG_H
