#ifndef pixel_h
#define pixel_h

#include "SDL.h"

struct Pixel
{
	Pixel() : red(0), green(0), blue(0), alpha(255)
	{
	};
	Uint8 red;
	Uint8 green;
	Uint8 blue;
	Uint8 alpha;
};

#endif