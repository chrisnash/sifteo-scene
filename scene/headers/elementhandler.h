/*
 * elementhandler.h
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_ELEMENTHANDLER_H_
#define SCENE_ELEMENTHANDLER_H_

namespace Scene
{
	class ElementHandler
	{
		friend int32_t doRedraw();
	protected:
		virtual void drawElement(Element *el, Sifteo::VideoBuffer &v) = 0;
		virtual int32_t updateElement(Element *el, uint8_t fc=0) = 0;
	};
}

#endif /* ELEMENTHANDLER_H_ */
