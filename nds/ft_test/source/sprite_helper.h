#ifndef SPRITE_HELPER_H

void tile_32x16_8bpp_sprite( u8* source_buffer, u8* dest_buffer );

void set_16bpp_sprite_opague( u16* vram, int width, int height, u16 transparent_value=0 );

#endif //SPRITE_HELPER_H