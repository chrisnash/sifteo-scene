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
	bool scenelog = false;
#define SCENELOG if(scenelog) LOG
	uint64_t timestamp;
	bool scenetime = false;
#define START_TIMER if(scenetime) timestamp = SystemTime::now().uptimeNS();
#define END_TIMER if(scenetime) {uint64_t t = SystemTime::now().uptimeNS()-timestamp; SCENELOG("SCENE: elapsed time %uns\n", (uint32_t)t);}

	// the items that need a redraw now
	BitArray<SCENE_MAX_SIZE> redraw;
	// the items that need a redraw in order to do a full render
	BitArray<SCENE_MAX_SIZE> initialDraw;

	// current cube modes
	uint8_t currentModes[CUBE_ALLOCATION];

	// the current scene builder pointer
	Element *scenePointer;
	// the current scene size
	uint16_t sceneSize;
	// the scene buffer available to build scenes on the fly
	Element sceneBuffer[SCENE_MAX_SIZE];
	uint8_t elementModes[SCENE_MAX_SIZE];

	// video buffers
	VideoBuffer vid[CUBE_ALLOCATION];

	// Asset loader
	AssetLoader assetLoader;

	// The frame threshold. Log a warning if the update counter is too large.
	// The default value effectively disables this test.
	uint8_t frameThreshold = 0xFF;

	class DefaultLoadingScreen : public LoadingScreen
	{
	public:
		void init(uint8_t cube, Sifteo::VideoBuffer &v)
		{
			v.initMode(BG0_ROM);
		}
		void onAttach(uint8_t cube, Sifteo::VideoBuffer &v)
		{
			v.bg0rom.text(vec(4,7), "loading:", BG0ROMDrawable::GREEN_ON_WHITE);
		}
		void update(uint8_t cube, float progress, Sifteo::VideoBuffer &v)
		{
			v.bg0rom.hBargraph(vec(0,8), progress*128);
		}
	}
	defaultLoadingScreen;
	LoadingScreen *loadingScreen = &defaultLoadingScreen;

	bool fullRefresh = true;

	CubeSet attentionCubes;
	CubeSet attentionNeighbors;

	Neighborhood neighborhoods[CUBE_ALLOCATION];

	class NoMotion : public MotionMapper
	{
		void attachMotion(uint8_t cube, Sifteo::CubeID parameter) {}
		void detachMotion(uint8_t cube, Sifteo::CubeID parameter) {}
		void updateMotion(uint8_t cube) {}
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
			motionMapper->detachMotion(logical, physical);

			// bubble down allocated cubes to fill any spaces
			// basically keep moving the highest assigned logical cube to the lowest unassigned logical cube
			uint8_t highest = CUBE_ALLOCATION - 1;
			while((highest>logical) && (toPhysical[highest] == CubeID::UNDEFINED)) highest--;
			if(highest > logical)
			{
				physical = toPhysical[highest];
				toPhysical[highest] = CubeID::UNDEFINED;
				CubeID(physical).detachMotionBuffer();
				motionMapper->detachMotion(highest, physical);

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

		void detachAllMotion()
		{
			for(uint8_t logical=0; logical<CUBE_ALLOCATION; logical++)
			{
				uint8_t physical = toPhysical[logical];
				if(physical != CubeID::UNDEFINED)
				{
					CubeID cube(physical);
					cube.detachMotionBuffer();
					motionMapper->detachMotion(logical, cube);
				}
			}
		}

		void detachAllVideo()
		{
			for(uint8_t logical=0; logical<CUBE_ALLOCATION; logical++)
			{
				uint8_t physical = toPhysical[logical];
				if(physical != CubeID::UNDEFINED)
				{
					CubeID cube(physical);
					cube.detachVideoBuffer();
				}
			}
		}

		void attachAllMotion()
		{
			for(uint8_t logical=0; logical<CUBE_ALLOCATION; logical++)
			{
				uint8_t physical = toPhysical[logical];
				if(physical != CubeID::UNDEFINED)
				{
					CubeID cube(physical);
					motionMapper->attachMotion(logical, cube);
				}
			}
		}

		void updateAllMotion()
		{
			for(uint8_t logical=0; logical<CUBE_ALLOCATION; logical++)
			{
				uint8_t physical = toPhysical[logical];
				if(physical != CubeID::UNDEFINED)
				{
					CubeID cube(physical);
					motionMapper->updateMotion(logical);
				}
			}
		}


		void refreshNeighbors()
		{
			unsigned physical;
			while(attentionNeighbors.clearFirst(physical))
			{
				neighborhoods[physical] = Neighborhood(physical);
			}
		}

		bool neighborUtility(uint8_t cube, uint8_t &side, uint8_t &otherCube, uint8_t &otherSide)
		{
			uint8_t p1 = toPhysical[cube];
			uint8_t p2;

			if(p1 == CubeID::UNDEFINED) return false;

			if(otherCube != CubeID::UNDEFINED)
			{
				// the other cube is known, solve for side
				p2 = toPhysical[otherCube];
				if(p2 == CubeID::UNDEFINED) return false;
				side = neighborhoods[p1].sideOf(p2);
				if(side == Sifteo::NO_SIDE) return false;

			}
			else
			{
				// the side is known, solve for other cube
				CubeID c2 = neighborhoods[p1].cubeAt((Sifteo::Side)side);
				if(!c2.isDefined()) return false;
				p2 = c2;
				otherCube = toLogical[p2];
				if(otherCube == CubeID::UNDEFINED)
				{
					// p1 has a neighbor entry pointing to a disconnected cube.
					attentionNeighbors.mark(p1);
					return false;
				}
			}
			// get here, then p1:side -> p2 and  (in logical space) cube:side -> otherCube
			otherSide = neighborhoods[p2].sideOf(p1);
			if(otherSide == Sifteo::NO_SIDE)
			{
				// something's wrong, the neighbors don't agree.
				attentionNeighbors.mark(p1);
				attentionNeighbors.mark(p2);
				return false;
			}
			return true;
		}

		void refresh()
		{
			if(!attentionCubes.empty())
			{
				CubeSet current = CubeSet::connected();
				unsigned i;
				while(attentionCubes.clearFirst(i))
				{
					if(current.test(i))
					{
						// cube is connected
						if(allocate(i))
						{
							fullRefresh = true;
						}
					}
					else
					{
						// cube is not connected
						CubeID(i).detachVideoBuffer();
						if(deallocate(i))
						{
							fullRefresh = true;
						}
					}
				}
			}

			if(!attentionNeighbors.empty())
			{
				refreshNeighbors();
			}
			updateAllMotion();
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
			attentionNeighbors.mark(cubeID);
		}
		void onNeighborChange(unsigned id1, unsigned side1, unsigned id2, unsigned side2)
		{
			attentionNeighbors.mark(id1);
			attentionNeighbors.mark(id2);
		}

		void initialize()
		{
			Sifteo::Events::cubeRefresh.set(&EventHandler::onCubeRefresh, this);

			cubeMapping.initialize();
			attentionNeighbors = attentionCubes = CubeSet::connected();

			Sifteo::Events::cubeConnect.set(&EventHandler::onCubeChange, this);
			Sifteo::Events::cubeDisconnect.set(&EventHandler::onCubeChange, this);

			Sifteo::Events::neighborAdd.set(&EventHandler::onNeighborChange, this);
			Sifteo::Events::neighborRemove.set(&EventHandler::onNeighborChange, this);
		}
	}
	eventHandler;

	void initialize()
	{
		eventHandler.initialize();
	}

	void setLoadingScreen(LoadingScreen &p)
	{
		loadingScreen = &p;
	}

	void setMotionMapper(MotionMapper &p)
	{
		cubeMapping.refresh();
		cubeMapping.detachAllMotion();
		motionMapper = &p;
		cubeMapping.attachAllMotion();
	}
	void clearMotionMapper()
	{
		setMotionMapper(noMotion);
	}

	void setFrameRate(float fr)
	{
		frameRate = TimeTicker(fr);
		timeStep.next();				// reset the time stepper
	}

	void beginScene()
	{
		scenePointer = sceneBuffer;
		initialDraw = BitArray<SCENE_MAX_SIZE>(0,0);
	}

	void addElement(uint8_t type, uint8_t cube, uint8_t mode, uint8_t update, uint8_t autoupdate, void *object)
	{
		uint16_t sceneIndex = scenePointer - sceneBuffer;
		ASSERT(sceneIndex < SCENE_MAX_SIZE);

		scenePointer->type = type;
		if(cube & Scene::HIDE)
		{
			cube &= ~Scene::HIDE;
		}
		else
		{
			initialDraw.mark(sceneIndex);
		}
		scenePointer->cube = cube;
		elementModes[sceneIndex] = mode;
		scenePointer->update = update;
		scenePointer->autoupdate = autoupdate;
		scenePointer->object = object;

		scenePointer++;
	}

	void endScene()
	{
		sceneSize = scenePointer - sceneBuffer;
		fullRefresh = true;
	}

	int32_t doRedraw(Handler &handler)
	{
		ASSERT(loadingScreen != 0);

		cubeMapping.refresh();

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
			BitArray<CUBE_ALLOCATION> dirty(0,0);			// mark cubes that are drawn on, you can't mode switch until they paint
			BitArray<CUBE_ALLOCATION> attachPending(0,0);	// mark cubes that were drawn on in detached mode
			BitArray<SCENE_MAX_SIZE> todo = redraw;
			redraw.clear();

			unsigned i;
			while(todo.clearFirst(i))
			{
				Element &element = sceneBuffer[i];
				uint8_t cube = element.cube;
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
				uint8_t elementMode = elementModes[i];

				// if the mode is OK, just draw it
				if(currentMode == elementMode)
				{
					SCENELOG("SCENE: Draw element %d\n", i);
					START_TIMER;
					handler.drawElement(element, vid[cube]);
					END_TIMER;
					dirty.mark(cube);
				}
				// if the cube is dirty, just requeue this one for after the next paint event
				else if(dirty.test(cube))
				{
					redraw.mark(i);
				}
				else
				{
					// in the wrong mode, so need to do a mode switch
					// but first you need to check assets for this mode
					AssetConfiguration<ASSET_CAPACITY> *pAssets = handler.requestAssets(cube, elementMode);
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
							SCENELOG("SCENE: Opening a new loader\n");
							assetLoader.init();
						}
						SCENELOG("SCENE: Beginning download to cube %d\n", cube);
						assetLoader.start(*pAssets, CubeSet(physical));
						cubesLoading.mark(physical);
						currentMode = currentModes[cube] = NO_MODE;
						CubeID(physical).detachVideoBuffer();
						loadingScreen->init(cube, vid[cube]);
						vid[cube].attach(physical);
						loadingScreen->onAttach(cube, vid[cube]);
						redraw.mark(i);	// queue this item after at least one loader paint cycle
					}
					else
					{
						// ok to perform the mode switch, and may as well attach right now
						SCENELOG("SCENE: Mode switch of cube %d\n", cube);
						CubeID(physical).detachVideoBuffer();
						// some modes can be drawn detached (non-tile modes). You should defer these to save some radio.
						bool attachNow = handler.switchMode(cube, elementMode, vid[cube]);
						currentMode = currentModes[cube] = elementMode;
						if(attachNow)
						{
							vid[cube].attach(physical);
						}
						else
						{
							attachPending.mark(cube);
						}
						// on a mode switch, any active objects in this mode on this cube need to be redrawn ASAP
						// for the moment, loop through all items to find matches.
						for(unsigned j : initialDraw)
						{
							Element *resync = sceneBuffer + j;
							uint8_t resyncMode = elementModes[j];
							if((currentMode == resyncMode) && (resync->cube==cube))
							{
								todo.mark(j);
							}
						}
					}
				}
			}

			// attach any cubes that you queued up for later
			for(uint8_t cube : attachPending)
			{
				uint8_t physical = cubeMapping.physical(cube);
				vid[cube].attach(physical);
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
					SCENELOG("SCENE: Completed download to cube %d\n", i);
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
		// log a message if the frame step is too large, we want to see this even if scene logging is disabled
		if(fc > frameThreshold)
		{
			LOG("SCENE: Unexpected large frame step: %d\n", fc);
			fc = frameThreshold;							// clamp
		}
		int32_t exitCode = 0;

		for(uint16_t i = 0; (i<sceneSize); i++)
		{
			Element &element = sceneBuffer[i];
			if(element.update == Scene::FULL_UPDATE)
			{
				exitCode = handler.updateElement(element, fc);
				if(exitCode !=0) return exitCode;
			}
			else if(element.update != 0)
			{
				uint8_t ur = fc;
				while(ur >= element.update)
				{
					ur -= element.update;
					element.update = element.autoupdate;
					exitCode = handler.updateElement(element);
					if(exitCode !=0) return exitCode;
				}
				element.update -= ur;
			}
		}
		return 0;
	}

	int32_t execute(Handler &handler)
	{
		int32_t exitCode;
		while( (exitCode=doRedraw(handler)) == 0);
		return exitCode;
	}

	void close()
	{
		cubeMapping.refresh();
		cubeMapping.detachAllVideo();
		sceneSize = 0;
	}

	Element &getElement(uint16_t index)
	{
		ASSERT(index < sceneSize);
		return sceneBuffer[index];
	}

	void Element::repaint()
	{
		uint16_t index = this - sceneBuffer;
		ASSERT(index < sceneSize);
		redraw.mark(index);
	}

	void Element::setUpdate(uint8_t u)
	{
		update = u;
	}

	void Element::clearUpdate()
	{
		setUpdate(0);
	}
}
