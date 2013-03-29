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
		uint8_t update;			// framecount before next update
		uint8_t mode;			// the screen mode index, set Scene::HIDE to stop an item getting drawn
		uint8_t cube;			// the cube for this entity (or undefined)
		union
		{
			void *object;		// pointer to user defined object
			uint8_t data[4];	// user defined data
		};
	};
}
#endif /* ELEMENT_H_ */
