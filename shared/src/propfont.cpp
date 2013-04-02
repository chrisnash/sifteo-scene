/*
 * propfont.cpp
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */
#include <sifteo.h>
using namespace Sifteo;

extern uint8_t fontData[];

namespace Font
{
	const uint8_t fontHeight = 7;
	const uint16_t fontBase = 32;
	const uint16_t fontMax = 128;
	const uint16_t stride = fontMax - fontBase;
	const uint16_t fontReplace = (uint16_t)'?';

	uint8_t widthOf(uint16_t x)
	{
		x = ((x<fontBase)||(x>=fontMax)) ? fontReplace : x;
		x -= fontBase;
		return fontData[x];
	}

	void glyphAt(VideoBuffer &v, uint16_t x, uint16_t y, uint16_t g, uint8_t &w)
	{
		g = ((g<fontBase)||(g>=fontMax)) ? fontReplace : g;
		g -= fontBase;
		w = fontData[g];
		v.fb128.bitmap(vec(x,y), vec(w,fontHeight), fontData + g + stride, stride);
	}

	void drawAt(VideoBuffer &v, UInt2 topLeft, const char *text)
	{
		uint16_t x = topLeft.x;
		for(const char *p=text; *p; *p++)
		{
			uint8_t w;
			glyphAt(v, x, topLeft.y, *p, w);
			x += w;
		}
	}

	void drawCentered(VideoBuffer &v, UInt2 topLeft, UInt2 size, const char *text)
	{
		uint16_t width = 0;
		for(const char *p=text; *p; p++)
		{
			width += widthOf(*p);
		}
		drawAt(v, vec( topLeft.x + ((size.x-width)>>1), topLeft.y + ((size.y-fontHeight)>>1) ), text);
	}
}
