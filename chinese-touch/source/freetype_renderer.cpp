#include <iostream>

#include <nds.h>

#include "freetype_renderer.h"
#include "config.h"
#include "error_console.h"

#undef __FTERRORS_H__
#define FT_ERROR_START_LIST     ErrorMap ft_errors; void FT_Init_Errors(){
#define FT_ERRORDEF( e, v, s )  ft_errors[e]=s;
#define FT_ERROR_END_LIST       };

#include FT_ERRORS_H


FreetypeRenderer::FreetypeRenderer( const std::string& han_font, 
	const std::string& latin_font ) 
		: dpi_x(100), dpi_y(100)
{	
    FT_Init_Errors();
    LOG( "ft_errors.size(): " << ft_errors.size() );
    this->error = FT_Init_FreeType( &this->library );
    if( this->error )
    {
		std::stringstream msg;
		msg << "error initializing freetype: " << ft_errors[this->error];
        throw ERROR( msg.str() );
    }
    std::string _han_font = BASE_DIR + han_font;
    std::string _latin_font = BASE_DIR + latin_font;
    this->error = FT_New_Face( this->library, _han_font.c_str(), 0, &this->han_face );
    if( this->error )
    {
		std::stringstream msg;
        msg << "error loading chinese font: " << _han_font << " (" 
                << ft_errors[this->error] << ")";
		throw ERROR( msg.str() );
    }
    this->error = FT_New_Face( this->library, _latin_font.c_str(), 0, &this->latin_face );
    if( this->error )
    {
		std::stringstream msg;
        msg << "error loading latin font: " << _latin_font << " (" 
                << ft_errors[this->error] << ")";
        throw ERROR( msg.str() );
    }
#if 0
    // Select charmap
    this->error = FT_Select_Charmap( this->han_face, FT_ENCODING_UNICODE );
    if( this->error )
    {
		std::stringstream msg;
        msg << "error loading unicode charmap: " << ft_errors[this->error];
        throw ERROR( msg.str() );
    }
#endif   
}

FreetypeRenderer::~FreetypeRenderer()
{
    FT_Done_Face( this->han_face );
    FT_Done_Face( this->latin_face );
    FT_Done_FreeType( this->library );
}

void FreetypeRenderer::init_screen( Screen screen, RenderScreen& render_screen )
{
    // set the mode for 2 text layers and two extended background layers
	if( screen == SCREEN_MAIN )
	{
		videoSetMode(MODE_5_2D);
		vramSetBankA(VRAM_A_MAIN_BG);
		vramSetBankB(VRAM_B_MAIN_BG);
		render_screen.init( bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0) );
		render_screen.palette = BG_PALETTE;
		render_screen.init_bg( bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 3, 0) );
	}
	else /*if( screen == SCREEN_SUB )*/
	{
		videoSetModeSub(MODE_5_2D);
		vramSetBankC(VRAM_C_SUB_BG_0x06200000);
		render_screen.init( bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0) );
		render_screen.palette = BG_PALETTE_SUB;
	}
	// lineare 15-bit-graupalette mit 256 indizes aufbauen:
	for( int i=0; i<256; i++ )
	{
	    int value = (int)( ((double)i)*(double)(1<<5)/256.0 );
	    render_screen.palette[255-i] = /*(1 << 15) |*/ (value << 10) | (value << 5) | value;
	}
}

void RenderScreen::clear( int color )
{
    // 1. clear background buffer with background color
    memset( this->base_address, color, this->res_x*this->res_y*1 );
}

void RenderScreenBuffer::render_to( RenderScreen& dest, int x, int y, bool replace )
{
	// 16-bit-aligned to allow copying to vram:
	char* buffer = (char*)this->base_address;
	for( int row=0; row<this->res_y; row++ )
	{
		// FIXME: Optimierung durch zeilenweises dmaCopy?
		for( int pixel=0; pixel<this->res_x; pixel+=2 )
		{
			u16* base_address = dest.base_address
					+ (row+y)*dest.res_x/2
					+ pixel/2 + x/2;
			if( (base_address < dest.base_address)
				|| (base_address > (dest.base_address+dest.res_x*dest.res_y-2)) )
			{
				continue;
			}
			u16 value = 0;
			if( pixel < this->res_x-1 )
			{
				value = (buffer[row*this->res_x+pixel+1] << 8)
							+ buffer[row*this->res_x+pixel];
			}
			else
			{
				value = buffer[row*this->res_x+pixel];
			}
			if( !replace ) *base_address |= value;
			else *base_address = value;
		}
	}
}


RenderCharList::~RenderCharList()
{
	for( RenderCharList::iterator rc_it = this->begin();
		rc_it != this->end(); rc_it++ )
	{
		if( *rc_it ) delete *rc_it;
	}
}


