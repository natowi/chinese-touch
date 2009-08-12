#include <iostream>

#include <nds.h>

#include "freetype_renderer.h"

#undef __FTERRORS_H__
#define FT_ERROR_START_LIST     ErrorMap ft_errors; void FT_Init_Errors(){
#define FT_ERRORDEF( e, v, s )  ft_errors[e]=s;
#define FT_ERROR_END_LIST       };

#include FT_ERRORS_H


FreetypeRenderer::FreetypeRenderer( const std::string& han_font, 
        const std::string& latin_font ) : dpi_x(100), dpi_y(100), 
                        res_x(256), res_y(192)
{
    // FIXME: FreetypeRenderer sollte unabhängig vom Zielgerät funktionieren:
    // set the mode for 2 text layers and two extended background layers
	videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	this->bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);
	// lineare 15-bit-graupalette mit 256 indizes aufbauen:
	for( int i=0; i<256; i++ )
	{
	    int value = (int)( ((double)i)*(double)(1<<5)/256.0 );
	    BG_PALETTE[255-i] = /*(1 << 15) |*/ (value << 10) | (value << 5) | value;
	}

    FT_Init_Errors();
    std::cout << "ft_errors.size(): " << ft_errors.size() << std::endl;
    this->error = FT_Init_FreeType( &this->library );
    if( this->error )
    {
        std::cout << "error initializing freetype: " << ft_errors[this->error] 
                    << std::endl;
        return;
    }
    this->error = FT_New_Face( this->library, han_font.c_str(), 0, &this->han_face );
    if( this->error )
    {
        std::cout << "error loading chinese font: " << han_font << " (" 
                << ft_errors[this->error] << ")" << std::endl;
        return;
    }
    this->error = FT_New_Face( this->library, latin_font.c_str(), 0, &this->latin_face );
    if( this->error )
    {
        std::cout << "error loading chinese font: " << latin_font << " (" 
                << ft_errors[this->error] << ")" << std::endl;
        return;
    }
#if 0
    // Select charmap
    this->error = FT_Select_Charmap( this->han_face, FT_ENCODING_UNICODE );
    if( this->error )
    {
        std::cout << "error loading unicode charmap: " << ft_errors[this->error]
                << std::endl;
        return;
    }
#endif   
}

FreetypeRenderer::~FreetypeRenderer()
{
    FT_Done_Face( this->han_face );
    FT_Done_Face( this->latin_face );
    FT_Done_FreeType( this->library );
}

class RenderChar
{
public:
    RenderChar( unsigned long _char_code, unsigned long _glyph_index ) 
        : x(0), y(0), width(0), height(0), 
          char_code(_char_code), glyph_index(_glyph_index), 
          line_begin(false), curr_line_end_char(0) {}
public:
    int x, y, width, height;
    unsigned long char_code, glyph_index;
    bool line_begin;
    RenderChar* curr_line_end_char;
};
typedef std::list<RenderChar*> RenderCharList;

