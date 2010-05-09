#ifndef FREETYPE_RENDERER_H
#define FREETYPE_RENDERER_H

#include <map>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <nds.h>

#include "unicode.h"
#include "screen.h"
#include "../freetype/include/freetype/fttypes.h"
#include "../freetype/include/freetype/freetype.h"


typedef std::map<int,const char*> ErrorMap;
extern ErrorMap ft_errors;


class RenderStyle
{
public:
    RenderStyle() : center_x(false), center_y(false), 
            linebreak(true), autoscale(false), autofallback(true), 
			multiline(true), tab_spaces(2), indentation_offset(0) {}
public:
    bool center_x, center_y, linebreak, autoscale, autofallback, multiline;
	int tab_spaces, indentation_offset;
};


class RenderInfo
{
public:
    RenderInfo( int _x, int _y, int _width, int _height )
        : x(_x), y(_y), width(_width), height(_height), indentation_offset(0) {}
public:
    int x, y, width, height, indentation_offset;
};


class RenderScreen
{
public:
	RenderScreen( int _res_x=256, int _res_y=192 ) 
		: id(0), base_address(0), palette(0), 
			res_x(_res_x), res_y(_res_y) {}
	void init( int _id )
	{
		this->id = _id;
		this->base_address = bgGetGfxPtr( _id );
		bgSetPriority( _id, 2 );
	}
	void init_bg( int _bg_id )
	{
		this->bg_id = _bg_id;
		this->bg_base_address = bgGetGfxPtr( _bg_id );
		bgSetPriority( _bg_id, 3 );
	}
	void clear( int color=0 );
public:
	int id;
	u16* base_address;
	u16* palette;
    int res_x, res_y;
	int bg_id;
	u16* bg_base_address;
};


class RenderScreenBuffer : public RenderScreen
{
public:
	RenderScreenBuffer( int _res_x, int _res_y )
		: RenderScreen( _res_x, _res_y )
	{
		int size = this->res_x*this->res_y*1;
		this->base_address = (u16*)malloc( size );
		this->clear();
	}
	~RenderScreenBuffer()
	{
		free( this->base_address );
	}
	void render_to( RenderScreen& dest, int x, int y, bool replace=false );
};


class RenderChar
{
public:
    RenderChar( UCChar _uc_char, unsigned long _glyph_index, FT_Face _face ) 
        : x(0), y(0), width(0), height(0), 
          uc_char(_uc_char), glyph_index(_glyph_index), 
          line_begin(false), curr_line_end_char(0), face(_face) {}
    RenderChar( UCCharList::iterator _input_char_it, unsigned long _glyph_index, FT_Face _face ) 
        : x(0), y(0), width(0), height(0), 
          uc_char(*_input_char_it), glyph_index(_glyph_index), 
          line_begin(false), curr_line_end_char(0), face(_face),
		  input_char_it(_input_char_it) {}
public:
    int x, y, width, height;
	FT_Vector pen, advance;
	UCChar uc_char;
    unsigned long glyph_index;
    bool line_begin;
    RenderChar* curr_line_end_char;
	FT_Face face;
	UCCharList::iterator input_char_it; //!< reference to source position in input char list (only useful within FreetypeRenderer::render()!)
};

class RenderCharList : public std::list<RenderChar*>
{
public:
	~RenderCharList();
};


class FreetypeRenderer
{
public:
    FreetypeRenderer( const std::string& han_font, 
                    const std::string& latin_font, const std::string& jp_font );
    ~FreetypeRenderer();
	void init_screen( Screen screen, RenderScreen& render_screen );
    RenderInfo render( const RenderScreen& render_screen, const std::string& text, FT_Face face, int pixel_size, 
                int x, int y, RenderStyle* render_style=0, RenderCharList* render_char_list=0 );
    RenderInfo render( const RenderScreen& render_screen, UCCharList& char_list, FT_Face face, int pixel_size, 
                int x, int y, RenderStyle* render_style=0, RenderCharList* render_char_list=0 );
public:
    FT_Error error;
    FT_Face han_face, latin_face, jp_face;
    FT_Library library;
    int dpi_x, dpi_y;
};

#endif // FREETYPE_RENDERER_H
