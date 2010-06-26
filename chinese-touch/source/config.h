#ifndef CONFIG_H
#define CONFIG_H

#include "lesson.h"

#define CONFIG_STRING_SIZE 50

union ConfigData
{
    struct
    {
        char current_book_name[CONFIG_STRING_SIZE];
        int current_lesson_number;
        int current_word_id;
    } config;
    char sugar_cube[4096];
};

class Program;

class Config
{
public:
    Config( Program& _program );
    void load();
    void save();
    void save_word_position( NewWord* word, bool force=false );
    void save_position( NewWord* word, bool force=false );
	void save_position( Lesson* lesson, bool force=false );
	void save_position( Book* book, bool force=false );
    std::string get_current_book_name() { return this->data.config.current_book_name; }
    int get_current_lesson_number() { return this->data.config.current_lesson_number; }
    int get_current_word_id() { return this->data.config.current_word_id; }
protected:
    void save_really();
protected:
    static const int MIN_SAVE_PERIOD = 5;
    ConfigData data;
    NewWord* previous_word;
    bool changed;
    time_t prev_time;
	std::string config_file_name;
};

#endif //CONFIG_H
