#include<iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
typedef std::map<int,const char*> ErrorMap;

#undef __FTERRORS_H__
#define FT_ERROR_START_LIST     ErrorMap ft_errors; void FT_Init_Errors(){
#define FT_ERRORDEF( e, v, s )  ft_errors[e]=s;
#define FT_ERROR_END_LIST       };

#include FT_ERRORS_H

#include <string>
#include <sstream>

#define WARN(a) std::cout << "<!-- WARNING: -->" << a << std::endl
#define LOG(a) std::cout << a << std::endl

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

const char* Error::what()
{
	std::stringstream _full_msg;
	std::size_t right_slash_pos = file.rfind('/');
	std::string _file;
	if( right_slash_pos != file.npos 
		&& right_slash_pos < file.size()-1 )
	{
		_file = file.substr( right_slash_pos+1 );
	}
	_full_msg << "Error: " << *this << " (" << _file << ":" << line << ") [" << timestamp << "]";
	full_msg = _full_msg.str();
	return full_msg.c_str();
}

#include <list>

class UCChar
{
public:
	unsigned long code_point;
	int source_offset;
	int source_length;
};
typedef std::list<UCChar> UCCharList;

bool utf8_to_ucs4( const unsigned char* src, UCCharList& result_list )
{
    if( !src ) return false;
    const unsigned char* first = src;
    while( *first )
    {
        int len = 0;
        unsigned long result = 0;
        if( (*first >> 7) == 0 )
        {
            len = 1;
            result = *first;
        }
        else if( (*first >> 5) == 6 )
        {
            len = 2;
            result = *first & 31;
        }
        else if( (*first >> 4) == 14 )
        {
            len = 3;
            result = *first & 15;
        }
        else if( (*first >> 3) == 30 )
        {
            len = 4;
            result = *first & 7;
        }
        else return false;
        const unsigned char* next;
        for( next = first + 1;
            *next && ((*next >> 6) == 2) && (next-first<len); 
            next++ )
        {
            result = result << 6;
            result |= *next & 63;
        }
		UCChar uc_char;
		uc_char.code_point = result;
		uc_char.source_offset = first-src;
		uc_char.source_length = next-first;
        result_list.push_back( uc_char );
        first = next;
    }
    return true;
}

