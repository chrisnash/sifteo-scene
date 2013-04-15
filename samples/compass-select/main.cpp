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

class SimpleModeHandler : public Scene::ModeHandler
{
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf;
public:
	SimpleModeHandler()
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
};

class Debounce
{
	CubeID id;
	bool debounce;
public:
	void setCube(CubeID c)
	{
		id = c;
		debounce = (id !=CubeID::UNDEFINED) ? id.isTouching() : false;
	}

	bool isTouching()
	{
		if(id == CubeID::UNDEFINED) return false;

		bool now = id.isTouching();
		bool then = debounce;
		debounce = now;
		return (now && !then);
	}
};
Debounce sullyTouch[3];

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

	void updateElement(Scene::Element *el)
	{
		cycleIndex++;
		if(cycleIndex == arraysize(colorCycle))
		{
			cycleIndex = 0;
		}
		currentColor = colorCycle[cycleIndex];
		el->repaint();
	}
};

class SimpleElementHandler : public Scene::ElementHandler
{
public:
	void drawElement(Scene::Element *el, Sifteo::VideoBuffer &v)
	{
		switch(el->type)
		{
		case 0:
			v.bg0.image(vec(0,0), Sully);
			break;
		case 1:
			// the text type
			v.colormap[0] = RGB565::fromRGB(0x000080);
			v.colormap[1] = RGB565::fromRGB(0xFFFFFF);		// white on dark blue
			v.fb128.fill(vec(0,0), vec(128,16), 0);			// fill it
			Font::drawCentered(v, vec(0,0), vec(128, 16), (const char *)(el->object));
			break;
		default:
			// the color changer
			ColorChanger *cc = (ColorChanger*)(el->object);
			v.colormap[1] = cc->currentColor;
			break;
		}
	}
	int32_t updateElement(Scene::Element *el, uint8_t fc=0)
	{
		switch(el->type)
		{
		case 0:		// sully
			return sullyTouch[el->cube].isTouching() ? (el->cube+1) : 0;
		default:	// color changer
			ColorChanger *cc = (ColorChanger*)(el->object);
			cc->updateElement(el);
			return 0;
		}
	}
};

class SimpleLoadingScreen : public Scene::LoadingScreen
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
};

class SimpleMotionMapper : public Scene::MotionMapper
{
public:
	void attachMotion(uint8_t cube, CubeID param)
	{
		if(cube<3)
		{
			sullyTouch[cube].setCube(param);
		}
	}
	void detachMotion(uint8_t cube, CubeID param)
	{
		if(cube<3)
		{
			sullyTouch[cube].setCube(CubeID::UNDEFINED);
		}
	}
};

void main()
{
	// initialize scene
	Scene::initialize();
	SimpleModeHandler smh;
	Scene::setModeHandler(&smh);
	SimpleElementHandler seh;
	Scene::setElementHandler(&seh);
	SimpleLoadingScreen sls;
	Scene::setLoadingScreen(&sls);
	SimpleMotionMapper smm;
	Scene::setMotionMapper(&smm);

	// use the scene builder API
	Scene::beginScene();

	const char *text_messages[] = {"North","West", "South", "East"};
	ColorChanger changers[4];

	// build the sully elements
	Scene::addElement(0, 0,0, Scene::FULL_UPDATE);													// type 0 is the sully item
	for(int i=0; i<4; i++)
	{
		Scene::addElement(1, 0,i+1, Scene::NO_UPDATE, Scene::NO_UPDATE, (void *)text_messages[i]);	// type 1 cube 0 modes 1-4
		Scene::addElement(2, 0,i+1, i*2+1, 8, (void *)(changers + i));								// start color changers at different timer offsets for the moment
	}
	Scene::endScene();			// complete the scene build

	while(1)
	{
		int32_t code = Scene::execute();			// this call never returns because we have no update methods.
		LOG("A cube was touched: %d\n", code);
	}
}
