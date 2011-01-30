#ifndef CHINESE_TOUCH_H
#define CHINESE_TOUCH_H

#ifndef DEBUG
#define DEBUG 0
#endif

#include "ui_language.h"
#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"
#include "error_console.h"


class Program
{
public:
	UILanguage* ui_lang;
	FreetypeRenderer* ft;
	WordsDB* words_db;
	Library* library;
	Config* config;
	ErrorConsole* error_console;
	std::string name, version, fs_type, image_path, base_dir, words_db_name;
public:
	Program( int argc, char* argv[] );
	void initialize();
	void run();
};

#endif // CHINESE_TOUCH_H
