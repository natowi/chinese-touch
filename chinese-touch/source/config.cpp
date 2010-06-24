#include <time.h>
#include <iostream>

#include "config.h"
#include "error_console.h"
#include "chinese-touch.h"


Config::Config( Program& _program )
 : previous_word(0), changed(false),
	config_file_name( _program.base_dir+"/"+_program.name+".conf" )
{
    this->prev_time = time(0);
    memset( (void*)&this->data, 0, sizeof(this->data) );
}

void Config::save_word_position( NewWord* word, bool force )
{
    if( !word
		|| (this->previous_word == word && !this->changed && !force) )
    {
        return;
    }
    this->previous_word = word;
    this->changed = true;
    this->data.config.current_word_id = word->id;
    this->save();
}

void Config::save_position( NewWord* word, bool force )
{
    if( !word || !word->lesson || !word->lesson->book
		|| (this->previous_word == word && !this->changed && !force) )
    {
        return;
    }
    this->previous_word = word;
    this->changed = true;
    this->data.config.current_word_id = word->id;
    this->data.config.current_lesson_number = word->lesson->number;
    const std::string& book_name = word->lesson->book->name;
	memset( (void*)&this->data.config.current_book_name, 0, CONFIG_STRING_SIZE );
    memcpy( this->data.config.current_book_name, 
        book_name.c_str(),
        book_name.length() < CONFIG_STRING_SIZE ? book_name.length() 
                                                : CONFIG_STRING_SIZE );
    this->data.config.current_book_name[CONFIG_STRING_SIZE-1] = 0;
    this->save();
}

void Config::save_position( Lesson* lesson, bool force )
{
	if( !lesson || !lesson->book
		|| (this->data.config.current_lesson_number == lesson->number 
			&& !this->changed && !force) )
	{
		return;
	}
	this->previous_word = 0;
	this->changed = true;
	this->data.config.current_lesson_number = lesson->number;
	const std::string& book_name = lesson->book->name;
	memset( (void*)&this->data.config.current_book_name, 0, CONFIG_STRING_SIZE );
	memcpy( this->data.config.current_book_name, 
			book_name.c_str(),
			book_name.length() < CONFIG_STRING_SIZE ? book_name.length()
													: CONFIG_STRING_SIZE );
	this->data.config.current_book_name[CONFIG_STRING_SIZE-1] = 0;
	this->save();
}

void Config::save_position( Book* book, bool force )
{
	if( !book 
		|| (strncmp(this->data.config.current_book_name,
					book->name.c_str(),
					CONFIG_STRING_SIZE)==0 
			&& !this->changed && !force) )
	{
		return;
	}
	this->previous_word = 0;
	this->changed = true;
	this->data.config.current_word_id = 0;
	this->data.config.current_lesson_number = 0;
	const std::string& book_name = book->name;
	memset( (void*)&this->data.config.current_book_name, 0, CONFIG_STRING_SIZE );
	memcpy( this->data.config.current_book_name, 
			book_name.c_str(),
			book_name.length() < CONFIG_STRING_SIZE ? book_name.length()
													: CONFIG_STRING_SIZE );
	this->data.config.current_book_name[CONFIG_STRING_SIZE-1] = 0;
	this->save();
}

void Config::save()
{
    time_t curr_time = time(0);
    if( curr_time > (this->prev_time + Config::MIN_SAVE_PERIOD)
        && this->changed )
    {
        this->save_really();
        this->prev_time = curr_time;
     }
}

void Config::save_really()
{
    LOG( "Config::save_really()" );
    FILE* f = fopen( this->config_file_name.c_str(), "w" );
    if( f )
    {
        LOG( "writing: " << this->config_file_name );
        fwrite( &this->data, sizeof(this->data), 1, f );
        fflush( f );
        fclose( f );
        this->changed = false;
    }
    else LOG( "error opening: " << this->config_file_name );
}

void Config::load()
{
    FILE* f = fopen( this->config_file_name.c_str(), "r" );
    if( f )
    {
        LOG( "reading: " << this->config_file_name );
        fread( &this->data, sizeof(this->data), 1, f );
        fclose( f );
    }
    else LOG( "error opening: " << this->config_file_name );
}

