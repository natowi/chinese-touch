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

std::string replace_patterns( std::string src, StringList& patterns, const std::string& replacement )
{
	std::string result = src;
	for( StringList::iterator pi = patterns.begin(); pi != patterns.end(); pi++ )
	{
		result = replace_pattern( result, *pi, replacement );
	}
	return result;
}


void WordsDB::open( const std::string& file_name, bool create_db )
{
	if( this->db ) throw ERROR( "WordsDB ("+file_name+") already opened or not initialized" );
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

void WordsDB::create( const std::string& file_name )
{
	this->open( file_name, true );
	typedef std::list<std::string> StringList;
	StringList create_statements;
	create_statements.push_back( "CREATE TABLE words (id INTEGER PRIMARY KEY, word TEXT, lesson_id NUMERIC, duplicate_id NUMERIC, type TEXT, pronunciation TEXT, definition TEXT, comment TEXT, rating NUMERIC, atime NUMERIC, file_id NUMERIC, file_offset NUMERIC)" );
	create_statements.push_back( "CREATE INDEX i_words_word ON words (word)" );
	create_statements.push_back( "CREATE TABLE books (id INTEGER PRIMARY KEY, path TEXT)" );
	create_statements.push_back( "CREATE TABLE lessons (id INTEGER PRIMARY KEY, book_id NUMERIC, number NUMERIC)" );
	create_statements.push_back( "CREATE TABLE files (id INTEGER PRIMARY KEY, path TEXT, mtime NUMERIC)" );
	create_statements.push_back( "CREATE TABLE ft_matches (pattern_id NUMERIC, word_id NUMERIC)" );
	create_statements.push_back( "CREATE INDEX i_ft_matches_pattern_id ON ft_matches (pattern_id)" );
	create_statements.push_back( "CREATE INDEX i_ft_matches_word_id ON ft_matches (word_id)" );
	create_statements.push_back( "CREATE TABLE ft_patterns (id INTEGER PRIMARY KEY, pattern TEXT)" );
	create_statements.push_back( "CREATE INDEX i_ft_patterns_pattern ON ft_patterns (pattern)" );
	
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
	
	this->update_needed = true;
	this->update_reasons.push_back( "fresh words.db created");
}

void WordsDB::close()
{
	sqlite3_close( db );
	db = 0;
}

#define MAX_RESULT_SIZE 100

int int_list_callback( void *pv_int_list, int argc, char **argv, char **azColName )
{
	IntList* int_list = static_cast<IntList*>(pv_int_list);
	if( int_list->size()>=MAX_RESULT_SIZE )
		return 0;
	if( argc >= 1 ) int_list->push_back( atoi(argv[0]) );
	return 0;
}

int map_list_callback( void *pv_map_list, int argc, char **argv, char **azColName )
{
	MapList* map_list = static_cast<MapList*>(pv_map_list);
	if( map_list->size()>=MAX_RESULT_SIZE )
		return 0;
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
bool WordsDB::update()
{
	{
		/*! Update from 1.1 to 1.2 requires creation of the files table and 
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
			this->update_needed = true;
			this->update_reasons.push_back( "added support for file change monitoring");
		}
		stmt = "CREATE INDEX IF NOT EXISTS i_words_word ON words (word)";
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}	
	}
	{
		/*! Update from 1.4 to 1.5 requires creation of the ft_matches and 
			ft_patterns tables and corresponding indices for reverse lookups
			on custom word entries */
		int rc;
		std::string stmt = "select name from sqlite_master where name=\"ft_matches\"";
		MapList result;
		if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
		if( !result.size() )
		{
			stmt = "CREATE TABLE ft_matches (pattern_id NUMERIC, word_id NUMERIC)";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "CREATE INDEX i_ft_matches_pattern_id ON ft_matches (pattern_id)";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "CREATE INDEX i_ft_matches_word_id ON ft_matches (word_id)";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "CREATE TABLE ft_patterns (id INTEGER PRIMARY KEY, pattern TEXT)";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "CREATE INDEX i_ft_patterns_pattern ON ft_patterns (pattern)";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			this->update_needed = true;
			this->update_reasons.push_back( "added support for full-text search on custom word entries");
		}
		stmt = "CREATE INDEX IF NOT EXISTS i_words_atime ON words (atime DESC)";
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}	
		stmt = "CREATE INDEX IF NOT EXISTS i_words_lesson_id ON words (lesson_id)";
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}	
		stmt = "CREATE INDEX IF NOT EXISTS i_words_rating ON words (rating DESC)";
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}	
	}
	return this->update_needed;
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
			return this->get_book_id( book, false );
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
			return this->get_lesson_id( lesson, false );
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
		statement = "select max(id) from words";
		IntList int_result;
		if( (rc = sqlite3_exec(db, statement.c_str(), &int_list_callback, &int_result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
			throw ERROR( msg.str() );
		}
		if( int_result.size() )
		{
			new_word.id = *int_result.begin();
		}
		else throw ERROR( "No max(id) in words!?" );
	}
	else
	{
		StringMap& row_map = *map_list.begin();
		if( row_map["pronunciation"] != new_word.pinyin
			|| row_map["type"] != new_word.definitions["de"]->word_type
			|| row_map["definition"] != new_word.definitions["de"]->translation
			|| row_map["comment"] != new_word.definitions["de"]->comment
			|| atoi(row_map["rating"].c_str()) != new_word.rating
			|| atol(row_map["atime"].c_str()) < new_word.atime // atime can only grow
			|| atoi(row_map["file_id"].c_str()) != new_word.file_id 
			|| atoi(row_map["file_offset"].c_str()) != new_word.file_offset 
		)
		{
			new_word.id = atoi( row_map["id"].c_str() );
			// ensure atime growth:
			if( atol(row_map["atime"].c_str()) > new_word.atime ) 
				new_word.atime = atol(row_map["atime"].c_str());
			this->write_word( new_word );
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

void WordsDB::delete_word( NewWord& new_word )
{
	std::stringstream statement_stream;
	statement_stream << "delete from words"
		<< " where id=" << new_word.id;
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), 0, 0, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
	// remove id and lesson reference from word to prevent it from being 
	// re-added by a successive call to add_or_write_word:
	new_word.id = 0;
	new_word.lesson = 0;
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
		// FIXME: support multiple languages
		std::string def_lang = "de";
		if( !new_word.definitions.count(def_lang) )
		{
			Definition* def = new Definition();
			def->lang = def_lang;
			new_word.definitions[ def->lang ] = def;

		}
		new_word.definitions[def_lang]->word_type = row_map["type"];
		new_word.definitions[def_lang]->translation = row_map["definition"];
		new_word.definitions[def_lang]->comment = row_map["comment"];
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
			} 
			else if( lesson_number==0 and book->dictionary_lesson )
			{
				word->lesson = book->dictionary_lesson;
			} 
			else WARN( "invalid book pointer or unknown lesson number: " << lesson_number );
		} else WARN( "unknown book path: " << book_path );
		word->duplicate_id = atoi( (*i)["duplicate_id"].c_str() );
		word->file_id = atoi( (*i)["file_id"].c_str() );
		word->file_offset = atoi( (*i)["file_offset"].c_str() );
		result_list.push_back( word );
	}
}

