#ifndef ERROR_CONSOLE_H
#define ERROR_CONSOLE_H

#include <exception>
#include <iostream>
#include <sstream>

#include "chinese-touch.h"
#include "screen.h"

extern std::stringstream error_stream;

#define WARN(a) error_stream << "WARNING: " << a << std::endl
#if DEBUG
#define LOG(a) error_stream << a << std::endl
#else
#define LOG(a)
#endif

#define ERROR(x) Error( x, __FILE__, __LINE__, __TIMESTAMP__ )
class Error : public std::exception, public std::string
{
public:
	std::string file;
	int line;
	std::string timestamp;
	std::string full_msg;
public:
	Error( const char* msg, const char* _file, int _line, const char* _timestamp ) 
		: std::string(msg), file(_file), line(_line), timestamp(_timestamp) {}
	Error( const std::string& msg, const char* _file, int _line, const char* _timestamp ) 
		: std::string(msg), file(_file), line(_line), timestamp(_timestamp) {}
	virtual ~Error() throw () {}
	virtual const char* what();
};

class ErrorConsole
{
public:
    static void init_screen( Screen=SCREEN_SUB );
	static void dump();
	static void clear();
};

#endif
