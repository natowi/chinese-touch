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
	create_statements.push_back( "CREATE TABLE words (id INTEGER PRIMARY KEY, word TEXT, lesson_id NUMERIC, duplicate_id NUMERIC, type TEXT, pronunciation TEXT, definition TEXT, comment TEXT, rating NUMERIC, atime NUMERIC, file_id NUMERIC, file_offset NUMERIC)" );
	create_statements.push_back( "CREATE INDEX i_words_word ON words (word)" );
	create_statements.push_back( "CREATE TABLE books (id INTEGER PRIMARY KEY, path TEXT)" );
	create_statements.push_back( "CREATE TABLE lessons (id INTEGER PRIMARY KEY, book_id NUMERIC, number NUMERIC)" );
	create_statements.push_back( "CREATE TABLE files (id INTEGER PRIMARY KEY, path TEXT, mtime NUMERIC)" );
	
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

#define MAX_RESULT_SIZE 100

typedef std::list<int> IntList;
static int int_list_callback( void *pv_int_list, int argc, char **argv, char **azColName )
{
	IntList* int_list = static_cast<IntList*>(pv_int_list);
	if( int_list->size()>=MAX_RESULT_SIZE )
		return 1;
	if( argc >= 1 ) int_list->push_back( atoi(argv[0]) );
	return 0;
}

typedef std::map<std::string,std::string> StringMap;
typedef std::list<StringMap> MapList;
static int map_list_callback( void *pv_map_list, int argc, char **argv, char **azColName )
{
	MapList* map_list = static_cast<MapList*>(pv_map_list);
	if( map_list->size()>=MAX_RESULT_SIZE )
		return 1;
	StringMap row_map;
	for( int i=0; i<argc; i++ )
	{
		if( argv[i] ) row_map[ azColName[i] ] = argv[i];
		else row_map[ azColName[i] ] = "";
	}
	map_list->push_back( row_map );
	return 0;
}

/*! Implements conditional database update procedures from previous releases */
void WordsDB::update()
{
	{
		/*! Update from 1.1 to 1.2 requires creating of the files table and 
			the file_id and file_offset columns in the words table */
		int rc;
		std::string stmt = "select name from sqlite_master where name=\"files\"";
		MapList result;
		if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
		if( !result.size() )
		{
			stmt = "CREATE TABLE files (id INTEGER PRIMARY KEY, path TEXT, mtime NUMERIC)";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "alter table words add column file_id NUMERIC default 0";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "alter table words add column file_offset NUMERIC default 0";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
		}
		stmt = "CREATE INDEX IF NOT EXISTS i_words_word ON words (word)";
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}	
	}
}

int WordsDB::get_book_id( Book& book, bool add_missing )
{
	IntList int_list;
	std::string statement = "select id from books where path = \""+book.name+"\"";
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

void WordsDB::add_or_write_word( NewWord& new_word )
{
	MapList map_list;
	if( !new_word.lesson )
	{
		WARN( "Word has no lesson reference: \""+new_word.hanzi+"\"" );
		return;
	}
	std::stringstream statement_stream;
	statement_stream << "select id, pronunciation, type, definition, comment, rating, atime, file_id, file_offset from words"
		<< " where word=\"" << replace_pattern(new_word.hanzi,"\"","\"\"") << "\""
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
		statement_stream << "insert into words "
				<< "(word, pronunciation, type, definition, comment, rating, lesson_id, duplicate_id, atime, file_id, file_offset)"
			<< " values (" 
			<< "\"" << replace_pattern(new_word.hanzi,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.pinyin,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.definitions["de"]->word_type,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.definitions["de"]->translation,"\"","\"\"") << "\", "
			<< "\"" << replace_pattern(new_word.definitions["de"]->comment,"\"","\"\"") << "\", "
			<< new_word.rating << ", "
			<< new_word.lesson->id << ", "
			<< new_word.duplicate_id << ", "
			<< new_word.atime << ", "
			<< new_word.file_id << ", "
			<< new_word.file_offset << ")";
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
			|| atol(row_map["atime"].c_str()) != new_word.atime 
			|| atoi(row_map["file_id"].c_str()) != new_word.file_id 
			|| atoi(row_map["file_offset"].c_str()) != new_word.file_offset 
		)
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
		<< ", comment=\"" << replace_pattern(new_word.definitions["de"]->comment,"\"","\"\"") << "\"";
	if( new_word.rating!=RATING_ANY && new_word.rating!=RATING_NONE )
		statement_stream << ", rating=" << new_word.rating;
	statement_stream << ", atime=" << new_word.atime;
	if( new_word.file_id )
		statement_stream << ", file_id=" << new_word.file_id;
	statement_stream << ", file_offset=" << new_word.file_offset;
	statement_stream
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
	if( !new_word.lesson )
	{
		WARN( "Word has no lesson reference: \""+new_word.hanzi+"\"" );
		return false;
	}
	std::stringstream statement_stream;
	statement_stream << "select id, pronunciation, type, definition, comment, rating, atime, file_id, file_offset from words"
		<< " where word=\"" << replace_pattern(new_word.hanzi,"\"","\"\"") << "\""
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
		new_word.atime = atol( row_map["atime"].c_str() );
		new_word.file_id = atoi( row_map["file_id"].c_str() );
		new_word.file_offset = atoi( row_map["file_offset"].c_str() );
		return true;
	}
	return false;
}

