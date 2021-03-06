#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("simple-image")
    .package("com.github.sifteo-scene.simple-image", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

// cache the sully image in uncompressed tile buffer RAM. This is overkill for this demo,
// but does illustrate how it can be done.
TileBuffer<16,16> tileBuffers[CUBE_ALLOCATION];
// we also cache tile group base addresses, so we only have to redraw the tile buffer when absolutely needed.
uint16_t baseAddresses[CUBE_ALLOCATION];

class FancyLoadingScreen : public Scene::LoadingScreen
{
	virtual bool init(uint8_t cube, Sifteo::VideoBuffer &v, uint8_t part)
	{
		if(part==0)
		{
			v.initMode(FB32);
			RGB565 cyan = RGB565::fromRGB(0x00FFFF);
			RGB565 green = RGB565::fromRGB(0x00FF00);
			RGB565 black = RGB565::fromRGB(0x000000);

			// 0-5 6 7-12 (13 colors)
			// draw a 12 line fade to black from cyan, 8 lines of black, 12 line fade to green
			for(int i=0; i<6; i++)
			{
				int li = (0x00FF * i)/6;
				v.colormap[i] = cyan.lerp(black, li);
				v.colormap[12-i] = green.lerp(black, li);

				v.fb32.fill(vec(0,2*i),vec(32,2), i);
				v.fb32.fill(vec(0,30-2*i),vec(32,2), 12-i);
			}
			v.colormap[6] = black;
			v.fb32.fill(vec(0,12), vec(32,8), 6);

			return false;
		}
		else
		{
			v.initMode(FB128, 52, 24);	// use a 24 line frame buffer (the 3 black lines in either half of FB32)
			v.colormap[0] = RGB565::fromRGB(0x000000);	// on black
			v.colormap[1] = RGB565::fromRGB(0xFFFFFF);	// white
			Font::drawCentered(v, vec(0,0), vec(128,12), "loading...");
			// draw a progress bar outline on FB128, lines 14 to 21 inclusive
			v.fb128.span( vec(2,14), 124, 1);			// top line
			v.fb128.plot( vec(1,15), 1); v.fb128.plot( vec(126,15), 1);	// corners
			v.fb128.fill( vec(0,16), vec(1,4), 1); v.fb128.fill( vec(127,16), vec(1,4), 1); // endcaps
			v.fb128.plot( vec(1,20), 1); v.fb128.plot( vec(126,20), 1);	// corners
			v.fb128.span( vec(2,21), 124, 1);			// bottom line

			return true;
		}
	}

	virtual void onAttach(uint8_t cube, Sifteo::VideoBuffer &v, uint8_t part)
	{
		// both parts are framebuffered, so nothing to do after attach
	}

	virtual void update(uint8_t cube, float progress, Sifteo::VideoBuffer &v)
	{
		int pg = 126*progress;
		v.fb128.fill( vec(1,15), vec(pg,6), 1);
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

	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v, CubeID param)
	{
		switch(mode)
		{
		case 0:
			{
				v.initMode(BG0);
				// init the tile buffer and draw sully into RAM. Note we use a tile buffer to illustrate how the CubeID can be used to correctly tile relocate.
				tileBuffers[cube].setCube(param);	// connect to the cube
				uint16_t cba = Tiles.baseAddress(param);	// find out where the tiles group is on this cube
				if(baseAddresses[cube] != cba)
				{
					// redraw the tile buffer to relocate the indices
					baseAddresses[cube] = cba;
					tileBuffers[cube].image(vec(0,0), Sully);
				}
			}
			return true;					// attach immediately
		case 1:
			v.initMode(FB128, 112, 16);		// just the bottom 16 rows of the display
			return false;					// attach as late as you can get away with
		}
		return false;
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		switch(el.type)
		{
		case 0:
			// draw from the tile buffer
			v.bg0.image(vec(0,0), tileBuffers[el.cube]);
			break;
		case 1:
			v.colormap[0] = RGB565::fromRGB(0x000080);
			v.colormap[1] = RGB565::fromRGB(0xFFFFFF);		// white on dark blue
			v.fb128.fill(vec(0,0), vec(128,16), 0);			// fill it
			Font::drawCentered(v, vec(0,0), vec(128, 16), (const char *)el.object);
			break;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		// maybe someone touched a sully
		return touchEvents[el.cube] ? (el.cube+1) : 0;
	}

	void cubeCount(uint8_t cubes)
	{
		LOG("Cube count is now %d\n", (int)cubes);
	}

	void neighborAlert()
	{
		LOG("Neighbor state has changed\n");
	}

	void attachMotion(uint8_t cube, CubeID param)
	{
		_sullyTouch[cube].setCube(param);
	}

	void updateAllMotion(const BitArray<CUBE_ALLOCATION> &map)
	{
		// correctly suppress events from cubes no longer connected
		for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
		{
			touchEvents[i] = (map.test(i)) ? _sullyTouch[i].isTouching() : false;
		}
	}
};

void main()
{
	// mark the tile buffer base addresses as unknown
	for(int i=0;i<CUBE_ALLOCATION; i++) baseAddresses[i] = 0xFFFF;

	// initialize scene
	Scene::initialize();
	FancyLoadingScreen fls;
	Scene::setLoadingScreen(fls);

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	const char *text_messages[] = {"Cube 1", "Cube 2", "Cube 3", "Cube 4", "Cube 5", "Cube 6"};

	// build sully elements on all the cubes, even the ones you can't see
	for(int i=0; i<6; i++)
	{
		Scene::addElement(0, i,0, Scene::FULL_UPDATE);												// type 0 is the sully item
		Scene::addElement(1, i,1, Scene::NO_UPDATE, Scene::NO_UPDATE, (void *)text_messages[i]);	// type 1 is the text area
	}

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("A cube was touched: %d\n", code);
	}
}
