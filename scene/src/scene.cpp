/*
 * scene.cpp
 *
 *  Created on: Mar 29, 2013
 *      Author: chrisnash
 */
#include <scene.h>
#include "../headers/defines.h"

using namespace Sifteo;

namespace Scene
{
	// the items that need a redraw now
	BitArray<SCENE_MAX_SIZE> redraw;
	// the items that need a redraw in order to do a full render
	BitArray<SCENE_MAX_SIZE> initialDraw;

	// current cube modes
	uint8_t currentModes[CUBE_ALLOCATION];

	// the current scene
	Element *sceneData;
	// the current scene size
	uint16_t sceneSize;
	// the scene buffer available to build scenes on the fly
	Element sceneBuffer[SCENE_MAX_SIZE];

	void initialize()
	{
		for(uint8_t i=0; i<CUBE_ALLOCATION; i++) currentModes[i] = NO_MODE;
	}

	// preprocess the scene to get the initial draw flags (hide scene elements)
	void preprocessScene()
	{
		initialDraw = BitArray<SCENE_MAX_SIZE>(0, sceneSize);
		for(uint16_t i = 0; (i<sceneSize); i++)
		{
			Element *element = sceneData + i;
			if(element->mode & Scene::HIDE)
			{
				element->mode &= ~Scene::HIDE;
				initialDraw.clear(i);
			}
		}
		redraw = initialDraw;
	}

	void setScene(Element *scene, uint16_t size)
	{
		ASSERT(scene != 0);
		ASSERT(size < SCENE_MAX_SIZE);

		sceneData = scene;
		sceneSize = size;
		preprocessScene();
	}

	void copyScene(Element *scene, uint16_t size)
	{
		ASSERT(scene != 0);
		ASSERT(size < SCENE_MAX_SIZE);

		Sifteo::memcpy((uint8_t*)sceneBuffer, (uint8_t*)scene, size*sizeof(Element));
		setScene(sceneBuffer, size);
	}

	Element *beginScene()
	{
		return sceneBuffer;
	}

	void endScene(Element *p)
	{
		setScene(sceneBuffer, p - sceneBuffer);
	}

	void doRedraw()
	{
		// the redraw loop goes thru all elements whose redraw flag is set.
		// if the mode matches, then that element can be drawn. Otherwise we need to consider
		// performing a mode switch. If a mode switch element is found, we should find
		// any other cubes that also need a mode switch.

		// a mode switch might require download of new assets. It may require a buffer detach and change,
		// (which might mean an attach, paint, finish if there is stuff drawn in there). If a download
		// is required, then we best look for any other cubes that need a download too. Basically though
		// we do as much as is possible, and in a panic we just set that bit for the next pass. Note there's
		// nothing to stop us drawing on other cubes if one is downloading ;)

		BitArray<SCENE_MAX_SIZE> todo = redraw;
		BitArray<SCENE_MAX_SIZE> defer(0,0);	// nothing set

		while(!todo.empty())
		{
			defer = todo;
		}
	}

	int32_t execute()
	{
		while(1)
		{
			System::paint();
		}
	}
}
