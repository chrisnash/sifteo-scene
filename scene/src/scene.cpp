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
#define END_TIMER if(scenetime) {uint64_t t = SystemTime::now().uptimeNS()-timestamp; SCENELOG("SCENE: elapsed time %uus\n", (uint32_t)(t/1000));}

	// the items that need a redraw now
	BitArray<SCENE_MAX_SIZE> redraw;
	// the items that need a redraw in order to do a full render
	BitArray<SCENE_MAX_SIZE> initialDraw;

	bool resetEvent = false;

	// current cube modes
	uint8_t currentModes[CUBE_ALLOCATION];

	// the current scene size
	uint16_t sceneSize;
	// the scene buffer available to build scenes on the fly
	Element sceneBuffer[SCENE_MAX_SIZE];
	uint8_t elementModes[SCENE_MAX_SIZE];

	// video buffers
	VideoBuffer vid[CUBE_ALLOCATION];
	VideoBuffer *currentlyAttached[CUBE_ALLOCATION];

	// Asset loader
	AssetLoader assetLoader;
	BitArray<CUBE_ALLOCATION> cubesLoading(0,0);	// nothing set
	uint8_t syncMode = Scene::SYNC_DOWNLOAD;

	// The frame threshold. Log a warning if the update counter is too large.
	// The default value assumes updates are within 10Hz
	uint8_t frameThreshold = 0x06;

	void fastAttach(uint8_t physical, VideoBuffer &v)
	{
		if(currentlyAttached[physical] != &v)
		{
			if(currentlyAttached[physical])
			{
				// if attached anywhere else
				v.attach(CubeID(physical));
			}
			else
			{
				// we can do a low level attach without a finish
		        v.sys.cube = CubeID(physical);
		        _SYS_vbuf_init(v);
		        _SYS_setVideoBuffer(v, v);
			}
			currentlyAttached[physical] = &v;
		}
	}

	void fastDetach(uint8_t physical)
	{
		if(currentlyAttached[physical])
		{
			CubeID(physical).detachVideoBuffer();
			currentlyAttached[physical] = NULL;
		}
	}

	class DefaultLoadingScreen : public LoadingScreen
	{
	public:
		bool init(uint8_t cube, Sifteo::VideoBuffer &v, uint8_t part)
		{
			v.initMode(BG0_ROM);
			return true;	// flag, this is the last part
		}
		void onAttach(uint8_t cube, Sifteo::VideoBuffer &v, uint8_t part)
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

	BitArray<CUBE_ALLOCATION> attentionCubes(0,0);
	BitArray<CUBE_ALLOCATION> attentionNeighbors(0,0);

	Neighborhood neighborhoods[CUBE_ALLOCATION];

	TimeTicker frameRate(60.0);
	TimeStep timeStep;

	class CubeMapping
	{
		uint8_t toPhysical[CUBE_ALLOCATION];
		uint8_t toLogical[CUBE_ALLOCATION];
		BitArray<CUBE_ALLOCATION> connectionMap;
	public:
		void initialize()
		{
			for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
			{
				toPhysical[i] = CubeID::UNDEFINED;
				toLogical[i] = CubeID::UNDEFINED;
			}
			connectionMap.clear();
			Sifteo::bzero(currentlyAttached);
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
			connectionMap.mark(logical);
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
			connectionMap.clear(logical);

			// bubble down allocated cubes to fill any spaces
			// basically keep moving the highest assigned logical cube to the lowest unassigned logical cube
			uint8_t highest = CUBE_ALLOCATION - 1;
			while((highest>logical) && (toPhysical[highest] == CubeID::UNDEFINED)) highest--;
			if(highest > logical)
			{
				physical = toPhysical[highest];
				toPhysical[highest] = CubeID::UNDEFINED;
				connectionMap.clear(highest);

				// bind physical to logical, bind highest to nowhere
				toLogical[physical] = logical;
				toPhysical[logical] = physical;
				connectionMap.mark(logical);
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

		uint8_t getCubeCount()
		{
			return connectionMap.count();
		}

		BitArray<CUBE_ALLOCATION> &getConnectionMap()
		{
			return connectionMap;
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

		bool refreshState()
		{
			if(!attentionNeighbors.empty())
			{
				refreshNeighbors();
				return true;
			}
			return false;
		}

		void NOINLINE unhappyCubeEvents()
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
						SCENELOG("SCENE: new allocation of cube %d\n", i);
						resetEvent = true;
					}
				}
				else
				{
					// cube is not connected
					if(deallocate(i))
					{
						SCENELOG("SCENE: deallocation of cube %d\n", i);
						resetEvent = true;
					}
				}
			}
		}

		void NOINLINE unhappyResetEvent()
		{
			SCENELOG("SCENE: detaching all video\n");
			// detach all the video buffers. Yes, I'm serious
			START_TIMER;
			for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
			{
				fastDetach(i);
				CubeID(i).detachMotionBuffer();
			}
			END_TIMER;
		}

		bool pumpEvents()
		{
			if(!attentionCubes.empty())
			{
				unhappyCubeEvents();
			}
			if(resetEvent)
			{
				unhappyResetEvent();
			}
			return resetEvent;
		}

		void detachAllMotion()
		{
			for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
			{
				CubeID(i).detachMotionBuffer();
			}
		}
		void attachAllMotion(Handler &h)
		{
			for(uint8_t l : connectionMap)
			{
				uint8_t p = toPhysical[l];
				h.attachMotion(l, CubeID(p));
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
			// for the moment, just do a hard reset of the scene when this event arrives
			// we may get better granularity later.
			resetEvent = true;
		}
		void onCubeChange(unsigned cubeID)
		{
			attentionCubes.mark(cubeID);
			attentionNeighbors.mark(cubeID);
		}
		void onNeighborChange(unsigned id1, unsigned side1, unsigned id2, unsigned side2)
		{
			// protect against neighboring the base
			if(id1<CUBE_ALLOCATION) attentionNeighbors.mark(id1);
			if(id2<CUBE_ALLOCATION) attentionNeighbors.mark(id2);
		}

		void initialize()
		{
			Sifteo::Events::cubeRefresh.set(&EventHandler::onCubeRefresh, this);

			Sifteo::Events::cubeConnect.set(&EventHandler::onCubeChange, this);
			Sifteo::Events::cubeDisconnect.set(&EventHandler::onCubeChange, this);

			Sifteo::Events::neighborAdd.set(&EventHandler::onNeighborChange, this);
			Sifteo::Events::neighborRemove.set(&EventHandler::onNeighborChange, this);

			cubeMapping.initialize();
			attentionNeighbors.mark();
			attentionCubes.mark();
			cubeMapping.pumpEvents();
			resetEvent = false;
		}
	}
	eventHandler;

	void reset()
	{
		// hard cancel any in-progress loaders
		if(!cubesLoading.empty())
		{
			assetLoader.cancel();
			cubesLoading.clear();
		}
		// perform a full reset of the scene
		redraw = initialDraw;
		for(uint8_t i=0; i<CUBE_ALLOCATION; i++) currentModes[i] = NO_MODE;
		attentionNeighbors.mark();
		timeStep.next();
	}

	void initialize()
	{
		eventHandler.initialize();
	}

	void setLoadingScreen(LoadingScreen &p)
	{
		loadingScreen = &p;
	}

	void setFrameRate(float fr)
	{
		frameRate = TimeTicker(fr);
		timeStep.next();				// reset the time stepper
	}

	void beginScene()
	{
		sceneSize = 0;
		initialDraw = BitArray<SCENE_MAX_SIZE>(0,0);
		reset();
	}

	Element &createElement(uint8_t type, uint8_t cube, uint8_t mode, void *object)
	{
		return getElement(addElement(type, cube, mode, Scene::NO_UPDATE, Scene::NO_UPDATE, object));
	}

	uint16_t addElement(uint8_t type, uint8_t cube, uint8_t mode, uint8_t update, uint8_t autoupdate, void *object)
	{
		ASSERT(sceneSize < SCENE_MAX_SIZE);
		Element &el = sceneBuffer[sceneSize];

		el.type = type;
		if(cube & Scene::HIDE)
		{
			cube &= ~Scene::HIDE;
		}
		else
		{
			initialDraw.mark(sceneSize);
			redraw.mark(sceneSize);
		}
		el.cube = cube;
		elementModes[sceneSize] = mode;
		el.update = update;
		el.autoupdate = autoupdate;
		el.object = object;

		return sceneSize++;
	}

	void NOINLINE unhappyScene(Handler &handler)
	{
		Scene::reset();
		handler.cubeCount( cubeMapping.getCubeCount() );
		cubeMapping.attachAllMotion(handler);
		resetEvent = false;	// mark reset event as correctly handled
	}

	void NOINLINE unhappyAttach(const BitArray<CUBE_ALLOCATION> &attachPending)
	{
		for(uint8_t cube : attachPending)
		{
			uint8_t physical = cubeMapping.physical(cube);
			fastAttach(physical, vid[cube]);
		}
	}

	void NOINLINE unhappyLoader()
	{
		// if no sync, do every cube individually
		if(syncMode == Scene::SYNC_NONE)
		{
			for(uint8_t i : cubesLoading)
			{
				float progress = assetLoader.cubeProgress(i);
				uint8_t logical = cubeMapping.logical(i);
				loadingScreen->update(logical, progress, vid[logical]);
				if(assetLoader.isComplete(i))
				{
					SCENELOG("SCENE: Completed download to cube %d\n", i);
					fastDetach(i);
					cubesLoading.clear(i);
				}
			}
		}
		else
		{
			float progress = assetLoader.averageProgress();	// same progress on all cubes
			for(uint8_t i : cubesLoading)
			{
				uint8_t logical = cubeMapping.logical(i);
				loadingScreen->update(logical, progress, vid[logical]);
			}
			if(assetLoader.isComplete())
			{
				SCENELOG("SCENE: Completed download to all cubes\n");
				for(uint8_t i : cubesLoading)
				{
					fastDetach(i);
				}
				cubesLoading.clear();
			}
		}
		if(cubesLoading.empty())
		{
			assetLoader.finish();
			timeStep.next();			// reset the frame clock
		}
	}

	BitArray<CUBE_ALLOCATION> dirty;;			// mark cubes that are drawn on, you can't mode switch until they paint
	BitArray<CUBE_ALLOCATION> attachPending;	// mark cubes that were drawn on in detached mode
	BitArray<SCENE_MAX_SIZE> todo;

	void NOINLINE unhappyDraw(Handler &handler, unsigned i, bool drawBan)
	{
		Element &element = sceneBuffer[i];
		uint8_t cube = element.cube;
		uint8_t physical = cubeMapping.physical(cube);
		uint8_t currentMode = currentModes[cube];
		uint8_t elementMode = elementModes[i];

		if(dirty.test(cube))
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

				// allow for a multimodal loading screen
				bool loadingScreenReady = false;
				uint8_t loadingScreenPart = 0;
				while(!loadingScreenReady)
				{
					fastDetach(physical);
					loadingScreenReady = loadingScreen->init(cube, vid[cube], loadingScreenPart);
					fastAttach(physical, vid[cube]);
					loadingScreen->onAttach(cube, vid[cube], loadingScreenPart);
					if(!loadingScreenReady)
					{
						System::paintUnlimited();
						loadingScreenPart++;
					}
				}
				redraw.mark(i);	// queue this item after at least one loader paint cycle
			}
			else if(!drawBan)
			{
				// ok to perform the mode switch, and may as well attach right now
				SCENELOG("SCENE: Mode switch of cube %d\n", cube);
				SCENELOG("SCENE: Detach old\n");
				START_TIMER;
				fastDetach(physical);
				END_TIMER;
				// some modes can be drawn detached (non-tile modes). You should defer these to save some radio.
				SCENELOG("SCENE: Attach new\n");
				START_TIMER;
				bool attachNow = handler.switchMode(cube, elementMode, vid[cube]);
				currentMode = currentModes[cube] = elementMode;
				if(attachNow)
				{
					fastAttach(physical, vid[cube]);
				}
				else
				{
					attachPending.mark(cube);
				}
				END_TIMER;
				// on a mode switch, any active objects in this mode on this cube need to be redrawn ASAP
				// for the moment, loop through all items to find matches.
				SCENELOG("SCENE: Refresh redraw list\n");
				START_TIMER;
				for(unsigned j : initialDraw)
				{
					Element *resync = sceneBuffer + j;
					uint8_t resyncMode = elementModes[j];
					if((currentMode == resyncMode) && (resync->cube==cube))
					{
						todo.mark(j);
					}
				}
				END_TIMER;
			}
			else
			{
				// mode switch was requested but drawing was banned
				redraw.mark(i);
			}
		}
	}

	int32_t doRedraw(Handler &handler)
	{
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

		// for at least one paint cycle
		do
		{
			// if we are in full sync mode and a download is taking place, drawing is banned.
			bool drawBan = (syncMode == Scene::SYNC_FULL) && (!cubesLoading.empty());

			dirty.clear();
			attachPending.clear();
			todo = redraw;
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
					if(drawBan)
					{
						redraw.mark(i);	// try again later
					}
					else
					{
						SCENELOG("SCENE: Draw element %d\n", i);
						START_TIMER;
						handler.drawElement(element, vid[cube]);
						END_TIMER;
						dirty.mark(cube);
					}
				}
				// if the cube is dirty, just requeue this one for after the next paint event
				else
				{
					unhappyDraw(handler, i, drawBan);
				}
			}

			// attach any cubes that you queued up for later
			if(!attachPending.empty())
			{
				unhappyAttach(attachPending);
			}

			// yield a while (in indeterminate state)
			System::paint();
			if(cubeMapping.pumpEvents())
			{
				unhappyScene(handler);
			}

			// check for loading cubes that have finished
			if(!cubesLoading.empty())
			{
				unhappyLoader();
			}
		}
		while(!redraw.empty() || !cubesLoading.empty());

		handler.updateAllMotion(cubeMapping.getConnectionMap());
		if(cubeMapping.refreshState())
		{
			SCENELOG("SCENE: Refresh neighbors\n");
			handler.neighborAlert();							// sample motion and neighbor events
		}

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
			uint8_t ur = fc;
			while((ur > 0) && (element.update>0))
			{
				if(element.update == Scene::FULL_UPDATE)
				{
					SCENELOG("SCENE: full update, element %d\n", i);
					exitCode = handler.updateElement(element, ur);
					ur = 0;
					if(exitCode !=0) return exitCode;
				}
				else if(ur >= element.update)
				{
					ur -= element.update;
					element.update = element.autoupdate;
					SCENELOG("SCENE: firing update, element %d\n", i);
					exitCode = handler.updateElement(element);
					if(exitCode !=0) return exitCode;
				}
				else
				{
					element.update -= ur;
					ur = 0;
				}
			}
		}
		return 0;
	}

	int32_t execute(Handler &handler)
	{
		// make at least one neighbor callback
		attentionNeighbors.mark();
		// and make an immediate cube count callback
		handler.cubeCount( cubeMapping.getCubeCount() );
		// attach all motion
		cubeMapping.attachAllMotion(handler);

		int32_t exitCode;
		while( (exitCode=doRedraw(handler)) == 0);

		cubeMapping.detachAllMotion();

		return exitCode;
	}

	bool neighborAt(uint8_t cube, uint8_t side, uint8_t &otherCube, uint8_t &otherSide)
	{
		otherCube = CubeID::UNDEFINED;	// make the neighbor utility run in 'side is known' mode
		bool rv = cubeMapping.neighborUtility(cube, side, otherCube, otherSide);
		if(!rv)
		{
			otherCube = otherSide = CubeID::UNDEFINED;
		}
		return rv;
	}

	void setFrameThreshold(uint8_t ft)
	{
		frameThreshold = ft;
	}

	void setSyncMode(uint8_t sm)
	{
		syncMode = sm;
	}

	Element &getElement(uint16_t index)
	{
		ASSERT(index < sceneSize);
		return sceneBuffer[index];
	}

	Element &Element::repaint()
	{
		uint16_t index = this - sceneBuffer;
		ASSERT(index < sceneSize);
		redraw.mark(index);
		return *this;
	}

	Element &Element::setUpdate(uint8_t u)
	{
		update = u;
		return *this;
	}

	Element &Element::clearUpdate()
	{
		return setUpdate(Scene::NO_UPDATE);
	}

	Element &Element::fullUpdate()
	{
		return setUpdate(Scene::FULL_UPDATE);
	}

	Element &Element::setAutoupdate(uint8_t au)
	{
		autoupdate = au;
		return *this;
	}

	Element &Element::show()
	{
		uint16_t index = this - sceneBuffer;
		ASSERT(index < sceneSize);
		initialDraw.mark(index);
		redraw.mark(index);
		return *this;
	}

	Element &Element::hide()
	{
		uint16_t index = this - sceneBuffer;
		ASSERT(index < sceneSize);
		initialDraw.clear(index);
		redraw.clear(index);
		return *this;
	}
}
