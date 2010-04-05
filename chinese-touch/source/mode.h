#ifndef MODE_H
#define MODE_H

#include "freetype_renderer.h"

class Mode
{
public:
	virtual void render( Screen screen )=0;
	virtual void run_until_exit()=0;
};

#endif // MODE_H