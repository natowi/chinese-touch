#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <nds.h>
#include <fat.h>
#include <dirent.h>
#include <errno.h>
#include <sstream>

#include "lesson.h"
#include "config.h"
#include "error_console.h"
#include "words_db.h"


/*! Scans the directory structure "/chinese-touch/books/\<bookname\>/*" for book and lesson configuration files */
void Library::rescan()
{
    std::string books_path = "/" PROGRAM_NAME "/books";
    DIR* books_dir = opendir( books_path.c_str() );
    if( !books_dir )
    {
        std::stringstream msg;
		msg << "failed to open: " << books_path << " (" << strerror(errno) << ")";
		throw ERROR( msg.str() );
    }
    struct dirent* books_entry;
    while( (books_entry = readdir(books_dir))!=NULL )
    {
        struct stat book_stat;
        std::string book_name = books_entry->d_name;
        std::string book_path = books_path + "/" + book_name;
        if( stat(book_path.c_str(), &book_stat)==-1 )
        {
            LOG( "failed to stat: " << books_entry->d_name );
            LOG( strerror(errno) );
            continue;
        }
        if( S_ISDIR(book_stat.st_mode) && book_name!="." && book_name!=".." )
        {
            std::string lessons_path = books_path + "/" + book_name;
			 /*! book configuration files are found once within each book directory 
				and called \<bookdir\>.conf */
            std::string book_conf_path = lessons_path + "/" + book_name + ".conf";
            struct stat book_conf_stat;
            if( stat(book_conf_path.c_str(), &book_conf_stat)==-1 )
            {
                LOG( "failed to open (read): " << book_conf_path );
                LOG( strerror(errno) );
                continue;
            }
            Book* book = new Book( book_name, this );
            book->parse_config( book_conf_path ); //! \see Book::parse_config
			book->id = WordsDB::get_book_id( *book, true );
            (*this)[ book_name ] = book;
            LOG( "book: " << book_path );
            DIR* lessons_dir = opendir( lessons_path.c_str() );
            if( !lessons_dir )
            {
                LOG( "failed to open: " << lessons_path );
                LOG( strerror(errno) );
                continue;
            }
            struct dirent* lessons_entry;
            while( (lessons_entry = readdir(lessons_dir))!=NULL )
            {
                struct stat lesson_stat;
                std::string lesson_name = lessons_entry->d_name;
                std::string lesson_path = lessons_path + "/" + lesson_name;
                if( stat(lesson_path.c_str(), &lesson_stat)==-1 )
                {
                    LOG( "failed to stat: " << lessons_entry->d_name );
                    LOG( strerror(errno) );
                    continue;
                }
                if( !S_ISDIR(lesson_stat.st_mode) )
                {
                    std::string::size_type ldot_pos = lesson_name.find( '.' );
                    std::string::size_type rdot_pos = lesson_name.rfind( '.' );
                    std::string lesson_extension = "";
                    if( rdot_pos != std::string::npos )
                    {
                        lesson_extension = lesson_name.substr( rdot_pos+1 );
                    }
					std::stringstream lesson_number_stringstream( lesson_name.substr(0, ldot_pos) );
					int lesson_number = -1;
					lesson_number_stringstream >> lesson_number;
					if( lesson_number > 0 )
					{
						LOG( "lesson #" << lesson_number << ": " << lesson_path );
						Lesson* lesson;
						if( book->count(lesson_number) )
						{
							lesson = (*book)[ lesson_number ];
						}
						else
						{
							lesson = new Lesson( lesson_number, book );
							lesson->id = WordsDB::get_lesson_id( *lesson, true );
							(*book)[ lesson_number ] = lesson;
						}
						/*! The following lesson specific configuration files may appear multiple times 
							within each book directory, once for each lesson: */
						/*! - A lessons dictionary table is called \<lesson-number\>.dict */
						if( lesson_extension == "dict" )
						{
							lesson->parse_dictionary( lesson_path, this->dictionary ); //! \see Lesson::parse_dictionary
						}
						/*! - A lessons base configuration file is called \<lesson-number\>.conf */
						else if( lesson_extension == "conf" )
						{
							lesson->parse_config( lesson_path ); //! \see Lesson::parse_config
						}
						/*! - A lessons text file is called \<lesson-number\>.text */
						else if( lesson_extension == "text" )
						{
							lesson->parse_text( lesson_path, lesson->lesson_texts ); //! \see Lesson::parse_text
						}
						/*! - A lessons grammar text file is called \<lesson-number\>.grammar */
						else if( lesson_extension == "grammar" )
						{
							lesson->parse_text( lesson_path, lesson->grammar_texts ); //! \see Lesson::parse_text
						}
						/*! - A lessons excercises text file is called \<lesson-number\>.exercise */
						else if( lesson_extension == "exercise" )
						{
							lesson->parse_text( lesson_path, lesson->exercises ); //! \see Lesson::parse_text
						}
					}
                }
            }
            closedir( lessons_dir );
        }
    }
    closedir( books_dir );
}


