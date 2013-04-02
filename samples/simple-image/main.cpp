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
	void switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v)
	{
		switch(mode)
		{
		case 0:
			v.initMode(BG0);
			break;
		case 1:
			v.initMode(FB128, 112, 16);		// just the bottom 16 rows of the display
			break;
		}
	}
};

CubeID sullyTouch[3];

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
			v.colormap[0] = RGB565::fromRGB(0x000080);
			v.colormap[1] = RGB565::fromRGB(0xFFFFFF);		// white on dark blue
			v.fb128.fill(vec(0,0), vec(128,16), 0);			// fill it
			Font::drawCentered(v, vec(0,0), vec(128, 16), (const char *)(el->object));
			break;
		}
	}
	int32_t updateElement(Scene::Element *el, uint8_t fc=0)
	{
		// maybe someone touched a sully
		return sullyTouch[el->cube].isTouching() ? (el->cube+1) : 0;
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

	}
	void update(uint8_t cube, float progress, Sifteo::VideoBuffer &v)
	{

	}
};

class SimpleMotionMapper : public Scene::MotionMapper
{
public:
	void attachMotion(uint8_t cube, CubeID param)
	{
		if(cube<3)
		{
			sullyTouch[cube] = param;
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
	Scene::Element *p = Scene::beginScene();
	LOG("Initial scene pointer %p\n", p);

	const char *text_messages[] = {"Cube 1", "Cube 2", "Cube 3"};

	// build three sully elements
	for(int i=0; i<3; i++)
	{
		bzero(*p);					// type 0 update 0 mode 0 cube 0
		p->mode |= Scene::ATTACHED;	// can't draw tiles unless the cube is attached
		p->cube = i;
		p->update = Scene::FULL_UPDATE;	// update sully every pass
		p++;						// next element

		bzero(*p);
		p->type = 1;				// type 1 is text
		p->mode = 1;				// mode 1 is text at the bottom
		p->cube = i;
		p->object = (void *)text_messages[i];
		p++;
	}

	Scene::endScene(p);			// complete the scene build

	while(1)
	{
		int32_t code = Scene::execute();			// this call never returns because we have no update methods.
		LOG("A cube was touched: %d\n", code);
	}
}
