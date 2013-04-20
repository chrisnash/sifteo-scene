#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("scene-pong")
    .package("com.github.sifteo-scene.scene-pong", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

float ballRadius = 2.0;
float batWidth = 4.0;
float batLength = 16.0;

uint16_t ballElement;

class BallObj
{
public:
	float x;
	float y;
	uint8_t angle;	// multiple of 5 degrees
};

class Bat
{
public:
	float y;
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
		return (mode==0) ? &assetconf : NULL;
	}

	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v)
	{
		if(mode==0)
		{
			v.initMode(BG0_SPR_BG1);
			// create the BG1 mask
			v.bg1.fillMask(vec((cube==0)?12:1,1), vec(3,5));
			return true;	// tile based
		}
		else
		{
			v.initMode(FB128, 48, 32);	// right across the middle
			v.colormap[0] = RGB565::fromRGB(0x000080);
			v.colormap[1] = RGB565::fromRGB(0xFFFFFF);
			return false;
		}
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		switch(el.type)
		{
		case 0:	// board
			v.bg0.image(vec(0,0), Board, el.cube);		// frame number is cube
			break;
		case 1: // ball
			break;
		case 2: // bat
			{
				Bat *bat = (Bat*)el.object;
				int y = round(bat->y - batLength/2);
				int x = (el.cube==0)?0:120;
				v.sprites[1].setImage(Bats, el.cube);
				v.sprites[1].move(vec(x,y));
			}
			break;
		case 3: // score
			v.bg1.image(vec((el.cube==0)?12:1,1), Digits, el.data[0]);
			break;
		case 4: // game over message
			break;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		switch(el.type)
		{
		case 1: // ball
			break;
		case 2: // bat
			break;
		case 4:	// game over
			break;
		}
		return 0;	// just say it's never returning
	}

	void cubeCount(uint8_t cubes) {}

	void neighborAlert() {}
};

void main()
{
	// initialize scene
	Scene::initialize();

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	// board
	Scene::addElement(0, 	0,0);
	Scene::addElement(0, 	1,0);

	// ball
	BallObj ballobj;
	ballobj.x = 64.0;
	ballobj.y = 64.0;
	ballobj.angle = 1;	// 5 degrees south
	ballElement = Scene::addElement(1,	0,0, Scene::FULL_UPDATE, Scene::NO_UPDATE, &ballobj);
	Scene::addElement(1,	1,0);

	// bat
	Bat bat1; bat1.y = 64.0;
	Bat bat2; bat2.y = 64.0;
	Scene::addElement(2,	0,0, Scene::FULL_UPDATE, Scene::NO_UPDATE, &bat1);
	Scene::addElement(2,	1,0, Scene::FULL_UPDATE, Scene::NO_UPDATE, &bat2);

	// score
	Scene::addElement(3,	0,0);
	Scene::addElement(3,	1,0);

	// game over
	Scene::addElement(4,	0|Scene::HIDE,1);
	Scene::addElement(4,	1|Scene::HIDE,1);

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("execute returned: %d\n", code);
	}
}
