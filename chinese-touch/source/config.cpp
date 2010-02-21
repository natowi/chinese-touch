#include <time.h>
#include <iostream>

#include "config.h"
#include "error_console.h"

bool global_fat_initialized = false;

Config::Config()
 : previous_word(0), changed(false)
{
    this->prev_time = time(0);
    memset( (void*)&this->data, 0, sizeof(this->data) );
}

void Config::save_position( NewWord* word )
{
    if( !word || !word->lesson || !word->lesson->book
		|| (this->previous_word == word && !this->changed) )
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

void Config::save_position( Lesson* lesson )
{
	if( !lesson || !lesson->book
		|| (this->data.config.current_lesson_number == lesson->number 
			&& !this->changed) )
	{
		return;
	}
	this->previous_word = 0;
	this->changed = true;
	this->data.config.current_word_id = 0;
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

void Config::save_position( Book* book )
{
	if( !book 
		|| (strncmp(this->data.config.current_book_name,
					book->name.c_str(),
					CONFIG_STRING_SIZE)==0 
			&& !this->changed) )
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
    if( !global_fat_initialized )
    {
        WARN( "cannot save without fat" );
        return;
    }
    FILE* f = fopen( CONFIG_FILE_NAME, "w" );
    if( f )
    {
        LOG( "writing: " << CONFIG_FILE_NAME );
        fwrite( &this->data, sizeof(this->data), 1, f );
        fflush( f );
        fclose( f );
        this->changed = false;
    }
    else LOG( "error opening: " << CONFIG_FILE_NAME );
}

void Config::load()
{
    if( !global_fat_initialized ) return;
    FILE* f = fopen( CONFIG_FILE_NAME, "r" );
    if( f )
    {
        LOG( "reading: " << CONFIG_FILE_NAME );
        fread( &this->data, sizeof(this->data), 1, f );
        fclose( f );
    }
    else LOG( "error opening: " << CONFIG_FILE_NAME );
}

