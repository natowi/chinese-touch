#ifndef SPRITE_HELPER_H

void tile_8bpp_sprite( u8* source_buffer, u8* dest_buffer, int width, int height );

void set_16bpp_sprite_opague( u16* vram, int width, int height=1, u16 transparent_value=0 );

void log_oam_state_sub();

#endif //SPRITE_HELPER_H