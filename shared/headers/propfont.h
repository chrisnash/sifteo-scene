/*
 * propfont.h
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */

#ifndef PROPFONT_H_
#define PROPFONT_H_

#include <sifteo.h>
using namespace Sifteo;

namespace Font
{
	void drawCentered(VideoBuffer &v, UInt2 topLeft, UInt2 size, const char *text);
	void drawAt(VideoBuffer &v, UInt2 topLeft, const char *text);
}

#endif /* PROPFONT_H_ */
