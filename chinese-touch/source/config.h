#ifndef CONFIG_H
#define CONFIG_H

#include "lesson.h"

#define PROGRAM_NAME "chinese-touch"
#define PROGRAM_VERSION "1.0.1"
#define CONFIG_STRING_SIZE 50
#define BASE_DIR "/" PROGRAM_NAME "/"
#define CONFIG_FILE_NAME BASE_DIR PROGRAM_NAME ".conf"
#define LOG_FILE_NAME BASE_DIR PROGRAM_NAME ".log"

extern bool global_fat_initialized;

union ConfigData
{
    struct
    {
        char current_book_name[CONFIG_STRING_SIZE];
        int current_lesson_number;
        int current_word_number;
    } config;
    char sugar_cube[4096];
};

class Config
{
public:
    Config();
    void load();
    void save();
    void save_position( NewWord* word, unsigned int number );
	void save_position( Lesson* lesson );
    std::string get_current_book_name() { return this->data.config.current_book_name; }
    int get_current_lesson_number() { return this->data.config.current_lesson_number; }
    int get_current_word_number() { return this->data.config.current_word_number; }
protected:
    void save_really();
protected:
    static const int MIN_SAVE_PERIOD = 5;
    ConfigData data;
    NewWord* previous_word;
    bool changed;
    time_t prev_time;
};

#endif //CONFIG_H
