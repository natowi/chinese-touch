#ifndef IMAGE_PNG
#define IMAGE_PNG

#include <string>
#include <nds.h>

void read_png( const std::string& file_name, u16*& dest_buffer, int& width, int& height, int left=0, int top=0, char alpha_threshold = 0xff );

#endif // IMAGE_PNG