/*! Parse a book configuration file, containing various key=value pairs (one per line), defining book properties */
void Book::parse_config( const std::string& conf_file_name )
{
	std::ifstream book_conf_file( conf_file_name.c_str() );
	char line_buffer[1024];
	std::string key, value;
	while( book_conf_file.good() )
	{
		book_conf_file.getline( line_buffer, sizeof(line_buffer) );
		std::string line = line_buffer;
		std::string::size_type equals_pos = line.find( '=' );
		if( equals_pos != std::string::npos )
		{
			key = line.substr( 0, equals_pos );
			value = line.substr( equals_pos+1 );
			//! \verbatim title=Title of the book \endverbatim
			if( key=="title" )
			{
				this->title = value;
			}
			//! \verbatim description=Longer description of the book (one line) \endverbatim
			else if( key=="description" ) 
			{
				this->description = value;
			}
			//! \verbatim author=Name of the auther of the book \endverbatim
			else if( key=="author" ) 
			{
				this->author = value;
			}
			//! \verbatim publisher=Publisher of the book \endverbatim
			else if( key=="publisher" ) 
			{
				this->publisher = value;
			}
			//! \verbatim isbn=<isbn> \endverbatim
			else if( key=="isbn" ) 
			{
				this->isbn = value;
			}
			//! \verbatim year=<yearnum> \endverbatim
			else if( key=="year" ) 
			{
				this->year = atoi( value.c_str() );
			}
		}
	}
}


/*! Parse a lesson configuration file, containing various key=value pairs (one per line), defining lesson properties */
void Lesson::parse_config( const std::string& conf_file_name )
{
	std::ifstream lesson_conf_file( conf_file_name.c_str() );
	char line_buffer[1024];
	std::string key, value;
	while( lesson_conf_file.good() )
	{
		lesson_conf_file.getline( line_buffer, sizeof(line_buffer) );
		std::string line = line_buffer;
		std::string::size_type equals_pos = line.find( '=' );
		if( equals_pos != std::string::npos )
		{
			key = line.substr( 0, equals_pos );
			value = line.substr( equals_pos+1 );
			//! \verbatim title=Title of the lesson \endverbatim
			if( key=="title" )
			{
				this->title = value;
			}
			//! \verbatim description=Longer description of purpose and contents of the lesson (one line) \endverbatim
			else if( key=="description" )
			{
				this->description = value;
			}
		}
	}
}


/*! Parse a dictionary file, containing a table definition in MediaWiki format. 
	You can easily create those tables using OpenOffice and its MediaWiki export filter. 
	Anyway here is an example of the expected plain text format: \n
\verbatim
{|  

...

|- 
| 玩
| wán
| play (the game)
| (n)
| It can't be used like "play the piano" or "play video"... etc.

...

|}
\endverbatim */
void Lesson::parse_dictionary( const std::string& dict_file_name, Dictionary&  dictionary )
{
	struct stat dict_file_stats;
	if( stat( dict_file_name.c_str(), &dict_file_stats)==-1 )
	{
		WARN( strerror(errno) );
		return;
	}
    std::ifstream dict_file( dict_file_name.c_str() );
    char line_buffer[1024];
    std::string hanzi, pinyin;
    Definition definition;
    definition.lang = "de";
    int word_count = 0;
    int column = 0;
	typedef std::map<std::string,int> StringMap;
	StringMap seen_words;
    while( dict_file.good() )
    {
        dict_file.getline( line_buffer, sizeof(line_buffer) );
        std::string line = line_buffer;
        if( line.substr(0, 2) == "|-"  //! "|-" At the beginning of a line marks the begin of a new word row.
			|| line.substr(0, 2) == "|}" ) /*! "|}" At the beginning of a line marks the 
																end of the dictionary table. */
        {
            if( word_count )
            {
                NewWord* word = new NewWord( hanzi, pinyin, this );
                word->definitions[ definition.lang ] = new Definition( definition );
				word->duplicate_id = seen_words.count(word->hanzi) ? seen_words[word->hanzi] : 0;
				word->atime = dict_file_stats.st_mtime;
				seen_words[word->hanzi] = word->duplicate_id+1;
                this->new_words.push_back( word );
				// way too slow to do that here for all words at once:
				// WordsDB::add_or_update_word( *word );
				dictionary.add_new_word( word );
                hanzi = "";
                pinyin = "";
                definition.translation = "";
                definition.word_type = "";
                definition.comment = "";
            }
            word_count++;
            column = 0;
        }
        else if( line.substr(0, 2) == "| " ) /*! "| " The pipe the beginning of a line, followed by a space and 
												arbitrary text defines a column of a word row. */
        {
            column++;
            line = line.substr(2);
        }
		else
		{
			// continuation of multi line field
			// though prepend previous line brake:
			line = "\n" + line;
		}
        switch( column )
        {
		/*! A word rows colums have to appear after the word rows begin marker and 
			before the next words begin marker and before the dictionary tables end marker.
			The columns are expected to appear in the following order: */
		case 1: hanzi.append( line ); break; //! - column 1: The foreign language word
		case 2: pinyin.append( line ); break; //! - column 2: The foreign word pronunciation
		case 3: definition.translation.append( line ); break; //! - column 3: The native language translation
		case 4: definition.word_type.append( line ); break; //! - column 4: A short, usually abbreviated, string, defining the word type
		case 5: definition.comment.append( line ); break; //! - column 5: An additional comment in native language
		/*! Column order is mandatory, but the actual number of columns may differ from the  
			number of meaningful columns described above, though the value of exceeding columns 
			will be ignored. Column values may contain line breaks or being left blank. Pipes and curly 
			brackets should be avoided within column values. */
        }
    }
    dict_file.close();
}


/*! Copy the contents of a lessons text file into an in memory buffer for later use */
void Lesson::parse_text( const std::string& text_file_name, TextVector& container )
{
	std::ifstream text_file( text_file_name.c_str() );
	char buffer[512];
	Text* text = new Text( "none", this );
	container.push_back( text );
	while( text_file.good() )
	{
 		text_file.read( buffer, sizeof(buffer)-1 );
		buffer[ text_file.gcount() ] = 0;
		text->append( buffer );
	}
}
