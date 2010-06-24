#ifndef ERROR_CONSOLE_H
#define ERROR_CONSOLE_H

#include <exception>
#include <iostream>
#include <sstream>

#include "screen.h"


extern std::stringstream error_stream;

#define CRITICAL(a) error_stream << "CRITICAL: " << a << std::endl
#define WARN(a) error_stream << "WARNING: " << a << std::endl
#if DEBUG
#define LOG(a) error_stream << a << std::endl
#else
#define LOG(a)
#endif

#define ERROR(x) Error( x, __FILE__, __LINE__, __FUNCTION__, __TIMESTAMP__ )
class Error : public std::exception, public std::string
{
protected:
	std::string full_msg;
public:
	std::string file;
	int line;
	std::string function;
	std::string timestamp;
public:
	Error( const char* msg, const char* _file, int _line, const char* _function, const char* _timestamp ) 
		: std::string(msg), file(_file), line(_line), function(_function), timestamp(_timestamp) {}
	Error( const std::string& msg, const char* _file, int _line, const char* _function, const char* _timestamp ) 
		: std::string(msg), file(_file), line(_line), function(_function), timestamp(_timestamp) {}
	virtual ~Error() throw () {}
	virtual const char* what();
};

class Program;

class ErrorConsole
{
public:
	ErrorConsole( Program& _program );
    void init_screen( Screen=SCREEN_SUB );
	void dump();
	void clear();
protected:
	std::string log_file_name;
};

#endif
