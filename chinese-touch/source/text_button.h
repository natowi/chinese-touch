#ifndef TEXT_BUTTON_H
#define TEXT_BUTTON_H

#include <string>
#include <list>
#include <set>
#include <nds/arm9/sprite.h>

#include "freetype_renderer.h"


class RenderScreen;

class TextButton
{
public:
	RenderScreen* render_screen; //!< pointer instead of reference, because we may want to change screens dynamically
	std::string name, text;
	SpriteSize sprite_size, text_sprite_size;
	int width, height, sensor_width, sensor_height, text_width, text_height, 
		x, y, text_x_offset, text_y_offset, bg_prio, text_prio, fg_prio,
		bg_rotation, text_rotation, fg_rotation;
	u16 *text_vram, *bg_vram, *bg_active_vram, *bg_inactive_vram, *fg_vram;
	bool active, disabled, hidden, owns_bg_vram, owns_text_vram, owns_fg_vram;
	FT_Face face;
	int font_size;
public:
	TextButton( TextButton& text_button );
	TextButton( RenderScreen& _render_screen, const std::string& _text, 
				SpriteSize _sprite_size, int _x, int _y, 
				FT_Face _face, int _font_size, 
				int _text_x_offset=0, int _text_y_offset=0 );
	void get_dimensions_from_sprite_size( SpriteSize _sprite_size, int& _width, int& _height );
	~TextButton();
	void init_vram( const void* source, u16*& vram_dest );
	void init_text_layer(FreetypeRenderer& freetype_renderer);
	OamState* get_oam();
	void render_to( int& oam_entry );
	void render_to( int& oam_entry, int _x, int _y );
	bool is_visible( int _x, int _y, int _width, int _height );
	bool is_responsible( int ref_x, int ref_y );
	void free_all();
};

class TextButtonList : public std::list<TextButton*>
{
public:
	void free_all();
};

class TextButtonListStorage : public TextButtonList
{
public:
	~TextButtonListStorage();
};

class TextButtonSetStorage : public std::set<TextButton*>
{
public:
	~TextButtonSetStorage();
	void free_all();
};

class TextButtonMapStorage : public std::map<std::string,TextButton*>
{
public:
	~TextButtonMapStorage();
	void add_text_button( const std::string &name, TextButton* text_button );
	TextButton* get_text_button( const std::string &name );
	void free_all();
};

#endif
