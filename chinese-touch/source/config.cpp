#include <time.h>
#include <iostream>

#include "config.h"
#include "error_console.h"
#include "chinese-touch.h"


Config::Config()
 : db(0)
{
}

void Config::open( const std::string& file_name, bool create_db )
{
	if( this->db ) throw ERROR( "Settings DB ("+file_name+") already opened or not initialized" );
	int rc;
	int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;
	if( create_db ) flags |= SQLITE_OPEN_CREATE;
	LOG( "opening: " << file_name );
	if( (rc = sqlite3_open_v2( file_name.c_str(), &db, flags, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << ": " << rc;
		this->close();
		throw ERROR( msg.str() );
	}
}

void Config::create( const std::string& file_name )
{
	this->open( file_name, true );
	typedef std::list<std::string> StringList;
	StringList create_statements;
	create_statements.push_back( "CREATE TABLE settings (key TEXT, value TEXT)" );
	create_statements.push_back( "CREATE INDEX i_settings_key on settings(key ASC)" );
	
	for( StringList::iterator i=create_statements.begin(); i!=create_statements.end(); i++ )
	{
		int rc;
		if( (rc = sqlite3_exec(db, i->c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << *i;
			throw ERROR( msg.str() );
		}
	}
}

void Config::close()
{
	sqlite3_close( db );
	db = 0;
}

void Config::set( const std::string& name, int number )
{
	std::stringstream text;
	text << number;
	this->set( name, text.str() );
}

void Config::set( const std::string& name, const std::string& text )
{
	int rc;
	std::string stmt = "select value from settings where key=\"" + name + "\"";
	MapList result;
	if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( result.size() )
	{
		stmt = "update settings set value=\"" + replace_pattern(text,"\"","\"\"") + "\" where key=\"" + name + "\"";
		MapList result;
		if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
	}
	else
	{
		stmt = "insert into settings (key,value) values (\"" + name + "\", \"" + replace_pattern(text,"\"","\"\"") + "\")";
		MapList result;
		if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
	}
}

void Config::set( const std::string& name, const StringList& list )
{
	std::stringstream text;
	for( StringList::const_iterator i = list.begin(); i != list.end(); i++ )
	{
		if( i != list.begin() ) text << ",";
		text << *i;
	}
	this->set( name, text.str() );
}

int Config::get( const std::string& name, int default_value )
{
	int result;
	std::stringstream text;
	std::stringstream default_text;
	default_text << default_value;
	text << this->get( name, default_text.str() );
	text >> result;
	return result;
}

std::string Config::get( const std::string& name, const std::string& default_value )
{
	int rc;
	std::string stmt = "select value from settings where key=\"" + name + "\"";
	MapList result;
	if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( result.size() )
	{
		return (*result.begin())["value"];
	}
	else
	{
		return default_value;
	}
}

void Config::get( const std::string& name, StringList& list, const StringList& default_value )
{
	std::string text = this->get( name, "" );
	if( text.length() )
	{
		list.clear();
		std::string::size_type prev_pos = 0;
		for( std::string::size_type pos = 0; pos != text.npos; prev_pos = pos, pos = text.find(',',pos) )
		{
			if( pos==0 ) continue;
			std::string item = text.substr( prev_pos, pos-prev_pos );
			if( item.length() ) list.push_back( item );
		}
	}
	else list = default_value;
}

void Config::save_word_position( NewWord* word )
{
	int word_id = 0;
	if( word )
	{
		word_id = word->id;
	}
	int old_id = this->get( "current_word_id", 0 );
	if( old_id != word_id )
	{
		this->set( "current_word_id", word_id );
	}
}

void Config::save_position( NewWord* word )
{
	if( word )
	{
		this->save_position( word->lesson );
	}
	this->save_word_position( word );
}

void Config::save_position( Lesson* lesson )
{
	int lesson_number = 0;
	if( lesson )
	{
		lesson_number = lesson->number;
		this->save_position( lesson->book );
	}
	int old_number = this->get( "current_lesson_number", 0 );
	if( old_number != lesson_number )
	{
		this->save_word_position( (NewWord*)0 );
		this->set( "current_lesson_number", lesson_number );
	}
}

void Config::save_position( Book* book )
{
	std::string book_name = "";
	if( book )
	{
		book_name = book->name;
	}
	std::string old_name = this->get( "current_book_name", "" );
	if( old_name != book_name )
	{
		this->save_word_position( (NewWord*)0 );
		this->save_position( (Lesson*)0 );
		this->set( "current_book_name", book_name );
	}
}
