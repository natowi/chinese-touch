#ifndef TEXT_BUTTON_H
#define TEXT_BUTTON_H

#include <string>
#include <list>

#include "freetype_renderer.h"


class RenderScreen;

class TextButton
{
public:
	OamState* oam;
	std::string text;
	int width, height, x, y, text_x_offset, text_y_offset;
	u16 *text_vram, *bg_vram, *bg_active_vram, *bg_inactive_vram;
	bool active, inactive, owns_bg_vram;
	FT_Face face;
	int font_size;
public:
	TextButton( OamState* _oam, const std::string& _text, 
				int _width, int _height, int _x, int _y, 
				FT_Face _face, int _font_size, 
				int _text_x_offset=0, int _text_y_offset=0 );
	~TextButton();
	void render_to( RenderScreen& render_screen );
	bool is_responsible( int ref_x, int ref_y );
};

class TextButtonList : public std::list<TextButton*>
{
};

class TextButtonListStorage : public TextButtonList
{
public:
	~TextButtonListStorage();
};

#endif