void WordsDB::query_static_words( Library& library, const std::string& condition, NewWordList& result_list, Lesson* owner_lesson, const std::string& ordering )
{
	MapList map_list;
	std::stringstream statement_stream;
	statement_stream << "select words.id as id, word, pronunciation, type, definition, comment"
			<< ", rating, lesson_id, duplicate_id, atime, file_id, file_offset"
		<< " from words"
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
		NewWord* word = new NewWord( (*i)["word"], (*i)["pronunciation"], owner_lesson );
		word->from_static_db = true;
		word->duplicate_id = atoi( (*i)["duplicate_id"].c_str() );
		bool need_to_synchronize = library.words_db.read_word( *word );
		if( !need_to_synchronize )
		{
			// We do not want to clobber id, rating and atime of words already in the user db:
			word->id = atoi( (*i)["id"].c_str() );
			word->rating = (Rating) atoi( (*i)["rating"].c_str() );
			word->atime = atol( (*i)["atime"].c_str() );
		}
		// FIXME: support multiple languages
		std::string def_lang = "de";
		if( !word->definitions.count(def_lang) )
		{
			Definition* def = new Definition();
			def->lang = def_lang;
			word->definitions[ def_lang ] = def;
		}
		word->definitions[def_lang]->word_type = (*i)["type"];
		word->definitions[def_lang]->translation = (*i)["definition"];
		word->definitions[def_lang]->comment = (*i)["comment"];
		word->file_id = atoi( (*i)["file_id"].c_str() );
		word->file_offset = atoi( (*i)["file_offset"].c_str() );
		
		if( need_to_synchronize )
		{
			// synchronize potentially updated entry from static dictionary to user db:
			library.words_db.write_word( *word );
			// fix an already existing entry of the user db copy in the current result list:
			bool found_previous_result = false;
			for( NewWordList::iterator ri=result_list.begin(); ri!=result_list.end(); ri++ )
			{
				if( (*ri)->id == word->id )
				{
					found_previous_result = true;
					library.words_db.read_word( **ri );
					break;
				}
			}
			if( found_previous_result )
			{
				// prevent duplicates in result lists:
				delete word;
				continue;
			}
		}
		
		result_list.push_back( word );
	}
}

