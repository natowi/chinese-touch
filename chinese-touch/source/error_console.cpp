#include <nds.h>
#include <iostream>
#include <fstream>

#include "error_console.h"
#include "config.h"
#include "chinese-touch.h"


std::stringstream error_stream;


const char* Error::what()
{
	std::stringstream _full_msg;
	std::size_t right_slash_pos = this->file.rfind('/');
	std::string _file;
	if( right_slash_pos != this->file.npos 
		&& right_slash_pos < this->file.size()-1 )
	{
		_file = this->file.substr( right_slash_pos+1 );
	}
	_full_msg << "Error: " << *this << " (" << _file << ":" << this->line << " in " << this->function << ") [" << this->timestamp << "]";
	this->full_msg = _full_msg.str();
	return this->full_msg.c_str();
}


ErrorConsole::ErrorConsole(Program& _program)
	: log_file_name( _program.base_dir+"/"+_program.name+".log" )
{
}

void ErrorConsole::init_screen( Screen screen )
{
	switch( screen )
	{
		case SCREEN_SUB: 
		{
			videoSetModeSub( MODE_5_2D );
			vramSetBankC( VRAM_C_SUB_BG );
			BG_PALETTE_SUB[0] = 0;
			consoleDemoInit();
			//consoleSelect( consoleGetDefault() );
			error_stream << "sub console intialized" << std::endl;
			break; 
		}
		case SCREEN_MAIN: 
		{
			videoSetMode(MODE_5_2D | DISPLAY_SCREEN_BASE(2));
			vramSetBankB(VRAM_B_MAIN_BG);
			BG_PALETTE[0] = 0;
			PrintConsole* topScreen = consoleInit( 0, 0, BgType_Text4bpp, BgSize_T_256x256, 4, 8, true, true );
			topScreen->fontBgMap = BG_MAP_RAM(4 + 64);
			//consoleSelect( topScreen );
			error_stream << "main console intialized" << std::endl;
			break;
		}
		case SCREEN_NONE:
		{
			break;
		}
	}
	std::cout << error_stream.str();
}

void ErrorConsole::dump()
{
	std::ofstream logfile( this->log_file_name.c_str(), std::ios_base::out|std::ios_base::app );
	std::string errors = error_stream.str();
	logfile.write( errors.c_str(), errors.length() );
	logfile.close();
	ErrorConsole::clear();
	LOG( "logs dumped" );
}

void ErrorConsole::clear()
{
	error_stream.str(""); error_stream.clear();
	LOG( "log buffer cleared" );
}
