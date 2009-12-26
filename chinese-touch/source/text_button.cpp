#include <nds.h>

#include "freetype_renderer.h"
#include "text_button.h"

TextButton::TextButton( OamState* _oam, const std::string& _text, 
						int _width, int _height, int _x, int _y,
						FT_Face _face, int _font_size,
						int _text_x_offset, int _text_y_offset )
	: oam(_oam), text(_text), width(_width), height(_height), x(_x), y(_y), 
	text_x_offset(_text_x_offset), text_y_offset(_text_y_offset),
	text_vram(0), bg_vram(0), bg_active_vram(0), bg_inactive_vram(0), 
	active(false), inactive(false), owns_bg_vram(true),
	face(_face), font_size(_font_size)
{
}

TextButton::~TextButton()
{
	if( this->text_vram ) oamFreeGfx( this->oam, this->text_vram );
	if( this->owns_bg_vram )
	{
		if( this->bg_vram ) oamFreeGfx( this->oam, this->bg_vram );
		if( this->bg_active_vram ) oamFreeGfx( this->oam, this->bg_active_vram );
		if( this->bg_inactive_vram ) oamFreeGfx( this->oam, this->bg_inactive_vram );
	}
}

void TextButton::render_to( RenderScreen& render_screen )
{
}

bool TextButton::is_responsible( int ref_x, int ref_y )
{
	return     ref_x > this->x && ref_x < this->x+this->width
			&& ref_y > this->y && ref_y < this->y+this->height;
}


TextButtonListStorage::~TextButtonListStorage()
{
	for( TextButtonListStorage::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( *i ) delete *i;
	}
}