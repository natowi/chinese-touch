#include <nds.h>
#include <nds/arm9/sprite.h>

#include "error_console.h"


void tile_8bpp_sprite( u8* source_buffer, u8* dest_buffer, int width, int height )
{
	int trows = width/8;
	int tlines = height/8;
	int dest_offset = 0;
	for( int tlin=0; tlin<tlines; tlin++ )
		for( int trow=0; trow<trows; trow++ )
			for( int plin=0; plin<8; plin++ )
				for( int prow=0; prow<8; prow++, dest_offset++ )
				{
					int source_offset = (width*8*tlin)+(plin*trows+trow)*8+prow;
					dest_buffer[dest_offset] = source_buffer[source_offset];
				}
}

void set_16bpp_sprite_opague( u16* vram, int width, int height, u16 transparent_value )
{
	for( int i=0; i<width*height; i++ )
		if( vram[i]!=transparent_value )
			vram[i] |= 1<<15;
}

void log_oam_state_sub()
{
	LOG( "gfxOffsetStep: " << oamSub.gfxOffsetStep );
	for( int i=0; i<20; i++ )
	{
		LOG( "[" << i << "] gfxIndex: " << oamSub.oamMemory[i].gfxIndex );
	}
}