void WordsDB::query_words( Library& library, const std::string& condition, NewWordList& result_list, const std::string& ordering )
{
	MapList map_list;
	std::stringstream statement_stream;
	statement_stream << "select words.id as id, word, pronunciation, type, definition, comment"
			<< ", rating, lesson_id, duplicate_id, atime, book_id, lessons.number as lesson_number"
			<< ", file_id, file_offset, books.path as book_path"
		<< " from words left join lessons on lesson_id=lessons.id"
					<< " left join books on book_id=books.id"
		<< " where " << condition;
	if( ordering.length() )
		statement_stream << " order by " << ordering;
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
		std::string book_path = (*i)["book_path"];
		int lesson_number = atoi( (*i)["lesson_number"].c_str() );
		if( library.count(book_path) )
		{
			Book* book = library[book_path];
			if( book && book->count(lesson_number) )
			{
				Lesson* lesson = (*book)[lesson_number];
				word->lesson = lesson;
			} else WARN( "invalid book pointer or unknown lesson number: " << lesson_number );
		} else WARN( "unknown book path: " << book_path );
		word->duplicate_id = atoi( (*i)["duplicate_id"].c_str() );
		word->file_id = atoi( (*i)["file_id"].c_str() );
		word->file_offset = atoi( (*i)["file_offset"].c_str() );
		result_list.push_back( word );
	}
}

int WordsDB::get_file_id( const std::string& file_path )
{
	std::string stmt = "select id from files where path='" + file_path + "'";
	MapList map_list;
	int rc;
	if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &map_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( map_list.size()>1 )
	{
		throw ERROR( "Multiple entries for file_path: " + file_path );
	}
	for( MapList::iterator i=map_list.begin(); i!=map_list.end(); i++ )
	{
		return atoi( (*i)["id"].c_str() );
	}
	return 0;
}

int WordsDB::get_file_mtime( const std::string& file_path )
{
	std::string stmt = "select mtime from files where path='" + file_path + "'";
	MapList map_list;
	int rc;
	if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &map_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( map_list.size()>1 )
	{
		throw ERROR( "Multiple entries for file_path: " + file_path );
	}
	for( MapList::iterator i=map_list.begin(); i!=map_list.end(); i++ )
	{
		return atol( (*i)["mtime"].c_str() );
	}
	return 0;
}

/*! Sets mtime entry of files with file_path to new_mtime */
void WordsDB::set_file_mtime( const std::string& file_path, int new_mtime )
{
	int file_id = WordsDB::get_file_id( file_path );
	if( !file_id )
	{
		std::stringstream stmt_stream;
		stmt_stream << "insert into files (path, mtime) values ('" << file_path << "', " << new_mtime << ")";
		std::string stmt = stmt_stream.str();
		int rc;
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
	}
	else
	{
		std::stringstream stmt_stream;
		stmt_stream << "update files set mtime=" << new_mtime << " where id=" << file_id;
		std::string stmt = stmt_stream.str();
		int rc;
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
	}
}