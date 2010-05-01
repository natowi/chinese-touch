#ifndef CONFIG_H
#define CONFIG_H

#include "lesson.h"

#define PROGRAM_NAME "chinese-touch"
#define PROGRAM_VERSION "1.3"
#define CONFIG_STRING_SIZE 50
#define BASE_DIR "/" PROGRAM_NAME "/"
#define BOOKS_DIR BASE_DIR "books/"
#define CONFIG_FILE_NAME BASE_DIR PROGRAM_NAME ".conf"
#define LOG_FILE_NAME BASE_DIR PROGRAM_NAME ".log"
#define WORDS_DB_FILE_NAME BASE_DIR "words.db"

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

class Config
{
public:
    Config();
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
};

#endif //CONFIG_H
