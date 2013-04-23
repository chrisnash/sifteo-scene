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

namespace Scene
{
	const uint8_t HIDE 			= 0x80;

	const uint8_t NO_MODE		= 0xFF;

	const uint8_t NO_UPDATE		= 0x00;
	const uint8_t FULL_UPDATE	= 0xFF;

	const uint8_t SYNC_NONE		= 0x00;
	const uint8_t SYNC_DOWNLOAD	= 0x01;
	const uint8_t SYNC_FULL		= 0x02;

	void initialize();

	void setLoadingScreen(LoadingScreen &p);

	void setFrameRate(float frameRate);
	void setSyncMode(uint8_t sm);

	// API to build a scene directly into the buffer
	void beginScene();
	uint16_t addElement(uint8_t type, uint8_t cube, uint8_t mode, uint8_t update=NO_UPDATE, uint8_t autoupdate=NO_UPDATE, void *object=NULL);
	int32_t execute(Handler &handler);

	Element &getElement(uint16_t index);

	bool neighborAt(uint8_t cube, uint8_t side, uint8_t &otherCube, uint8_t &otherSide);
	void setFrameThreshold(uint8_t ft);
}

#endif /* SCENE_H_ */
