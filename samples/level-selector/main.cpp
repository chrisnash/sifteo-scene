#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("level-selector")
    .package("com.github.sifteo-scene.level-selector", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();


class TextField
{
public:
	String<64> text;
};

class LevelSelector
{
};

class Debounce
{
	CubeID id;
	bool debounce;
public:
	void setCube(CubeID c)
	{
		id = c;
		debounce = id.isTouching();
	}

	bool isTouching()
	{
		bool now = id.isTouching();
		bool then = debounce;
		debounce = now;
		return (now && !then);
	}
};
Debounce _sullyTouch[CUBE_ALLOCATION];
bool touchEvents[CUBE_ALLOCATION];

TiltShakeRecognizer tsr[2];
int8_t tiltEvents[2];

const Sifteo::AssetImage *characters[] =
{
		&None, &Sully1, &Sully2, &Sully3, &Sully4
};

class SimpleHandler : public Scene::Handler
{
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf1;
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf2;
public:
	SimpleHandler()
	{
		assetconf1.clear();
		assetconf1.append(slot_1, Scroller);

		assetconf2.clear();
		assetconf2.append(slot_2, Character);
	}

	Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode)
	{
		return (mode==0) ? ((cube!=2) ? &assetconf1 : &assetconf2) : 0;
	}

	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v, CubeID param)
	{
		switch(mode)
		{
		case 0:
			if(cube != 2)
			{
				v.initMode(BG0_SPR_BG1, 16, 96);
			}
			else
			{
				v.initMode(BG0_SPR_BG1, 0, 96);			// 96 pixels at screen top
				v.bg1.fillMask(vec(4,2), vec(8,8) );	// the character right in the middle
			}
			return true;					// attach immediately
		case 1:
			// 16 pixels at top
			v.initMode(FB128, 0, 16);
			break;
		case 2:
			// 16 pixels at bottom
			v.initMode(FB128, 112, 16);
			break;
		case 3:
			// 32 pixels at bottom
			v.initMode(FB128, 96, 32);
			break;
		}
		v.colormap[0] = RGB565::fromRGB(0x000080);
		v.colormap[1] = RGB565::fromRGB(0xFFFFFF);
		return false;
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		switch(el.type)
		{
		case 0:
			// the scroll register
			break;
		case 1:
			// text in a 16-pixel area
			Font::drawCentered( v, vec(0,0), vec(128,16), el.obj<TextField>().text);
			break;
		case 2:
			// text in a 32-pixel area
			Font::drawCentered( v, vec(0,0), vec(128,32), el.obj<TextField>().text);
			break;
		case 3:
			// a vertical scrolling level selector, takes some work
			break;
		case 4:
			// a sprite overlay, type, x, y, hidden
			if(el.data[3])
			{
				v.sprites[el.data[0]].hide();
			}
			else
			{
				v.sprites[el.data[0]].setImage( el.data[0] ? Down : Up);
				v.sprites[el.data[0]].move( el.data[1], el.data[2]);
			}
			break;
		case 5:
			// background of the character area
			v.bg0.image(vec(0,0), Background);
			break;
		case 6:
			// the character
			v.bg1.image(vec(4,2), *characters[el.d8]);
			break;
		case 7:
			// the character's scroll registers
			break;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		switch(el.type)
		{
		case 0:
			// the scroll register
			break;
		case 1:
			// text in a 16-pixel area
			break;
		case 2:
			// text in a 32-pixel area
			break;
		case 3:
			// a vertical scrolling level selector, takes some work
			break;
		case 4:
			// a sprite overlay, data is type, x, y, hidden
			break;
		case 5:
			// background of the character area
			break;
		case 6:
			// the character
			break;
		case 7:
			// the character's scroll registers
			break;
		}
		return 0;
	}

	void cubeCount(uint8_t cubes) {}
	void neighborAlert() {}

	void attachMotion(uint8_t cube, CubeID param)
	{
		_sullyTouch[cube].setCube(param);
		if(cube<2)
		{
			tsr[cube].attach(param);
		}
	}

	void updateAllMotion(const BitArray<CUBE_ALLOCATION> &map)
	{
		// correctly suppress events from cubes no longer connected
		for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
		{
			touchEvents[i] = (map.test(i)) ? _sullyTouch[i].isTouching() : false;
			if(i<2)
			{
				if(map.test(i))
				{
					tsr[i].update();
					tiltEvents[i] = tsr[i].tilt.y;
				}
				else tiltEvents[i] = 0;
			}
		}
	}
};

void setupSprite(Scene::Element &el, uint8_t type, uint8_t x, uint8_t y, uint8_t hidden)
{
	el.data[0] = type;
	el.data[1] = x;
	el.data[2] = y;
	el.data[3] = hidden;
}

void main()
{
	// initialize scene
	Scene::initialize();

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	// entities
	TextField t1_top; 		t1_top.text << "Chapter 1";
	TextField t1_bottom;	t1_bottom.text << "Completed 0/9";
	TextField t2_top;		t2_top.text << "Level 1";
	TextField t2_bottom;	t2_bottom.text << "Best time 00:59.99";
	TextField t3;			t3.text << "Tilt and tap to select";

	// mode 0 body mode 1 16 top mode 2 16 bottom mode 3 32 bottom
	// put mode 0 entries first to do the asset download up front
	// 0 scroll register
	Scene::createElement(0);
	// 3 level selector
	Scene::createElement(3, 0);
	Scene::createElement(3, 1);

	// 4 sprite
	setupSprite(Scene::createElement(4, 0), 0, 56, 0, 0);
	setupSprite(Scene::createElement(4, 0), 1, 56, 80, 0);
	setupSprite(Scene::createElement(4, 1), 0, 56, 0, 0);
	setupSprite(Scene::createElement(4, 1), 1, 56, 80, 0);

	// 1 16 px text fields
	Scene::createElement(1, 0,1, &t1_top);
	Scene::createElement(1, 0,2, &t1_bottom);
	Scene::createElement(1, 1,1, &t2_top);
	Scene::createElement(1, 1,2, &t2_bottom);

	// 2 32 px text fields
	Scene::createElement(2, 2,3, &t3);

	// 5 character background
	Scene::createElement(5, 2);
	// 6 character
	Scene::createElement(6, 2).d8 = 1;
	// 7 character scroll registers
	Scene::createElement(7, 2);

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("Selection %04x\n", code);
	}
}