void FreetypeRenderer::render( const std::string& text, FT_Face& face, 
            int pixel_size, int x, int y, RenderStyle* render_style )
{
    // 1. unicode conversion
    CharList char_list;
    RenderCharList render_char_list;
    if( !utf8_to_ucs4((unsigned char*)text.c_str(), char_list) )
    {
        std::cout << "error in utf-8 input (non fatal)" << std::endl;
    }
    if( char_list.size()==0 ) return;
    
    // 2. initialize Freetype
    FT_Error error;
    error = FT_Set_Char_Size( face, 0, pixel_size*64, this->dpi_x, this->dpi_y );
    if( error )
    {
        std::cout << "error setting pixel size: " << ft_errors[error] << std::endl;
        return;
    }
    
    // 3. compute preliminary character offsets and dimensions
    int full_width = 0;
    int line_height = pixel_size;
    FT_Vector pen;
    pen.x = x * 64;
    pen.y = -y * 64;
    for( CharList::iterator char_it=char_list.begin(); 
            char_it!=char_list.end(); char_it++ )
    {
        // Load Char
        std::cout << "character code: " << *char_it << std::endl;
        //FT_UInt glyph_index = FT_Get_Name_Index(face, "a");
        FT_UInt glyph_index = FT_Get_Char_Index( face, *char_it );
        if( !glyph_index )
        {
            std::cout << "error translating character code: " << *char_it << std::endl;
            return;
        }
        FT_Set_Transform( face, 0, &pen );
        //std::cout << "glyph index: " << glyph_index << std::endl;
        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER );
        if( error )
        {
            std::cout << "error loading glyph index: " << glyph_index << std::endl;
            std::cout << ft_errors[error] << std::endl;
            return;
        }
        char buffer[1000];
        FT_Get_Glyph_Name( face, glyph_index, buffer, 1000 );
        //std::cout << "glyph name: " << buffer << std::endl;
        
        FT_GlyphSlot& glyph = face->glyph;
        FT_Bitmap& bitmap = face->glyph->bitmap;
        //std::cout << "w/r: " << bitmap.width << "/" << bitmap.rows 
        //            << " t: " << glyph->bitmap_top << std::endl;
        RenderChar* render_char = new RenderChar( *char_it, glyph_index );
        render_char->width = glyph->advance.x/64;
        render_char->height = bitmap.rows;
        render_char->x = pen.x/64;
        render_char->y = -pen.y/64;
        render_char_list.push_back( render_char );
        if( !bitmap.buffer )
        {
            std::cout << "warning: got no bitmap for current glyph" << std::endl;
        }
        if( bitmap.rows > line_height ) line_height = bitmap.rows;
        pen.x += glyph->advance.x;
    }
    full_width = pen.x/64-x;
    
    // 4. insert line breaks
    RenderCharList::iterator prev_whitespace_it = render_char_list.end();
    RenderCharList::iterator last_line_it = render_char_list.begin();
    int x_correction = 0;
    int y_correction = 0;
    int x_limit = this->res_x;
    int line_count = 1;
    if( render_style && render_style->center_x )
    {
        x_limit -= x;
    }
    for( RenderCharList::iterator rchar_it=render_char_list.begin(); 
            rchar_it!=render_char_list.end(); rchar_it++ )
    {
        std::cout << "c=" << (*rchar_it)->char_code << " x=" << (*rchar_it)->x 
            << " y=" << (*rchar_it)->y << " xc=" << x_correction 
            << " yc=" << y_correction << std::endl;
        (*rchar_it)->x += x_correction;
        (*rchar_it)->y += y_correction;
        if( (*rchar_it)->char_code == 32 )
        {
            prev_whitespace_it = rchar_it;
        }
        if( (*rchar_it)->x+(*rchar_it)->width > this->res_x )
        {
            // go back to previous white space if possible and adjust 
            // correction values for next line
            if( prev_whitespace_it != render_char_list.end() )
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
                prev_whitespace_it = render_char_list.end();
            }
            else
            {
                (*rchar_it)->x -= x_correction;
                (*rchar_it)->y -= y_correction;
            }
            y_correction += line_height;
            x_correction = x-(*rchar_it)->x;
            RenderCharList::iterator prev_last_line_it = last_line_it;
            last_line_it = rchar_it;
            line_count++;
            rchar_it--;
            (*prev_last_line_it)->curr_line_end_char = *rchar_it;
        }
    }
    (*last_line_it)->curr_line_end_char = *render_char_list.rbegin();

    if( render_style && render_style->center_x )
    {
        // 5. x-center every line
        x_correction = 0;
        for( RenderCharList::iterator rchar_it=render_char_list.begin();
                rchar_it!=render_char_list.end(); rchar_it++ )
        {
            if( (*rchar_it)->curr_line_end_char )
            {
                x_correction = ( this->res_x - x 
                        - (*rchar_it)->curr_line_end_char->x
                        - (*rchar_it)->curr_line_end_char->width ) / 2;
            }
            (*rchar_it)->x += x_correction;
        }
    }
    if( render_style && render_style->center_y )
    {
        // 6. y-center all lines
        // TODO
    }

    // 7. render characters at final locations
    for( RenderCharList::iterator rchar_it=render_char_list.begin(); 
            rchar_it!=render_char_list.end(); rchar_it++ )
    {
        pen.x = (*rchar_it)->x*64;
        pen.y = (*rchar_it)->y*-64;
        FT_Set_Transform( face, 0, &pen );
        error = FT_Load_Glyph( face, (*rchar_it)->glyph_index, FT_LOAD_RENDER );
        if( error )
        {
            std::cout << "error loading glyph index: " << (*rchar_it)->glyph_index << std::endl;
            std::cout << ft_errors[error] << std::endl;
            return;
        }
        FT_GlyphSlot& glyph = face->glyph;
        FT_Bitmap& bitmap = face->glyph->bitmap;
        for( int row=0; bitmap.buffer && row<bitmap.rows; row++ )
        {
            // FIXME Korrektur an bitmap.width um -1, damit wir mit der
            // 16-Bit-Kopie nicht die Puffergrenze verletzen und falsche
            // Pixel kopieren. Allerdings kopieren wir so manchmal zu wenig.
            for( int pixel=0; pixel<bitmap.width-1; pixel+=2 )
            {
                u16 value = (bitmap.buffer[row*bitmap.pitch+pixel+1] << 8)
                            + bitmap.buffer[row*bitmap.pitch+pixel];
                u16* bg_gfx_ptr = bgGetGfxPtr(this->bg3);
                u16* base_address = bg_gfx_ptr
                        + (row+(line_height-glyph->bitmap_top))*this->res_x/2
                        + pixel/2 + glyph->bitmap_left/2;
                if( base_address < bg_gfx_ptr
                    || base_address > bg_gfx_ptr+this->res_x*this->res_y-2 )
                {
                    continue;
                }
                *base_address = value;
            }
        }
    }
}