int dump_outlines( int argc, char* argv[] )
{
	int pixel_size = 200;
    FT_Error error;
    FT_Face han_face;
    FT_Library library;
    FT_Init_Errors();
    int dpi_x = 100;
    int dpi_y = 100;
    LOG( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" );
	LOG( "<svg xmlns=\"http://www.w3.org/2000/svg\"" );
	LOG( "	xmlns:xlink=\"http://www.w3.org/1999/xlink\"" );
	LOG( "	xmlns:ev=\"http://www.w3.org/2001/xml-events\"" );
	LOG( "	version=\"1.1\" baseProfile=\"full\"" );
	LOG( "	width=\"500px\" height=\"500px\" viewBox=\"0 0 500 500\">" );
    error = FT_Init_FreeType( &library );
    if( error )
    {
		std::stringstream msg;
		msg << "error initializing freetype: " << ft_errors[error];
        throw ERROR( msg.str() );
    }
    std::string _han_font = "flash_data/chinese-touch/ukai.ttc";
    error = FT_New_Face( library, _han_font.c_str(), 0, &han_face );
    if( error )
    {
		std::stringstream msg;
        msg << "error loading chinese font: " << _han_font << " (" 
                << ft_errors[error] << ")";
		throw ERROR( msg.str() );
    }
    error = FT_Set_Char_Size( han_face, 0, pixel_size*64, dpi_x, dpi_y );
    if( error )
    {
        std::stringstream msg;
		msg << "error setting pixel size: " << ft_errors[error];
		throw ERROR( msg.str() );
    }
	for( int i=1; i<argc; i++ )
	{
		UCCharList char_list;
		if( !utf8_to_ucs4((unsigned char*)argv[i], char_list) )
		{
		    WARN( "<!-- error in utf-8 input (non fatal) -->" );
		}
		for( UCCharList::iterator ci=char_list.begin(); ci!=char_list.end(); ci++ )
		{
        	FT_UInt glyph_index = FT_Get_Char_Index( han_face, ci->code_point );
        	if( !glyph_index )
        		continue;
		    LOG( "<!-- glyph index: " << glyph_index << "-->" );
		    char buffer[1000]; buffer[0]=0;
		    FT_Get_Glyph_Name( han_face, glyph_index, buffer, 1000 );
		    LOG( "<!-- glyph name: " << buffer << "-->" );
			FT_Vector pen;
			pen.x = 0;
			pen.y = 0;
	        FT_Set_Transform( han_face, 0, &pen );
		    error = FT_Load_Glyph( han_face, glyph_index, FT_LOAD_RENDER );
		    if( error )
		    {
				WARN( "error loading glyph index: " << glyph_index << " (" << ft_errors[error] << ")" );
				continue;
		    }
        	FT_GlyphSlot& glyph = han_face->glyph;
        	LOG( "<!-- contours: " << glyph->outline.n_contours << "-->" );
        	LOG( "<!-- points: " << glyph->outline.n_points << "-->" );
        	short i_begin = 0;
        	for( int i_cont=0; i_cont<glyph->outline.n_contours; i_begin++, i_cont++ )
        	{
	        	std::stringstream path;
	        	path << "<path fill=\"none\" stroke=\"black\" stroke-width=\"2px\" d=\"";
        		short i_end = glyph->outline.contours[i_cont];
        		FT_Vector p_begin = glyph->outline.points[i_begin];
        		FT_Vector p_end = glyph->outline.points[i_end];
        		LOG( "<!--START-->" );
        		FT_Vector p_a = p_begin;
				FT_Vector p_b = p_a;
				FT_Vector p_c = p_a;
				path << "M" << p_begin.x/64.0 << "," << 1.5*pixel_size-p_begin.y/64.0 << " ";
				for( int i_point=i_begin+1; i_point<=i_end; i_point++ )
				{
					p_b = glyph->outline.points[i_point];
					int tag = FT_CURVE_TAG( glyph->outline.tags[i_point] );
					if( tag==FT_CURVE_TAG_ON )
					{
        				LOG( "<line x1=\"" << p_c.x/64.0 << "\" y1= \"" << 1.5*pixel_size-p_c.y/64.0 << "\" x2=\"" << p_b.x/64.0 << "\" y2=\"" << 1.5*pixel_size-p_b.y/64.0 << "\" stroke=\"green\" stroke-width=\"1px\" />" );
        				//path << "S" << p_c.x/64.0 << "," << 1.5*pixel_size-p_c.y/64.0 << " " << p_b.x/64.0 << "," << 1.5*pixel_size-p_b.y/64.0 << " ";
        				path << "C" << p_a.x/64.0 << "," << 1.5*pixel_size-p_a.y/64.0 << " " 
        							<< p_c.x/64.0 << "," << 1.5*pixel_size-p_c.y/64.0 << " " 
        					<< p_b.x/64.0 << "," << 1.5*pixel_size-p_b.y/64.0 << " ";
        				LOG( "<line x1=\"" << p_a.x/64.0 << "\" y1= \"" << 1.5*pixel_size-p_a.y/64.0 << "\" x2=\"" << p_b.x/64.0 << "\" y2=\"" << 1.5*pixel_size-p_b.y/64.0 << "\" stroke=\"blue\" stroke-width=\"1px\" />" );
        				p_a = p_b;
        			}
        			else
        			{
        				LOG( "<line x1=\"" << p_a.x/64.0 << "\" y1= \"" << 1.5*pixel_size-p_a.y/64.0 << "\" x2=\"" << p_b.x/64.0 << "\" y2=\"" << 1.5*pixel_size-p_b.y/64.0 << "\" stroke=\"red\" stroke-width=\"1px\" />" );
        				p_c = p_b;
        			}
				}
   				LOG( "<line x1=\"" << p_b.x/64.0 << "\" y1= \"" << 1.5*pixel_size-p_b.y/64.0 << "\" x2=\"" << p_begin.x/64.0 << "\" y2=\"" << 1.5*pixel_size-p_begin.y/64.0 << "\" stroke=\"green\" stroke-width=\"1px\" />" );
   				//path << "S" << p_b.x/64.0 << "," << 1.5*pixel_size-p_b.y/64.0 << " " << p_begin.x/64.0 << "," << 1.5*pixel_size-p_begin.y/64.0 << "\" />";
   				path << "C" << p_a.x/64.0 << "," << 1.5*pixel_size-p_a.y/64.0 << " " 
   							<< p_b.x/64.0 << "," << 1.5*pixel_size-p_b.y/64.0 << " " 
   					<< p_begin.x/64.0 << "," << 1.5*pixel_size-p_begin.y/64.0 << "\" />";
   				LOG( "<line x1=\"" << p_a.x/64.0 << "\" y1= \"" << 1.5*pixel_size-p_a.y/64.0 << "\" x2=\"" << p_begin.x/64.0 << "\" y2=\"" << 1.5*pixel_size-p_begin.y/64.0 << "\" stroke=\"blue\" stroke-width=\"1px\" />" );
        		i_begin = i_end;
        		LOG( path.str() );
        	}
		}
	}
	LOG( "</svg>" );
}

int main( int argc, char* argv[] )
{
	try
	{
		dump_outlines( argc, argv );
	}
	catch( Error& e )
	{
		std::cout << e.what() << std::endl;
	}
}

