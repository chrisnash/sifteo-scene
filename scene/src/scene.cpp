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

	bool fullRefresh = true;

	CubeSet attentionCubes;

	class NoMotion : public MotionMapper
	{
		void attachMotion(uint8_t cube, Sifteo::CubeID parameter) {}
	}
	noMotion;

	MotionMapper *motionMapper = &noMotion;

	TimeTicker frameRate(60.0);
	TimeStep timeStep;

	class CubeMapping
	{
		uint8_t toPhysical[CUBE_ALLOCATION];
		uint8_t toLogical[CUBE_ALLOCATION];
	public:
		void initialize()
		{
			for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
			{
				toPhysical[i] = CubeID::UNDEFINED;
				toLogical[i] = CubeID::UNDEFINED;
			}
		}

		// allocate a physical cube to a logical slot
		// return true if anything changed
		bool allocate(uint8_t physical)
		{
			uint8_t logical = toLogical[physical];
			if(logical != CubeID::UNDEFINED) return false;
			logical = 0;
			while(toPhysical[logical] != CubeID::UNDEFINED) logical++;
			toPhysical[logical] = physical;
			toLogical[physical] = logical;
			motionMapper->attachMotion(logical, CubeID(physical));
			return true;
		}

		// deallocate a physical cube
		// return true if anything changed
		bool deallocate(uint8_t physical)
		{
			uint8_t logical = toLogical[physical];
			if(logical == CubeID::UNDEFINED) return false;
			toLogical[physical] = CubeID::UNDEFINED;
			toPhysical[logical] = CubeID::UNDEFINED;
			CubeID(physical).detachMotionBuffer();

			// bubble down allocated cubes to fill any spaces
			// basically keep moving the highest assigned logical cube to the lowest unassigned logical cube
			uint8_t highest = CUBE_ALLOCATION - 1;
			while((highest>logical) && (toPhysical[highest] == CubeID::UNDEFINED)) highest--;
			if(highest > logical)
			{
				physical = toPhysical[highest];
				toPhysical[highest] = CubeID::UNDEFINED;
				CubeID(physical).detachMotionBuffer();

				// bind physical to logical, bind highest to nowhere
				toLogical[physical] = logical;
				toPhysical[logical] = physical;
				motionMapper->attachMotion(logical, CubeID(physical));
			}

			return true;
		}

		uint8_t physical(uint8_t logical)
		{
			return toPhysical[logical];
		}
		uint8_t logical(uint8_t physical)
		{
			return toLogical[physical];
		}

		void reattachMotion()
		{
			for(uint8_t logical=0; logical<CUBE_ALLOCATION; logical++)
			{
				uint8_t physical = toPhysical[logical];
				if(physical != CubeID::UNDEFINED)
				{
					CubeID cube(physical);
					cube.detachMotionBuffer();
					motionMapper->attachMotion(logical, physical);
				}
			}
		}
	}
	cubeMapping;

	// Event handler
	class EventHandler
	{
	public:
		void onCubeRefresh(unsigned cubeID)
		{
			fullRefresh = true;
		}
		void onCubeChange(unsigned cubeID)
		{
			attentionCubes.mark(cubeID);
		}

		void initialize()
		{
			Sifteo::Events::cubeRefresh.set(&EventHandler::onCubeRefresh, this);

			cubeMapping.initialize();
			attentionCubes = CubeSet::connected();

			Sifteo::Events::cubeConnect.set(&EventHandler::onCubeChange, this);
			Sifteo::Events::cubeDisconnect.set(&EventHandler::onCubeChange, this);
		}
	}
	eventHandler;

	void initialize()
	{
		fullRefresh = true;
		eventHandler.initialize();
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

	void setMotionMapper(MotionMapper *p)
	{
		motionMapper = (p == 0) ? &noMotion : p;
		cubeMapping.reattachMotion();
	}

	void setFrameRate(float fr)
	{
		frameRate = TimeTicker(fr);
		timeStep.next();				// reset the time stepper
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

	int32_t doRedraw()
	{
		ASSERT(modeHandler != 0);
		ASSERT(elementHandler != 0);
		ASSERT(loadingScreen != 0);

		if(!attentionCubes.empty())
		{
			CubeSet current = CubeSet::connected();
			unsigned i;
			while(attentionCubes.clearFirst(i))
			{
				if(current.test(i))
				{
					// cube is connected
					if(cubeMapping.allocate(i))
					{
						fullRefresh = true;
					}
				}
				else
				{
					// cube is not connected
					CubeID(i).detachVideoBuffer();
					if(cubeMapping.deallocate(i))
					{
						fullRefresh = true;
					}
				}
			}
		}

		if(fullRefresh)
		{
			redraw = initialDraw;
			for(uint8_t i=0; i<CUBE_ALLOCATION; i++) currentModes[i] = NO_MODE;
			fullRefresh = false;
		}

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
			BitArray<CUBE_ALLOCATION> dirty(0,0);		// mark cubes that are drawn on, you can't mode switch until they paint
			BitArray<SCENE_MAX_SIZE> todo = redraw;
			redraw.clear();

			unsigned i;
			while(todo.clearFirst(i))
			{
				Element *element = sceneData + i;
				uint8_t cube = element->cube;
				uint8_t physical = cubeMapping.physical(cube);
				if(physical == CubeID::UNDEFINED)
				{
					continue;		// you can't see this cube, so don't draw it
				}
				if(cubesLoading.test(physical))
				{
					redraw.mark(i);	// we can't do this object yet
					continue;		// skip any cubes in download mode
				}

				// check to see if we are in the right mode (ignore top bits)
				uint8_t currentMode = currentModes[cube];
				uint8_t elementMode = element->mode;

				if( ((currentMode & MODE_MASK) != (elementMode & MODE_MASK)) && !dirty.test(cube))
				{
					// in the wrong mode, so need to do a mode switch
					// but first you need to check assets for this mode
					AssetConfiguration<ASSET_CAPACITY> *pAssets = modeHandler->requestAssets(cube, elementMode & MODE_MASK);
					bool alreadyInstalled = true;
					if(pAssets)
					{
						for(AssetConfigurationNode node : *pAssets)
						{
							if(!node.group()->isInstalled(physical))
							{
								alreadyInstalled = false;
								break;
							}
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
						assetLoader.start(*pAssets, CubeSet(physical));
						cubesLoading.mark(physical);
						currentMode = currentModes[cube] = NO_MODE;
						CubeID(physical).detachVideoBuffer();
						loadingScreen->init(cube, vid[cube]);
						vid[cube].attach(physical);
						loadingScreen->onAttach(cube, vid[cube]);
					}
					else
					{
						// ok to perform the mode switch, and may as well attach right now
						LOG("SCENE: Mode switch of cube %d\n", cube);
						CubeID(physical).detachVideoBuffer();
						modeHandler->switchMode(cube, elementMode & MODE_MASK, vid[cube]);
						currentMode = currentModes[cube] = elementMode & MODE_MASK;
						vid[cube].attach(physical);
					}
				}

				if((currentMode & MODE_MASK) == (elementMode & MODE_MASK))
				{
					LOG("SCENE: Draw element %d\n", i);
					elementHandler->drawElement(element, vid[cube]);
					dirty.mark(cube);
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
				uint8_t logical = cubeMapping.logical(i);
				loadingScreen->update(logical, progress, vid[logical]);
				if(assetLoader.isComplete(i))
				{
					LOG("SCENE: Completed download to cube %d\n", i);
					cubesLoading.clear(i);
					timeStep.next();			// reset the frame clock
				}
			}
			if(cubesLoading.empty())
			{
				assetLoader.finish();
			}
		}
		while(!redraw.empty() || !cubesLoading.empty());

		timeStep.next();									// get a time sample
		uint8_t fc = frameRate.tick(timeStep.delta());		// figure out how much to advance the clock
		int32_t exitCode = 0;

		for(uint16_t i = 0; (i<sceneSize); i++)
		{
			Element *element = sceneData + i;
			if(element->update == Scene::FULL_UPDATE)
			{
				exitCode = elementHandler->updateElement(element, fc);
				if(exitCode !=0) return exitCode;
			}
			else if(element->update != 0)
			{
				uint8_t ur = fc;
				while(ur >= element->update)
				{
					ur -= element->update;
					element->update = 0;
					exitCode = elementHandler->updateElement(element);
					if(exitCode !=0) return exitCode;
				}
				element->update -= ur;
			}
		}
		return exitCode;
	}

	int32_t execute()
	{
		int32_t exitCode;
		while( (exitCode=doRedraw()) == 0);
		return exitCode;
	}
}
