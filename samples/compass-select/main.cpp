#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("compass-select")
    .package("com.github.sifteo-scene.compass-select", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

// current UI state
int currentlySelected = 0;

// current motion parameters updated by the motion updater
int selected = 0;
bool touching = false;

RGB565 colorCycle[] = {
		RGB565::fromRGB(0xFF0000),	// red
		RGB565::fromRGB(0xFF00FF),	// magenta
		RGB565::fromRGB(0x00FF00),	// green
		RGB565::fromRGB(0x00FFFF),	// cyan
		RGB565::fromRGB(0xFFFF00),	// yellow
		RGB565::fromRGB(0xFFFFFF)	// white
};

class ColorChanger
{
public:
	RGB565 currentColor = RGB565::fromRGB(0xC0C0C0);	// a boring gray
	int cycleIndex = 0;									// when animating

	void updateElement(Scene::Element &el)
	{
		cycleIndex++;
		if(cycleIndex == arraysize(colorCycle))
		{
			cycleIndex = 0;
		}
		currentColor = colorCycle[cycleIndex];
		el.repaint();
	}
};

class SimpleHandler : public Scene::Handler
{
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf;
public:
	SimpleHandler()
	{
		assetconf.clear();
		assetconf.append(slot_1, Tiles);
	}
	Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode)
	{
		return (mode==0)? &assetconf : 0;
	}
	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v)
	{
		switch(mode)
		{
		case 0:
			v.initMode(BG0);
			return true;					// attach immediately
		default:
			v.initMode(FB128, 0, 16);		// just the top 16 rows of the display
			v.setOrientation((Sifteo::Side)(mode-1));		// modes 1 2 3 4 are T,L,B,R
			return false;					// attach as late as you can get away with
		}
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		switch(el.type)
		{
		case 0:
			v.bg0.image(vec(0,0), Sully);
			break;
		case 1:
			// the text type
			v.colormap[0] = RGB565::fromRGB(0x000080);
			v.fb128.fill(vec(0,0), vec(128,16), 0);			// fill it
			Font::drawCentered(v, vec(0,0), vec(128, 16), (const char *)el.object);
			break;
		default:
			// the color changer
			ColorChanger *cc = (ColorChanger*)el.object;
			v.colormap[1] = cc->currentColor;
			break;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		switch(el.type)
		{
		case 0:		// sully
			{		// introduce scope
				// if anything is selected and the selection has changed...
				if(selected && (selected != currentlySelected))
				{
					if(currentlySelected)
					{
						Scene::Element &old = Scene::getElement(currentlySelected*2);	// the color cycler for this guy
						ColorChanger *cc = (ColorChanger *)old.object;
						cc->currentColor = RGB565::fromRGB(0xC0C0C0);					// make him gray
						old.clearUpdate();												// and stop him updating
						old.repaint();													// and make sure he gets repainted
					}
					currentlySelected = selected;
					Scene::Element &now = Scene::getElement(currentlySelected*2);
					ColorChanger *cc = (ColorChanger *)now.object;
					now.setUpdate();													// and start him updating, he'll repaint when he does
				}
			}
			return (currentlySelected && touching) ? currentlySelected : 0;	// exit if something selected and you touched it
		default:	// color changer
			ColorChanger *cc = (ColorChanger*)el.object;
			cc->updateElement(el);
			return 0;
		}
	}

	void cubeCount(uint8_t cubes) {}
	void neighborAlert() {}
};

class SimpleMotionMapper : public Scene::MotionMapper
{
	CubeID id;
	bool debounce;
	TiltShakeRecognizer tsr;
public:
	bool isTouching()
	{
		bool now = id.isTouching();
		bool then = debounce;
		debounce = now;
		return (now && !then);
	}

	void attachMotion(uint8_t cube, CubeID param)
	{
		if(cube==0)
		{
			id = param;
			debounce = param.isTouching();
			tsr.attach(param);
		}
	}
	void updateAllMotion(const BitArray<CUBE_ALLOCATION> &map)
	{
		if(map.test(0))
		{
			tsr.update();
			Byte3 &tilt = tsr.tilt;
			if(tilt.y) selected = tilt.y+2; else if(tilt.x) selected = tilt.x+3;
			touching = isTouching();
		}
	}
}
smm;

void main()
{
	// initialize scene
	Scene::initialize();
	Scene::setMotionMapper(smm);	// not on the stack

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	const char *text_messages[] = {"North","West", "South", "East"};
	ColorChanger changers[4];

	// build the sully elements
	Scene::addElement(0, 0,0, Scene::FULL_UPDATE);													// type 0 is the sully item
	for(int i=0; i<4; i++)
	{
		Scene::addElement(1, 0,i+1, Scene::NO_UPDATE, Scene::NO_UPDATE, (void *)text_messages[i]);	// type 1 cube 0 modes 1-4
		Scene::addElement(2, 0,i+1, 0, 8, (void *)(changers + i));									// color changers are not running initially
	}

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("A selection was made: %d\n", code);
	}
}
