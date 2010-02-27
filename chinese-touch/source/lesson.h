#ifndef LESSON_H
#define LESSON_H

#include <list>
#include <vector>
#include <map>
#include <string>

#include "freetype_renderer.h"
#include "dictionary.h"


/*! Stores which parts of dictionary entries shall be displayed on top screen. */
class NewWordRenderSettings
{
	public:
		bool render_hanzi, render_pinyin, render_word_type, render_translation, render_comment;
	public:
		NewWordRenderSettings() : render_hanzi(true), render_pinyin(true), render_word_type(true),
			render_translation(true), render_comment(true) {}
		void toggle_hanzi() { this->render_hanzi = !this->render_hanzi; }
		void toggle_pinyin() { this->render_pinyin = !this->render_pinyin; }
		void toggle_word_type() { this->render_word_type = !this->render_word_type; }
		void toggle_translation() { this->render_translation = !this->render_translation; }
		void toggle_comment() { this->render_comment = !this->render_comment; }
};

class Lesson;

/*! Basically a string with an additional title field and knowledge about the lesson it belongs to. */
class Text : public std::string
{
public:
	std::string title;
	Lesson* lesson;
public:
	Text( const std::string& _title, Lesson* _lesson ) : title(_title), lesson(_lesson) {}
};
//! An array of texts. \see Text
typedef std::vector<Text*> TextVector;

/*! A definition for a foreign language word for a specific native language. */
class Definition
{
public:
    std::string lang, //!< The native language this definition is written in.
		translation, //!< The translation of the foreign word into this native language.
		word_type, //!< A usally abreviated, short text giving the words type.
		comment, //!< An additional comment for this entry in native language.
		example; //!< One or few example sentences in the foreign language.
};
/*! A mapping of ISO language keys to Definition pointers. \see Definition */
typedef std::map<std::string,Definition*> Definitions;

/*! Difficulty rating levels. */
typedef enum
{
	RATING_NONE = 0, //!< no rating given by the user until now.
	RATING_IMPOSSIBLE, //!< very hard to read and/or write.
	RATING_HARD, //!< hard to read and/or write.
	RATING_MEDIUM, //!< medium to read and/or write.
	RATING_EASY, //!< easy to read and/or write.
} Rating;

/*! A foreign language word and several associated properties like translations and so on. */
class NewWord
{
public:
    NewWord( const std::string& _hanzi, const std::string& _pinyin, 
            Lesson* _lesson ) 
        : hanzi(_hanzi), pinyin(_pinyin), lesson(_lesson),
        rating(RATING_NONE), id(0), duplicate_id(0) {};
    void render( FreetypeRenderer& ft, RenderScreen& render_screen, NewWordRenderSettings& render_settings );
public:
    std::string hanzi, //!< Foreign language word.
				pinyin; //!< Foreign language pronunciation.
	/*! Collection of definitions/translations in various native languages, 
		being meant for different native language learners. \see Definition */
    Definitions definitions; 
    Lesson* lesson; //!< Pointer to the lesson this word belongs to.
	Rating rating; //!< User defined difficulty rating.
	int id, //!< This words unique id within the sqlite database (0 if unassociated).
	 /*! This words per-lesson duplication count (0 for first occurrence within one 
		lesson, 1 for second and so on). */
		duplicate_id;
	time_t atime; //!< Last time this words dictionary entry was shown to the user.
};
/*! An array of pointers to NewWord instances. \see NewWord */
typedef std::vector<NewWord*> NewWordVector;

class Book;

/*! A limited subset of texts and dictionary entries within a book. */
class Lesson
{
public:
    Lesson( int _number, Book* _book ) : number(_number), book(_book) {};
    void parse_config( const std::string& lesson_file_name );
    void parse_dictionary( const std::string& lesson_file_name, Dictionary& dictionary );
	void parse_text( const std::string& text_file_name, TextVector& container );
public:
	NewWordVector new_words; //!< The dictionary table of this lesson. \see NewWord
	TextVector lesson_texts; //!< An array of lesson texts. \see Text
	TextVector grammar_texts; //!< An array of the lessons grammar texts. \see Text
	TextVector exercises; //!< An array of the lessons exercises texts. \see Text
    std::string title,	//!< The lessons title.
				description; //!< The lessons short description.
    int number; //!< The number of this lesson within a book.
    Book* book; //!< A pointer to the book this lesson belongs to.
	int id; //!< This lessons unique id within the sqlite database (0 if unassociated).
};

class Library;

/*! A Book is a collection of numbered lessons. \see Lesson */
class Book : public std::map<int,Lesson*>
{
public:
    Book( const std::string& _name, Library* _library ) : name(_name),
        library(_library) {}
    void parse_config( const std::string& book_conf_file_name );
public:
    std::string name, //!< The name of the book is equal to its subdirectory name below /chinese-touch/books/.
		title, //!< The books title.
		description, //!< The books short description.
		author, //!< The books authors name.
		publisher, //!< The name of the books publisher.
		isbn; //!< The books ISBN code.
    int year; //!< The publication year of the book.
    Library* library; //!< A pointer to the library this book belongs to.
	int id; //!< This books unique id within the sqlite database (0 if unassociated).
};

//! A Library is a collection of books. \see Book
class Library : public std::map<std::string,Book*>
{
public:
    Library( bool _fat_initialized, Dictionary& _dictionary ) 
		: fat_initialized(_fat_initialized), dictionary(_dictionary) {}
    void rescan();
protected:
    bool fat_initialized;
	Dictionary& dictionary; //!< A dictionary containing all words found in this library.
};

extern Lesson all_words_lesson; //!< FIXME: Is this still used?
void init_all_words_lesson(); //!< FIXME: Is this still used?

#endif // LESSON_H
