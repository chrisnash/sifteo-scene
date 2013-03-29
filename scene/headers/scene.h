/*
 * scene.h
 *
 *  Created on: Mar 29, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_SCENE_H_
#define SCENE_SCENE_H_

#include "element.h"

namespace Scene
{
	const uint8_t HIDE 			= 0x80;
	const uint8_t ATTACHED 		= 0x40;
	const uint8_t DIRTY			= 0x20;
	const uint8_t MODE_MASK		= 0x1F;
	const uint8_t NO_MODE		= 0x1F;
	const uint8_t STATE_MASK	= (ATTACHED | MODE_MASK);

	void initialize();

	// API to build a scene directly into the buffer
	Element *beginScene();
	void endScene(Element *);

	// API's to copy a scene from ROM, or to set the current scene directly
	void copyScene(Element *scene, uint16_t count);
	void setScene(Element *scene, uint16_t count);

	int32_t execute();
}

#endif /* SCENE_H_ */