void WordsDB::query_static_fulltext( Library& library, const StringList& patterns, NewWordList& result_list, Lesson* owner_lesson, const std::string& ordering )
{
	MapList map_list;
	std::stringstream statement_stream;
	statement_stream << "select words.id as id, word, pronunciation, type, definition, comment"
			<< ", rating, lesson_id, duplicate_id, atime, file_id, file_offset"
		<< " from words";
	int count = 0;
	std::stringstream condition;
	for( StringList::const_iterator s_it=patterns.begin(); s_it!=patterns.end(); s_it++, count++ )
	{
		statement_stream 
			<< " inner join ft_matches as m" << count 
				<< " on words.id=m" << count << ".word_id"
			<< " inner join ft_patterns as p" << count 
				<< " on p" << count << ".id=m" << count <<".pattern_id";
		if( count ) condition << " and ";
		condition << "p" << count << ".pattern='" << *s_it << "'";
	}
	statement_stream << " where " << condition.str();
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
		Lesson* current_owner_lesson = owner_lesson;
		// try to find an owner lesson in the library if none was given by the caller explicitly:
		if( !current_owner_lesson )
		{
			int owner_lesson_id = atoi( (*i)["lesson_id"].c_str() );
			for( Library::iterator book_it = library.begin(); !current_owner_lesson && book_it != library.end(); book_it++ )
			{
				for( Book::iterator lesson_it = book_it->second->begin(); !current_owner_lesson && lesson_it != book_it->second->end(); lesson_it++ )
				{
					if( lesson_it->second->id == owner_lesson_id )
					{
						current_owner_lesson = lesson_it->second;
					}
				}
			}
		}
		NewWord* word = new NewWord( (*i)["word"], (*i)["pronunciation"], current_owner_lesson );
		word->from_static_db = true;
		word->duplicate_id = atoi( (*i)["duplicate_id"].c_str() );
		bool need_to_synchronize = library.words_db.read_word( *word );
		if( !need_to_synchronize )
		{
			// We do not want to clobber id, rating and atime of words already in the user db:
			word->id = atoi( (*i)["id"].c_str() );
			word->rating = (Rating) atoi( (*i)["rating"].c_str() );
			word->atime = atol( (*i)["atime"].c_str() );
		}
		// FIXME: support multiple languages
		std::string def_lang = "de";
		if( !word->definitions.count(def_lang) )
		{
			Definition* def = new Definition();
			def->lang = def_lang;
			word->definitions[ def_lang ] = def;
		}
		word->definitions[def_lang]->word_type = (*i)["type"];
		word->definitions[def_lang]->translation = (*i)["definition"];
		word->definitions[def_lang]->comment = (*i)["comment"];
		word->file_id = atoi( (*i)["file_id"].c_str() );
		word->file_offset = atoi( (*i)["file_offset"].c_str() );
		
		if( need_to_synchronize )
		{
			// synchronize potentially updated entry from static dictionary to user db:
			library.words_db.write_word( *word );
			// fix an already existing entry of the user db copy in the current result list:
			bool found_previous_result = false;
			for( NewWordList::iterator ri=result_list.begin(); ri!=result_list.end(); ri++ )
			{
				if( (*ri)->id == word->id )
				{
					found_previous_result = true;
					library.words_db.read_word( **ri );
					break;
				}
			}
			if( found_previous_result )
			{
				// prevent duplicates in result lists:
				delete word;
				continue;
			}
		}
		
		result_list.push_back( word );
	}
}

