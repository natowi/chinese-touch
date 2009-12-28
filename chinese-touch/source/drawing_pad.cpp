#include "string.h"
#include "nds.h"
#include "math.h"
#include <iostream>

#include "drawing_pad.h"


int DrawingPad::MAX_ACCELERATION_FACTOR = 5;

DrawingPad::DrawingPad( RenderScreen& _render_screen ) : render_screen(_render_screen)
{
}

void DrawingPad::clear( int color )
{
	this->render_screen.clear( color );
}

void DrawingPad::draw_point( int x, int y )
{
    const int width = 5;
    const int height = 3;
    u8 buffer[width*height] = { 0x00, 0x06, 0x0a, 0x06, 0x00,
                                0x00, 0x0a, 0xff, 0x0a, 0x00,
                                0x00, 0x06, 0x0a, 0x06, 0x00 };
    this->draw( x, y, (u8*)&buffer, width, height );
}

void DrawingPad::draw( int x, int y, u8* buffer, int width, int height )
{
    for( int row=0; row<height; row++ )
    {
        for( int pixel=(x%2 ? 0 : 1); pixel<width-1; pixel+=2 )
        {
            u16* bg_gfx_ptr = this->render_screen.base_address;
            u16* base_address = bg_gfx_ptr
                    + ( row + y - height/2 ) * this->render_screen.res_x/2
                    + pixel/2 + x/2 - (width-1)/2/2;
            if( base_address < bg_gfx_ptr
                || base_address > bg_gfx_ptr+this->render_screen.res_x*this->render_screen.res_y-2 )
            {
                return;
            }
            u16 ov = *base_address;
            u8 ov1 = ov & 0x00ff;
            u8 ov2 = (ov & 0xff00) >> 8;
            u8 nv1 = ov1 + buffer[row*width+pixel];
            u8 nv2 = ov2 + buffer[row*width+pixel+1];
            *base_address = ((nv1 >= ov1) ? nv1 : 0xff)
                         + (((nv2 >= ov2) ? nv2 : 0xff) << 8);
//            *base_address = nv1 + (nv2 << 8);
        }
    }
}

void DrawingPad::draw_line( int x1, int y1, int x2, int y2 )
{
    double xd = x2-x1;
    double yd = y2-y1;
    int steps = abs(xd) > abs(yd) ? abs(xd) : abs(yd);
    this->draw_point( x1, y1 );
    // große Striche durch Sensorfehler ignorieren:
    if( steps > 40 )
        return;
    for( int i=1; i<=steps; i++ )
    {
        this->draw_point( x1+i*xd/steps, y1+i*yd/steps );
    }
}

