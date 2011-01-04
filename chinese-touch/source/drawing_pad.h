#ifndef DRAWING_PAD_H
#define DRAWING_PAD_H

#include "freetype_renderer.h"
#include "text_button.h"

class Pen
{
public:
	int width, height;
	u8* buffer;
public:
	Pen( int _width, int _heigth, u8* _buffer );
};

class DrawingPad
{
public:
    RenderScreen& render_screen;
	/*! a factor f, where: f * prev_line_length = max_next_line_length 
		(used to filter out some erroneous touch readings, occurring under very light pressure) */
	static int MAX_ACCELERATION_FACTOR;
	static Pen small_pen, medium_pen, ink_pen, large_pen;

public:
    DrawingPad( RenderScreen& _render_screen );
    void clear( int color=0 );
    void draw( int x, int y, const Pen& pen=small_pen, bool erase=false );
    void draw_line( int x1, int y1, int x2, int y2, const Pen& pen=small_pen, bool erase=false );
};

#endif // DRAWING_PAD_H
