#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

using namespace Sifteo;

static Metadata M = Metadata()
    .title("neighbor-update")
    .package("com.github.sifteo-scene.simple-image", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

#define IS_NEIGHBORED data[0]
#define NEIGHBOR_CUBE data[1]
#define NEIGHBOR_SIDE data[2]

int xpos[] = {7, 7, 0, 7,  14};
int ypos[] = {7, 0, 7, 14, 7};
const char *tag[] = {"C", "T", "L", "B", "R"};

uint16_t neighborBase;

class SimpleHandler : public Scene::Handler
{
public:
	Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode)
	{
		return NULL;	// no assets for this guy
	}

	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v)
	{
		v.initMode(BG0_ROM);
		return false;
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		String<3> msg;

		switch(el.type)
		{
		case 0:
			msg << tag[0];
			msg << (el.cube+1);
			break;
		default:	// types 1 to 4 are neighbors
			if(!el.IS_NEIGHBORED)
			{
				msg << "--";
			}
			else
			{
				msg << tag[el.NEIGHBOR_SIDE+1];
				msg << el.NEIGHBOR_CUBE+1;
			}
			break;
		}
		v.bg0rom.text(vec(xpos[el.type], ypos[el.type]), msg);
	}

	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		// the only elements that can update are the neighbor indicators
		uint8_t otherCube = 0;
		uint8_t otherSide = 0;
		//LOG("entering neighbor at\n");
		uint8_t flag = Scene::neighborAt(el.cube, el.type-1, otherCube, otherSide) ? 1 : 0;
		//LOG("exiting neighbor at\n");
		if((flag != el.IS_NEIGHBORED) || (otherCube != el.NEIGHBOR_CUBE) || (otherSide != el.NEIGHBOR_SIDE))
		{
			el.IS_NEIGHBORED = flag;
			el.NEIGHBOR_CUBE = otherCube;
			el.NEIGHBOR_SIDE = otherSide;
			el.repaint();
		}
		return 0;
	}

	void cubeCount(uint8_t cubes)
	{
	}

	void neighborAlert()
	{
		LOG("neighbor alert\n");
		for(int i=0; i<12; i++)
		{
			Scene::getElement(neighborBase + i).setUpdate();
		}
		LOG("neighbor alert handled\n");
	}
};

namespace Scene
{
extern bool scenelog;
}

void main()
{
	Scene::scenelog = true;

	// initialize scene
	Scene::initialize();
	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	// build the background elements
	for(int i=0; i<3; i++)
		Scene::addElement(0, i,0, Scene::NO_UPDATE);

	// build the neighbor elements
	neighborBase = 0;
	for(int i=0;i<3;i++)
	{
		for(int j=1; j<=4; j++)
		{
			uint16_t element = Scene::addElement(j, i,0, Scene::NO_UPDATE);
			if(neighborBase==0) neighborBase = element;
		}
	}

	Scene::endScene();			// complete the scene build

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("An event was returned: %d\n", code);
	}
}
