#ifndef CHINESE_TOUCH_H
#define CHINESE_TOUCH_H

#ifndef DEBUG
#define DEBUG 0
#endif

#include "ui_language.h"
#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"


class Program
{
public:
	bool first_run;
	UILanguage* ui_lang;
	FreetypeRenderer* ft;
	WordsDB* words_db;
	Library* library;
	Config* config;
public:
	Program();
	void run();
};

#endif // CHINESE_TOUCH_H