RenderInfo FreetypeRenderer::render( const RenderScreen& render_screen, const std::string& text, FT_Face face, 
            int pixel_size, int x, int y, RenderStyle* render_style, RenderCharList* render_char_list )
{
    // 1. unicode conversion
    UCCharList char_list;
    if( !utf8_to_ucs4((unsigned char*)text.c_str(), char_list) )
    {
        WARN( "error in utf-8 input (non fatal)" );
    }
	return this->render( render_screen, char_list, face, pixel_size, x, y, render_style, render_char_list );
}

RenderInfo FreetypeRenderer::render( const RenderScreen& render_screen, UCCharList& input_char_list, FT_Face face, 
            int pixel_size, int x, int y, RenderStyle* render_style, RenderCharList* render_char_list )
{
	// redirect uninitialized pointer to stack object with default configuration:
	RenderStyle default_render_style;
	if( !render_style ) render_style = &default_render_style;
	// redirect uninitialized pointer to stack object to ensure its destruction:
    RenderCharList stack_render_char_list;
	if( !render_char_list ) render_char_list = &stack_render_char_list;
    if( input_char_list.size()==0 ) return RenderInfo( x, y, 0, 0 );
    
	int han_pixel_size = pixel_size;
	int latin_pixel_size = pixel_size;
	if( face == this->han_face )
	{
		han_pixel_size = pixel_size;
		latin_pixel_size = (int)((double)han_pixel_size * 0.7);
	}
	else if( face == this->latin_face )
	{
		latin_pixel_size = pixel_size;
		han_pixel_size = (int)((double)latin_pixel_size * 1.4);
	}
    FT_Error error;
    error = FT_Set_Char_Size( this->han_face, 0, han_pixel_size*64, this->dpi_x, this->dpi_y );
    if( error )
    {
        std::stringstream msg;
		msg << "error setting pixel size: " << ft_errors[error];
		throw ERROR( msg.str() );
    }
    error = FT_Set_Char_Size( this->latin_face, 0, latin_pixel_size*64, this->dpi_x, this->dpi_y );
    if( error )
    {
        std::stringstream msg;
		msg << "error setting pixel size: " << ft_errors[error];
		throw ERROR( msg.str() );
    }
    
    int line_height = pixel_size+1;
	int indentation_offset = render_style->indentation_offset;
	// ignore leading tabs if we are in indentation mode:
	bool ignore_tabs = (render_style->indentation_offset != 0);
    FT_Vector pen;
    pen.x = (x + indentation_offset) * 64;
    pen.y = -y * 64;
	
	
	UCCharList::iterator input_char_it=input_char_list.begin();
    for( ; input_char_it!=input_char_list.end(); input_char_it++ )
    {
		//std::cout << "#";
        // Load Char
        LOG( "character code: " << input_char_it->code_point );
		FT_Face current_face = face;
        FT_UInt glyph_index = FT_Get_Char_Index( current_face, input_char_it->code_point );
		if( !glyph_index && render_style->autofallback && current_face != this->han_face )
		{
			glyph_index = FT_Get_Char_Index( this->han_face, input_char_it->code_point );
			if( glyph_index )
			{
				current_face = this->han_face;
			}
		}
		else if( !glyph_index && render_style->autofallback && current_face != this->latin_face )
		{
			glyph_index = FT_Get_Char_Index( this->latin_face, input_char_it->code_point );
			if( glyph_index )
			{
				current_face = this->latin_face;
			}
		}
        if( !glyph_index )
        {
			if( input_char_it->code_point==10 )
			{
				// Reset tab/indentation offset at line breaks, but only if the line break is not
				// immediately followed by one or more new tab characters. Those are interpreted 
				// as indentation-continuation markers and apart from that ignored.
				UCCharList::iterator next_char_it = input_char_it;
				next_char_it++;
				if( next_char_it != input_char_list.end() && next_char_it->code_point == 9 )
				{
					ignore_tabs = true;
				}
				else
				{
					indentation_offset = 0;
				}
				if( !render_style->linebreak )
				{
					// consume line break:
					input_char_it++;
					// and stop rendering before line break:
					break;
				}
				else
				{
					// include line breaks
					RenderChar* render_char = new RenderChar( input_char_it, glyph_index, current_face );
					render_char->x = pen.x/64;
					render_char->y = -pen.y/64;
					render_char_list->push_back( render_char );
					if( !render_style->multiline )
					{
						// we can stop after having pushed the new line character, when in single line mode:
						break;
					}
					else
					{
						continue;
					}
				}
			}
			else if( input_char_it->code_point==9 
					// ignore tabs at the beginning of lines following already indented text blocks
					// those are just indentation-continuation markers
					&& !ignore_tabs )
			{
				// insert dynamically sized tab space, based on pixel size and position:
				int tab_width = pixel_size * render_style->tab_spaces;
				x = pen.x/64;
				int tab_offset = tab_width - (x % tab_width);
				pen.x += 64 * tab_offset;
				// store tab end position as new indentation_offset if there is not already one
				if( !indentation_offset )
				{
					indentation_offset = x + tab_offset;
				}
				continue;
			}
			WARN(  "error translating character code: " << input_char_it->code_point );
			continue;
        }
		ignore_tabs = false;
        FT_Set_Transform( current_face, 0, &pen );
        LOG( "glyph index: " << glyph_index );
        error = FT_Load_Glyph( current_face, glyph_index, FT_LOAD_RENDER );
        if( error )
        {
			WARN( "error loading glyph index: " << glyph_index << "(" << ft_errors[error] << ")" );
			continue;
        }
        char buffer[1000]; buffer[0]=0;
        FT_Get_Glyph_Name( current_face, glyph_index, buffer, 1000 );
        LOG( "glyph name: " << buffer );
        
        RenderChar* render_char = new RenderChar( input_char_it, glyph_index, current_face );
        render_char->x = pen.x/64;
        render_char->y = -pen.y/64;
		FT_GlyphSlot& glyph = current_face->glyph;
		FT_Bitmap& bitmap = current_face->glyph->bitmap;
		LOG( "w/r: " << bitmap.width << "/" << bitmap.rows << " t: " << glyph->bitmap_top );
		render_char->width = glyph->advance.x/64;
		render_char->height = bitmap.rows;
		if( !bitmap.buffer )
		{
			LOG( "got no bitmap for current glyph" );
		}
		if( bitmap.rows > line_height ) line_height = bitmap.rows;
		if( render_char->x+render_char->width > render_screen.res_x 
			&& (!render_style->linebreak 
				|| !render_style->multiline) )
		{
			if( !render_style->multiline )
			{
				// push exceeding character so we can find the right position to break later:
				render_char_list->push_back( render_char );
			}
			else
			{
				// clean up clipped character...
				delete render_char;
			}
			// and stop rendering before line break:
			break;
		}
		pen.x += glyph->advance.x;
        render_char_list->push_back( render_char );
    }
	
	// nothing to render -> early return
	if( !render_char_list->size() ) 
	{
		// erase all successfully processed input characters from input char list:
		input_char_list.erase( input_char_list.begin(), input_char_it );
		return RenderInfo( x, y, 0, 0 );
	}
    
    // insert line breaks
    RenderCharList::iterator prev_whitespace_it = render_char_list->end();
    RenderCharList::iterator last_line_it = render_char_list->begin();
    int x_correction = 0;
    int y_correction = 0;
    int x_limit = render_screen.res_x;
    int line_count = 1;
    if( render_style->center_x )
    {
        x_limit -= x;
    }
	if( render_style->linebreak )
	{
		for( RenderCharList::iterator rchar_it=render_char_list->begin(); 
				rchar_it!=render_char_list->end(); rchar_it++ )
		{
			LOG( "c=" << (*rchar_it)->uc_char.code_point << " x=" << (*rchar_it)->x 
				<< " y=" << (*rchar_it)->y << " xc=" << x_correction 
				<< " yc=" << y_correction );
			(*rchar_it)->x += x_correction;
			(*rchar_it)->y += y_correction;
			if( (*rchar_it)->uc_char.code_point == 32 )
			{
				prev_whitespace_it = rchar_it;
			}
			if( (*rchar_it)->x+(*rchar_it)->width > render_screen.res_x 
				|| ( (*rchar_it)->uc_char.code_point == 10 
					&& (*rchar_it)->x > x /*prevent endless loop*/ ) )
			{
				if( (*rchar_it)->uc_char.code_point == 10 )
				{
					// prevent successive line breaks at this position:
					prev_whitespace_it = render_char_list->end();
				}
				// go back to previous white space if possible and adjust 
				// correction values for next line
				if( prev_whitespace_it != render_char_list->end() )
				{
					// re-correct already corrected chars in line since prev whitespace:
					RenderCharList::iterator reloc_char_it=prev_whitespace_it;
					reloc_char_it++;
					RenderCharList::iterator reloc_end_char_it=rchar_it;
					reloc_end_char_it++;
					for( ; reloc_char_it!=reloc_end_char_it; reloc_char_it++ )
					{
						(*reloc_char_it)->x -= x_correction;
						(*reloc_char_it)->y -= y_correction;
					}
					rchar_it = ++prev_whitespace_it;
					// prevent successive line breaks at this position:
					prev_whitespace_it = render_char_list->end();
				}
				else
				{
					(*rchar_it)->x -= x_correction;
					(*rchar_it)->y -= y_correction;
				}
				// delete/reject characters after line break, when not in multiline mode
				if( !render_style->multiline )
				{
					while( (*rchar_it)->uc_char.code_point==10 
						|| (*rchar_it)->uc_char.code_point==32 )
					{
						// consume (not reject) whitespaces
						rchar_it++;
					}
					for( RenderCharList::iterator del_char_it=rchar_it; 
						del_char_it!=render_char_list->end(); 
						del_char_it++ )
					{
						if( *del_char_it ) delete *del_char_it;
					}
					render_char_list->erase( rchar_it, render_char_list->end() );
					break;
				}
				y_correction += line_height+1;
				x_correction = x - (*rchar_it)->x;
				RenderCharList::iterator prev_last_line_it = last_line_it;
				last_line_it = rchar_it;
				line_count++;
				rchar_it--;
				(*prev_last_line_it)->curr_line_end_char = *rchar_it;
			}
		}
	}
    (*last_line_it)->curr_line_end_char = *render_char_list->rbegin();

    if( render_style->center_x )
    {
        // x-center every line
        x_correction = 0;
        for( RenderCharList::iterator rchar_it=render_char_list->begin();
                rchar_it!=render_char_list->end(); rchar_it++ )
        {
            if( (*rchar_it)->curr_line_end_char )
            {
                x_correction = ( render_screen.res_x - x 
                        - (*rchar_it)->curr_line_end_char->x
                        - (*rchar_it)->curr_line_end_char->width ) / 2;
            }
            (*rchar_it)->x += x_correction;
        }
    }
    if( render_style->center_y )
    {
        // y-center all lines
        // TODO
    }

    // render characters at final locations
    int max_x = 0;
    int max_y = 0;
    for( RenderCharList::iterator rchar_it=render_char_list->begin(); 
            rchar_it!=render_char_list->end(); rchar_it++ )
    {
		if( !(*rchar_it)->glyph_index )
			continue;
        int cur_rx = (*rchar_it)->x+(*rchar_it)->width-1;
        if( cur_rx > max_x ) max_x = cur_rx;
        int cur_by = (*rchar_it)->y+(*rchar_it)->height-1;
        if( cur_by > max_y ) max_y = cur_by;
        pen.x = (*rchar_it)->x*64;
        pen.y = (*rchar_it)->y*-64;
        FT_Set_Transform( (*rchar_it)->face, 0, &pen );
        error = FT_Load_Glyph( (*rchar_it)->face, (*rchar_it)->glyph_index, FT_LOAD_RENDER );
        if( error )
        {
            std::stringstream msg;
			msg << "error loading glyph index: " << (*rchar_it)->glyph_index 
				<< "(" << ft_errors[error];
			throw ERROR( msg.str() );
        }
        FT_GlyphSlot& glyph = (*rchar_it)->face->glyph;
        FT_Bitmap& bitmap = (*rchar_it)->face->glyph->bitmap;
		if( !render_screen.res_x || ! render_screen.res_y || !render_screen.base_address )
		{
			throw ERROR( "FreeTypeRenderer::render(): target buffer not initialized or of zero size" );
		}
        for( int row=0; bitmap.buffer && row<bitmap.rows; row++ )
        {
			int dest_y = row+(line_height-glyph->bitmap_top);
			if( dest_y < 0 || dest_y >= render_screen.res_y )
				continue;
            for( int pixel=0; pixel<bitmap.width; pixel+=2 )
            {
				int dest_x = pixel + glyph->bitmap_left;
				if( dest_x < 0 || dest_x > (render_screen.res_x-2) )
					continue;
                u16 value = 0;
                if( pixel < bitmap.width-1 )
                {
                    value = (bitmap.buffer[row*bitmap.pitch+pixel+1] << 8)
                                + bitmap.buffer[row*bitmap.pitch+pixel];
                }
                else
                {
                    value = bitmap.buffer[row*bitmap.pitch+pixel];
                }
                u16* bg_gfx_ptr = render_screen.base_address;
                u16* base_address = bg_gfx_ptr
                        + (row+(line_height-glyph->bitmap_top))*render_screen.res_x/2
                        + pixel/2 + glyph->bitmap_left/2;
                *base_address |= value;
            }
        }
    }
    int height = max_y-y;
    if( line_height > height ) height = line_height;
	RenderInfo info = RenderInfo( x, y, max_x-x, height );
	info.indentation_offset = indentation_offset;
	// erase all successfully processed input characters from input char list:
	input_char_list.erase( input_char_list.begin(), (*render_char_list->rbegin())->input_char_it );
	input_char_list.erase( (*render_char_list->rbegin())->input_char_it );
    return info;
}

