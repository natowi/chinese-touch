#include <nds.h>
#include <nds/arm9/sprite.h>

#include "freetype_renderer.h"
#include "text_button.h"
#include "sprite_helper.h"


TextButton::TextButton( TextButton& text_button )
{
	*this = text_button;
	this->owns_bg_vram = false;
	this->owns_text_vram = false;
}

TextButton::TextButton( RenderScreen& _render_screen, const std::string& _text, 
						SpriteSize _sprite_size, int _x, int _y,
						FT_Face _face, int _font_size,
						int _text_x_offset, int _text_y_offset )
	: render_screen(&_render_screen), text(_text), sprite_size(_sprite_size), text_sprite_size(_sprite_size), 
	x(_x), y(_y), text_x_offset(_text_x_offset), text_y_offset(_text_y_offset),
	bg_prio(1), text_prio(0),
	text_vram(0), bg_vram(0), bg_active_vram(0), bg_inactive_vram(0), 
	active(false), disabled(false), owns_bg_vram(true), owns_text_vram(true), hidden(false),
	face(_face), font_size(_font_size)
{
	this->get_dimensions_from_sprite_size( this->sprite_size, this->width, this->height );
	this->sensor_width = this->width; this->sensor_height = this->height;
	this->text_width = this->width; this->text_height = this->height;
}

void TextButton::get_dimensions_from_sprite_size(SpriteSize _sprite_size, int& _width, int& _height)
{
	if( SPRITE_SIZE_SHAPE(_sprite_size)==OBJSHAPE_SQUARE )
	{
		if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_8 ) _width=8;
		else if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_16 ) _width=16;
		else if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_32 ) _width=32;
		else if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_64 ) _width=64;
		_height = _width;
	}
	else
	{
		if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_8 ) { _width=16; _height=8; }
		else if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_16 ) { _width=32; _height=8; }
		else if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_32 )  { _width=32; _height=16; }
		else if( SPRITE_SIZE_SIZE(_sprite_size)==OBJSIZE_64 ) { _width=64; _height=32; }
		if( SPRITE_SIZE_SHAPE(_sprite_size)==OBJSHAPE_TALL )
		{
			int temp = _height; _height = _width; _width = temp;
		}
	}
}

TextButton::~TextButton()
{
	this->free_all();
}

void TextButton::free_all()
{
	if( this->text_vram && this->owns_text_vram ) 
	{
		oamFreeGfx( this->get_oam(), this->text_vram );
		this->text_vram = 0;
	}
	if( this->owns_bg_vram )
	{
		if( this->bg_vram )
		{
			oamFreeGfx( this->get_oam(), this->bg_vram );
			this->bg_vram = 0;
		}
		if( this->bg_active_vram ) 
		{
			oamFreeGfx( this->get_oam(), this->bg_active_vram );
			this->bg_active_vram = 0;
		}
		if( this->bg_inactive_vram ) 
		{
			oamFreeGfx( this->get_oam(), this->bg_inactive_vram );
			this->bg_inactive_vram = 0;
		}
	}
}

void TextButton::init_vram( const void* source, u16*& vram_dest )
{
	vram_dest = oamAllocateGfx( this->get_oam(), this->sprite_size, SpriteColorFormat_Bmp );
	DC_FlushRange( source, SPRITE_SIZE_PIXELS(this->sprite_size)*2 );
	dmaCopy( source, vram_dest, SPRITE_SIZE_PIXELS(this->sprite_size)*2 );
	set_16bpp_sprite_opague( vram_dest, SPRITE_SIZE_PIXELS(this->sprite_size)*2 );
}

