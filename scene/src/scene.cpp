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

	// video buffers
	VideoBuffer vid[CUBE_ALLOCATION];

	// Asset loader
	AssetLoader assetLoader;

	ModeHandler *modeHandler = 0;
	ElementHandler *elementHandler = 0;
	LoadingScreen *loadingScreen = 0;

	void initialize()
	{
		for(uint8_t i=0; i<CUBE_ALLOCATION; i++) currentModes[i] = NO_MODE;
	}

	void setModeHandler(ModeHandler *p)
	{
		modeHandler = p;
	}

	void setElementHandler(ElementHandler *p)
	{
		elementHandler = p;
	}

	void setLoadingScreen(LoadingScreen *p)
	{
		loadingScreen = p;
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
		ASSERT(modeHandler != 0);
		ASSERT(elementHandler != 0);
		ASSERT(loadingScreen != 0);

		// the redraw loop goes thru all elements whose redraw flag is set.
		// if the mode matches, then that element can be drawn. Otherwise we need to consider
		// performing a mode switch. If a mode switch element is found, we should find
		// any other cubes that also need a mode switch.

		// a mode switch might require download of new assets. It may require a buffer detach and change,
		// (which might mean an attach, paint, finish if there is stuff drawn in there). If a download
		// is required, then we best look for any other cubes that need a download too. Basically though
		// we do as much as is possible, and in a panic we just set that bit for the next pass. Note there's
		// nothing to stop us drawing on other cubes if one is downloading ;)

		BitArray<CUBE_ALLOCATION> cubesLoading(0,0);	// nothing set

		// for at least one paint cycle
		do
		{
			BitArray<SCENE_MAX_SIZE> todo = redraw;
			redraw.clear();

			unsigned i;
			while(todo.clearFirst(i))
			{
				Element *element = sceneData + i;
				uint8_t cube = element->cube;
				if(cubesLoading.test(cube))
				{
					redraw.mark(i);	// we can't do this object yet
					continue;		// skip any cubes in download mode
				}

				// check to see if we are in the right mode (ignore top bits)
				uint8_t currentMode = currentModes[cube];
				uint8_t elementMode = element->mode;

				if((currentMode & MODE_MASK) != (elementMode & MODE_MASK))
				{
					// in the wrong mode, so need to do a mode switch
					// but first you need to check assets for this mode
					AssetConfiguration<ASSET_CAPACITY> *pAssets = modeHandler->requestAssets(elementMode & MODE_MASK, cube);
					bool alreadyInstalled = true;
					for(AssetConfigurationNode node : *pAssets)
					{
						if(!node.group()->isInstalled(cube))
						{
							alreadyInstalled = false;
							break;
						}
					}
					// if not already installed, you need to start a loader on this cube
					if(!alreadyInstalled)
					{
						if(cubesLoading.empty())
						{
							LOG("SCENE: Opening a new loader\n");
							assetLoader.init();
						}
						LOG("SCENE: Beginning download to cube %d\n", cube);
						assetLoader.start(*pAssets, CubeSet(cube));
						cubesLoading.mark(cube);
						currentMode = currentModes[cube] = NO_MODE;
						CubeID(cube).detachVideoBuffer();
						loadingScreen->init(cube, vid[cube]);
						vid[cube].attach(cube);
						loadingScreen->onAttach(cube, vid[cube]);
					}
					else
					{
						// ok to perform the mode switch, and may as well attach right now
						LOG("SCENE: Mode switch of cube %d\n", cube);
						CubeID(cube).detachVideoBuffer();
						modeHandler->switchMode(cube, elementMode & MODE_MASK, vid[cube]);
						currentMode = currentModes[cube] = elementMode & MODE_MASK;
						vid[cube].attach(cube);
					}
				}

				if((currentMode & MODE_MASK) == (elementMode & MODE_MASK))
				{
					LOG("SCENE: Draw element %d\n", i);
					elementHandler->drawElement(element, vid[cube]);
				}
				else
				{
					redraw.mark(i);
				}
			}

			// yield a while
			System::paint();

			// check for loading cubes that have finished
			for(uint8_t i : cubesLoading)
			{
				float progress = assetLoader.cubeProgress(i);
				loadingScreen->update(i, progress, vid[i]);
				if(assetLoader.isComplete(i))
				{
					LOG("SCENE: Completed download to cube %d\n", i);
					cubesLoading.clear(i);
				}
			}
			if(cubesLoading.empty())
			{
				assetLoader.finish();
			}
		}
		while(!redraw.empty() || !cubesLoading.empty());
	}

	int32_t execute()
	{
		while(1)
		{
			doRedraw();
		}
	}
}