void WordsDB::add_fulltext_patterns( NewWord& word, const StringSet& patterns )
{
	StringList statements;
	for( StringSet::iterator pi=patterns.begin(); pi!=patterns.end(); pi++ )
	{
		// try to find already existing pattern from previous match:
		std::string stmt = "select id from ft_patterns where pattern='"+*pi+"'";
		int rc;
		IntList result;
		if( (rc = sqlite3_exec(db, stmt.c_str(), &int_list_callback, &result, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
		int pattern_id = 0;
		if( result.size() )
		{
			pattern_id = *result.begin();
		}
		else
		{
			// else insert new pattern for this match:
			stmt = "insert into ft_patterns (pattern) values ('"+*pi+"')";
			if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			stmt = "select max(id) from ft_patterns";
			if( (rc = sqlite3_exec(db, stmt.c_str(), &int_list_callback, &result, 0))!=SQLITE_OK )
			{
				std::stringstream msg;
				msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
				throw ERROR( msg.str() );
			}
			if( result.size() )
			{
				pattern_id = *result.begin();
			}
			else continue; // something went wrong, ignore pattern...
		}
		// finally insert match for this pattern:
		std::stringstream stmt_stream;
		stmt_stream << "insert into ft_matches (pattern_id,word_id) values (" << pattern_id << "," << word.id << ")";
		stmt = stmt_stream.str();
		if( (rc = sqlite3_exec(db, stmt.c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
			throw ERROR( msg.str() );
		}
	}
}

void WordsDB::clear_fulltext_patterns()
{
	StringList statements;
	statements.push_back( "delete from ft_matches" );
	statements.push_back( "delete from ft_patterns" );
	for( StringList::iterator si=statements.begin(); si!=statements.end(); si++ )
	{
		int rc;
		if( (rc = sqlite3_exec(db, si->c_str(), 0, 0, 0))!=SQLITE_OK )
		{
			std::stringstream msg;
			msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << *si;
			throw ERROR( msg.str() );
		}
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
	int file_id = this->get_file_id( file_path );
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

double WordsDB::get_avg_rating()
{
	return this->get_avg_rating( 0, 0 );
}

double WordsDB::get_avg_rating( Book* book )
{
	return this->get_avg_rating( book, 0 );
}

double WordsDB::get_avg_rating( Lesson* lesson )
{
	return this->get_avg_rating( 0, lesson );
}

double WordsDB::get_avg_rating( Book* book, Lesson* lesson )
{
	double avg_rating = 0;
	
	std::stringstream ss_stmt;
	ss_stmt << "select avg(rating) as avg_rating from words ";
	if( book ) ss_stmt << "inner join lessons on words.lesson_id=lessons.id ";
	if( book || lesson ) ss_stmt << "where ";
	if( book ) ss_stmt << "book_id=" << book->id << " ";
	if( lesson )
	{
		if( book ) ss_stmt << "and ";
		ss_stmt << "lesson_id=" << lesson->id << " ";
	}
	std::string stmt = ss_stmt.str();
	MapList map_list;
	int rc;
	if( (rc = sqlite3_exec(db, stmt.c_str(), map_list_callback, &map_list, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( map_list.begin() != map_list.end() ) avg_rating = atof( (*map_list.begin())["avg_rating"].c_str() );
	
	return avg_rating;
}

int WordsDB::get_max_word_id()
{
	std::string stmt = "select max(id) from words";
	IntList result;
	int rc;
	if( (rc = sqlite3_exec(db, stmt.c_str(), &int_list_callback, &result, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( result.size() ) return *result.begin();
	else return 0;
}

int WordsDB::count_words()
{
	return this->count_words( 0, 0 );
}

int WordsDB::count_words( Book* book )
{
	return this->count_words( book, 0 );
}

int WordsDB::count_words( Lesson* lesson )
{
	return this->count_words( 0, lesson );
}

int WordsDB::count_words( Book* book, Lesson* lesson )
{
	std::stringstream ss_stmt;
	ss_stmt << "select count(id) from words ";
	if( book ) ss_stmt << "inner join lessons on words.lesson_id=lessons.id ";
	if( book || lesson ) ss_stmt << "where ";
	if( book ) ss_stmt << "book_id=" << book->id << " ";
	if( lesson )
	{
		if( book ) ss_stmt << "and ";
		ss_stmt << "lesson_id=" << lesson->id << " ";
	}
	std::string stmt = ss_stmt.str();
	IntList result;
	int rc;
	if( (rc = sqlite3_exec(db, stmt.c_str(), &int_list_callback, &result, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << stmt;
		throw ERROR( msg.str() );
	}
	if( result.size() ) return *result.begin();
	else return 0;
}


void WordsDB::update_words( const std::string& set_clause, const std::string& condition )
{
	std::stringstream statement_stream;
	statement_stream << "update words set " << set_clause << " where " << condition;
	std::string statement = statement_stream.str();
	int rc;
	if( (rc = sqlite3_exec(db, statement.c_str(), 0, 0, 0))!=SQLITE_OK )
	{
		std::stringstream msg;
		msg << sqlite3_errmsg(db) << " (" << rc << "), in statement: " << statement;
		throw ERROR( msg.str() );
	}
}

void WordsDB::expire_lesson_words( Lesson* lesson )
{
	std::string set_clause = "file_id=0";
	std::stringstream condition_stream;
	condition_stream << "lesson_id=" << lesson->id;
	this->update_words( set_clause, condition_stream.str() );
}
