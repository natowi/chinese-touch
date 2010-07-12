#ifndef IMAGE_PNG
#define IMAGE_PNG

#include <string>
#include <nds.h>

void read_png( std::string file_name, u16* dest_buffer, int& width, int& height, char alpha_threshold = 0xff );

#endif // IMAGE_PNG
