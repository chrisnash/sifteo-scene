/*
 * element.h
 *
 *  Created on: Mar 29, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_ELEMENT_H_
#define SCENE_ELEMENT_H_

#include <sifteo.h>

namespace Scene
{
	class Element
	{
	public:
		uint8_t type;			// the element type
		uint8_t cube;			// the cube for this entity (or undefined)
		uint8_t update;			// framecount before next update
		uint8_t autoupdate;		// framecount to put back in update register for automatic looping
		union
		{
			void *object;		// pointer to user defined object
			uint8_t data[4];	// user defined data
		};

		Element &repaint();			// repaint this element
		Element &clearUpdate();
		Element &setUpdate(uint8_t update=1);
		Element &fullUpdate();
		Element &setAutoupdate(uint8_t autoupdate);
		Element &show();
		Element &hide();
	};
}
#endif /* ELEMENT_H_ */
