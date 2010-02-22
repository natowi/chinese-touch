#include "words_db.h"
#include "error_console.h"
#include "config.h"


std::string replace_pattern( std::string src, const std::string& pattern, const std::string& replacement )
{
	size_t pos=0;
	while( pos<src.length() && (pos=src.find(pattern,pos))!=src.npos )
	{
		src.replace( pos, pattern.length(), replacement );
		pos+=replacement.length();
	}
	return src;
}


sqlite3* WordsDB::db = 0;

void WordsDB::open( bool create_db )
{
	if( WordsDB::db ) throw ERROR( "WordsDB already opened" );
	int rc;
	int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;
	if( create_db ) flags |= SQLITE_OPEN_CREATE;
	if( (rc = sqlite3_open_v2( WORDS_DB_FILE_NAME, &db, flags, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << ": " << rc;
		WordsDB::close();
		throw ERROR( msg.str() );
	}
}

void WordsDB::create()
{
	WordsDB::open( true );
	typedef std::list<std::string> StringList;
	StringList create_statements;
	create_statements.push_back( "CREATE TABLE words (id INTEGER PRIMARY KEY, word TEXT, lesson_id NUMERIC, duplicate_id NUMERIC, type TEXT, pronunciation TEXT, definition TEXT, comment TEXT, rating NUMERIC, atime NUMERIC)" );
	create_statements.push_back( "CREATE TABLE books (id INTEGER PRIMARY KEY, path TEXT)" );
	create_statements.push_back( "CREATE TABLE lessons (id INTEGER PRIMARY KEY, book_id NUMERIC, number NUMERIC)" );
	
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

void WordsDB::close()
{
	sqlite3_close( db );
	db = 0;
}

typedef std::list<int> IntList;
static int int_list_callback( void *pv_int_list, int argc, char **argv, char **azColName )
{
	IntList* int_list = static_cast<IntList*>(pv_int_list);
	if( argc >= 1 ) int_list->push_back( atoi(argv[0]) );
	return 0;
}

typedef std::map<std::string,std::string> StringMap;
typedef std::list<StringMap> MapList;
static int map_list_callback( void *pv_map_list, int argc, char **argv, char **azColName )
{
	MapList* map_list = static_cast<MapList*>(pv_map_list);
	StringMap row_map;
	for( int i=0; i<argc; i++ )
	{
		if( argv[i] ) row_map[ azColName[i] ] = argv[i];
		else row_map[ azColName[i] ] = "";
	}
	map_list->push_back( row_map );
	return 0;
}

int WordsDB::get_book_id( Book& book, bool add_missing )
{
	IntList int_list;
	std::string statement = "select id from books where path like \""+book.name+"\"";
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), int_list_callback, &int_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
	if( int_list.size()>1 ) throw ERROR( "More than one book with path "+book.name );
	if( !int_list.size() )
	{
		if( add_missing )
		{
			std::string statement = "insert into books (path) values (\""+book.name+"\")";
			int rc;
			if( (rc = sqlite3_exec(db, statement.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
				throw ERROR( msg.str() );
			}
			return WordsDB::get_book_id( book, false );
		}
		else return 0;
	}
	else return *int_list.begin();
}

int WordsDB::get_lesson_id( Lesson& lesson, bool add_missing )
{
	IntList int_list;
	std::stringstream statement_stream;
	statement_stream << "select id from lessons where book_id=" << lesson.book->id << " and number=" << lesson.number;
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), int_list_callback, &int_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
	if( int_list.size()>1 ) throw ERROR( "More than one lesson returned by statement: "+statement );
	if( !int_list.size() )
	{
		if( add_missing )
		{
			std::stringstream statement_stream;
			statement_stream << "insert into lessons (book_id, number) values (" << lesson.book->id << ", " << lesson.number << ")";
			std::string statement = statement_stream.str();
			int rc;
			if( (rc = sqlite3_exec(db, statement.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
				throw ERROR( msg.str() );
			}
			return WordsDB::get_lesson_id( lesson, false );
		}
		else return 0;
	}
	else return *int_list.begin();
}

void WordsDB::find_words( const std::string& word, NewWordList& result_list, int book_id, int lesson_id )
{
}

void WordsDB::find_words_by_char_code( unsigned long char_code, NewWordList& result_list, int book_id, int lesson_id )
{
}

void WordsDB::find_words_by_context( const std::string& text, const UCCharList& search_list, 
		UCCharList::const_iterator pos, int max_range, NewWordList& result, int book_id, int lesson_id )
{
}

void WordsDB::add_or_write_word( NewWord& new_word )
{
	MapList map_list;
	if( !new_word.lesson ) throw ERROR( "Word has no lesson reference: \""+new_word.hanzi+"\"" );
	std::stringstream statement_stream;
	statement_stream << "select id, pronunciation, type, definition, comment, rating, atime from words"
		<< " where word like \"" << replace_pattern(new_word.hanzi,"\"","\"\"") << "\""
		<< " and lesson_id=" << new_word.lesson->id
		<< " and duplicate_id=" << new_word.duplicate_id;
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), map_list_callback, &map_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
	if( map_list.size()>1 ) throw ERROR( "More than one word returned by statement: "+statement );
	if( !map_list.size() && new_word.lesson->id )
	{
		std::stringstream statement_stream;
		statement_stream << "insert into words (word, pronunciation, type, definition, comment, rating, lesson_id, duplicate_id, atime) values (" 
			<< "\"" << replace_pattern(new_word.hanzi,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.pinyin,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.definitions["de"]->word_type,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.definitions["de"]->translation,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.definitions["de"]->comment,"\"","\"\"") << "\", "
			<< new_word.rating << ", "
			<< new_word.lesson->id << ", "
			<< new_word.duplicate_id << ", "
			<< new_word.atime << ")";
		std::string statement = statement_stream.str();
		int rc;
		if( (rc = sqlite3_exec(db, statement.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
			throw ERROR( msg.str() );
		}
	}
	else
	{
		StringMap& row_map = *map_list.begin();
		if( row_map["pronunciation"] != new_word.pinyin
			|| row_map["type"] != new_word.definitions["de"]->word_type
			|| row_map["definition"] != new_word.definitions["de"]->translation
			|| row_map["comment"] != new_word.definitions["de"]->comment
			|| atoi(row_map["rating"].c_str()) != new_word.rating
			|| atol(row_map["atime"].c_str()) != new_word.atime )
		{
			new_word.id = atoi( row_map["id"].c_str() );
			WordsDB::write_word( new_word );
		}
	}
}

void WordsDB::write_word( NewWord& new_word )
{
	std::stringstream statement_stream;
	statement_stream << "update words set"
		<< " pronunciation=\"" << replace_pattern(new_word.pinyin,"\"","\"\"") << "\""
		<< ", type=\"" << replace_pattern(new_word.definitions["de"]->word_type,"\"","\"\"") << "\""
		<< ", definition=\"" << replace_pattern(new_word.definitions["de"]->translation,"\"","\"\"") << "\""
		<< ", comment=\"" << replace_pattern(new_word.definitions["de"]->comment,"\"","\"\"") << "\""
		<< ", rating=" << new_word.rating
		<< ", atime=" << new_word.atime
		<< " where id=" << new_word.id;
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), 0, 0, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
}

bool WordsDB::read_word( NewWord& new_word )
{
	MapList map_list;
	if( !new_word.lesson ) throw ERROR( "Word has no lesson reference: \""+new_word.hanzi+"\"" );
	std::stringstream statement_stream;
	statement_stream << "select id, pronunciation, type, definition, comment, rating, atime from words"
		<< " where word like \"" << replace_pattern(new_word.hanzi,"\"","\"\"") << "\""
		<< " and lesson_id=" << new_word.lesson->id 
		<< " and duplicate_id=" << new_word.duplicate_id;
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), map_list_callback, &map_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
	if( map_list.size()>1 ) throw ERROR( "More than one word returned by statement: "+statement );
	if( map_list.size()==1 )
	{
		StringMap& row_map = *map_list.begin();
		new_word.id = atoi( row_map["id"].c_str() );
		new_word.pinyin = row_map["pronunciation"];
		new_word.definitions["de"]->word_type = row_map["type"];
		new_word.definitions["de"]->translation = row_map["definition"];
		new_word.definitions["de"]->comment = row_map["comment"];
		new_word.rating = (Rating) atoi( row_map["rating"].c_str() );
		new_word.atime = (Rating) atol( row_map["atime"].c_str() );
		return true;
	}
	return false;
}

void WordsDB::get_words_from_book_by_rating( NewWordList& word_list, Book* book, Rating selected_rating, int max_lesson_number )
{
	MapList map_list;
	if( !book ) throw ERROR( "Book undefined" );
	std::stringstream statement_stream;
	statement_stream << "select words.id as id, word, pronunciation, type, definition, comment"
			<< ", rating, lesson_id, duplicate_id, atime, book_id, lessons.number as lesson_number"
		<< " from words inner join lessons on lesson_id=lessons.id"
		<< " where rating=" << selected_rating
		<< " and book_id=" << book->id;
	if( max_lesson_number ) statement_stream << " and lesson_number<=" << max_lesson_number;
	statement_stream << " order by atime,id";
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), map_list_callback, &map_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
	for( MapList::iterator i=map_list.begin(); i!=map_list.end(); i++ )
	{
		NewWord* word = new NewWord( (*i)["word"], (*i)["pronunciation"], 0 );
		word->id = atoi( (*i)["id"].c_str() );
		Definition* def = new Definition();
		def->word_type = (*i)["type"];
		def->translation = (*i)["definition"];
		def->comment = (*i)["comment"];
		def->lang = "de";
		word->definitions[ def->lang ] = def;
		word->rating = (Rating) atoi( (*i)["rating"].c_str() );
		word->atime = atol( (*i)["atime"].c_str() );
		int lesson_number = atoi( (*i)["lesson_number"].c_str() );
		if( book->count(lesson_number) && (*book)[lesson_number] )
		{
			word->lesson = (*book)[lesson_number];
		}
		else 
		{
			delete word;
			throw ERROR( book->name+" has no lesson "+(*i)["lesson_number"] );
		}		
		word_list.push_back( word );
	}
}