void TextButton::init_text_layer( FreetypeRenderer& freetype_renderer )
{
	if( this->text.length() && this->owns_text_vram )
	{
		// allocate VRAM for 8bpp text layer
		this->text_vram = oamAllocateGfx( this->get_oam(), this->text_sprite_size, SpriteColorFormat_256Color );
		RenderScreenBuffer button_text( this->text_width, this->text_height );
		RenderStyle render_style;
		render_style.center_x = true;
		freetype_renderer.render( button_text, this->text, 
			this->face, this->font_size, 0, 1, &render_style );
		// convert 8bpp sprite data for use in 16bpp mode
		// FIXME: currently needs ram buffering, as vram supports 16bit operations only
		u8 conversion_buffer[ SPRITE_SIZE_PIXELS(this->text_sprite_size) ];
		tile_8bpp_sprite( (u8*)(button_text.base_address), conversion_buffer, this->text_width, this->text_height );
		//DC_FlushRange( conversion_buffer, SPRITE_SIZE_PIXELS(this->sprite_size)*1 );
		//dmaCopy( conversion_buffer, this->text_vram, SPRITE_SIZE_PIXELS(this->sprite_size)*1 );
		memcpy( this->text_vram, conversion_buffer, SPRITE_SIZE_PIXELS(this->text_sprite_size)*1 );
	}
}

OamState* TextButton::get_oam()
{
	switch( this->render_screen->screen )
	{
		case SCREEN_MAIN: return &oamMain;
		case SCREEN_SUB: return &oamSub;
		case SCREEN_NONE: return 0;
	}
	return 0;
}

void TextButton::render_to( int& oam_entry )
{
	this->render_to( oam_entry, this->x, this->y );
}

void TextButton::render_to( int& oam_entry, int _x, int _y )
{
	u16* vram = this->bg_vram;
	if( this->active && this->bg_active_vram ) vram = this->bg_active_vram;
	if( this->disabled && this->bg_inactive_vram ) vram = this->bg_inactive_vram;
	if( vram && this->is_visible(_x, _y, this->width, this->height) )
	{
		oamSet( this->get_oam(), oam_entry++,
				_x, _y, 	// position
				this->bg_prio, 1, this->sprite_size, SpriteColorFormat_Bmp, 
				vram,
				0, 0, 0, 0, 0, 0 );
	}
	if( this->text_vram 
		&& this->is_visible(_x+this->text_x_offset, _y+this->text_y_offset, 
							this->text_width, this->text_height) )
	{
		oamSet( this->get_oam(), oam_entry++,
				_x+this->text_x_offset, _y+this->text_y_offset, 	// position
				this->text_prio, 0, this->text_sprite_size, SpriteColorFormat_256Color, 
				this->text_vram,
				0, 0, 0, 0, 0, 0 );
	}
}

bool TextButton::is_visible(int _x, int _y, int _width, int _height)
{
	return !this->hidden
		&& (_x < this->render_screen->res_x)
		&& (_y < this->render_screen->res_y)
		&& ((_x+_width) >= 0)
		&& ((_y+_height) >= 0);
}

bool TextButton::is_responsible( int ref_x, int ref_y )
{
	return !this->disabled
			&& ref_x > this->x && ref_x < this->x+this->sensor_width
			&& ref_y > this->y && ref_y < this->y+this->sensor_height;
}


void TextButtonList::free_all()
{
	for( TextButtonList::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( *i ) (*i)->free_all();
	}
}


TextButtonListStorage::~TextButtonListStorage()
{
	for( TextButtonListStorage::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( *i ) delete *i;
	}
	this->clear();
}


TextButtonSetStorage::~TextButtonSetStorage()
{
	for( TextButtonSetStorage::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( *i ) delete *i;
	}
	this->clear();
}

void TextButtonSetStorage::free_all()
{
	for( TextButtonSetStorage::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( *i ) (*i)->free_all();
	}
}


TextButtonMapStorage::~TextButtonMapStorage()
{
	for( TextButtonMapStorage::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( i->second ) 
			delete i->second;
	}
	this->clear();
}

void TextButtonMapStorage::add_text_button(const std::string& name, TextButton* text_button)
{
	if( text_button )
	{
		if( this->count(name) && (*this)[name]!=text_button )
		{
			// clean-up old button:
			if( (*this)[name] )
				delete (*this)[name];
		}
		(*this)[name] = text_button;
		text_button->name = name;
	}
}

TextButton* TextButtonMapStorage::get_text_button(const std::string& name)
{
	if( this->count(name) )
	{
		return (*this)[name];
	} else return 0;
}

void TextButtonMapStorage::free_all()
{
	for( TextButtonMapStorage::iterator i=this->begin(); i!=this->end(); i++ )
	{
		if( i->second ) i->second->free_all();
	}
}
