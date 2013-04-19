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
#include "handler.h"
#include "loadingscreen.h"
#include "motionmapper.h"

namespace Scene
{
	const uint8_t HIDE 			= 0x80;

	const uint8_t NO_MODE		= 0xFF;

	const uint8_t NO_UPDATE		= 0x00;
	const uint8_t FULL_UPDATE	= 0xFF;

	void initialize();

	void setLoadingScreen(LoadingScreen &p);
	void setMotionMapper(MotionMapper &p);
	void clearMotionMapper();

	void setFrameRate(float frameRate);

	// API to build a scene directly into the buffer
	void beginScene();
	void addElement(uint8_t type, uint8_t cube, uint8_t mode, uint8_t update, uint8_t autoupdate=0, void *object=0);
	void endScene();

	int32_t execute(Handler &handler);
	void close();

	Element &getElement(uint16_t index);

	// yielding system call traps.
	bool paint();
	bool paintUnlimited();
	bool yield();
}

#endif /* SCENE_H_ */
