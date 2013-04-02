/*
 * scene.h
 *
 *  Created on: Mar 29, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_SCENE_H_
#define SCENE_SCENE_H_

#include <sifteo.h>
#include "element.h"
#include "defines.h"
#include "modehandler.h"
#include "elementhandler.h"
#include "loadingscreen.h"
#include "motionmapper.h"

namespace Scene
{
	const uint8_t HIDE 			= 0x80;
	const uint8_t ATTACHED 		= 0x40;
	const uint8_t DIRTY			= 0x20;
	const uint8_t MODE_MASK		= 0x1F;
	const uint8_t NO_MODE		= 0x1F;
	const uint8_t STATE_MASK	= (ATTACHED | MODE_MASK);

	const uint8_t NO_UPDATE		= 0x00;
	const uint8_t FULL_UPDATE	= 0xFF;

	void initialize();

	void setModeHandler(ModeHandler *p);
	void setElementHandler(ElementHandler *p);
	void setLoadingScreen(LoadingScreen *p);
	void setMotionMapper(MotionMapper *p);

	// API to build a scene directly into the buffer
	Element *beginScene();
	void endScene(Element *);

	// API's to copy a scene from ROM, or to set the current scene directly
	void copyScene(Element *scene, uint16_t count);
	void setScene(Element *scene, uint16_t count);

	int32_t execute();

	// perhaps not publicly exported
	uint8_t updateCount();
	int32_t doUpdate(Element *element, uint8_t frames=0);
	bool doRedraw(Element *element);

}

#endif /* SCENE_H_ */
