#include <nds.h>
#include <nds/arm9/sprite.h>

#include "freetype_renderer.h"
#include "text_button.h"

TextButton::TextButton( OamState* _oam, const std::string& _text, 
						SpriteSize _sprite_size, int _x, int _y,
						FT_Face _face, int _font_size,
						int _text_x_offset, int _text_y_offset )
	: oam(_oam), text(_text), sprite_size(_sprite_size), x(_x), y(_y), 
	text_x_offset(_text_x_offset), text_y_offset(_text_y_offset),
	text_vram(0), bg_vram(0), bg_active_vram(0), bg_inactive_vram(0), 
	active(false), inactive(false), owns_bg_vram(true),
	face(_face), font_size(_font_size)
{
	if( SPRITE_SIZE_SHAPE(sprite_size)==OBJSHAPE_SQUARE )
	{
		if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_8 ) this->width=8;
		else if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_16 ) this->width=16;
		else if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_32 ) this->width=32;
		else if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_64 ) this->width=64;
		this->height = this->width;
	}
	else
	{
		if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_8 ) { this->width=16; this->height=8; }
		else if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_16 ) { this->width=32; this->height=8; }
		else if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_32 )  { this->width=32; this->height=16; }
		else if( SPRITE_SIZE_SIZE(sprite_size)==OBJSIZE_64 ) { this->width=64; this->height=32; }
		if( SPRITE_SIZE_SHAPE(sprite_size)==OBJSHAPE_TALL )
		{
			int temp = this->height; this->height = this->width; this->width = temp;
		}
	}
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

void TextButton::init_vram( const void* source, u16*& vram_dest )
{
	vram_dest = oamAllocateGfx( this->oam, this->sprite_size, SpriteColorFormat_Bmp );
	dmaCopy( source, vram_dest, this->width * this->height * 2 );
}

void TextButton::render_to( int& oam_entry )
{
	this->render_to( oam_entry, this->x, this->y );
}

void TextButton::render_to( int& oam_entry, int _x, int _y )
{
	u16* vram = this->bg_vram;
	if( this->active && this->bg_active_vram ) vram = this->bg_active_vram;
	if( this->inactive && this->bg_inactive_vram ) vram = this->bg_inactive_vram;
	if( vram )
	{
		oamSet( this->oam, oam_entry++,
				_x, _y, 	// position
				1, 1, this->sprite_size, SpriteColorFormat_Bmp, 
				vram,
				0, 0, 0, 0, 0, 0 );
	}
	if( this->text_vram )
	{
		oamSet( this->oam, oam_entry++,
				_x+this->text_x_offset, _y+this->text_y_offset, 	// position
				0, 0, this->sprite_size, SpriteColorFormat_256Color, 
				this->text_vram,
				0, 0, 0, 0, 0, 0 );
	}
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