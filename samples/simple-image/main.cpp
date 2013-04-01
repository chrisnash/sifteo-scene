#include <sifteo.h>
#include <scene.h>

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
		return &assetconf;
	}
	void switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v)
	{
		v.initMode(BG0);
	}
};

class SimpleElementHandler : public Scene::ElementHandler
{
public:
	void drawElement(Scene::Element *el, Sifteo::VideoBuffer &v)
	{
		v.bg0.image(vec(0,0), Sully);
	}
	int32_t updateElement(Scene::Element *el)
	{
		return 0;
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

	// use the scene builder API
	Scene::Element *p = Scene::beginScene();
	LOG("Initial scene pointer %p\n", p);

	// build three sully elements
	for(int i=0; i<3; i++)
	{
		bzero(*p);					// type 0 update 0 mode 0 cube 0
		p->cube = i;
		p->mode |= Scene::ATTACHED;	// can't draw tiles unless the cube is attached
		p++;						// next element
	}

	Scene::endScene(p);			// complete the scene build

	Scene::execute();			// this call never returns because we have no update methods.
}
