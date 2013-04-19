#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("affine-rotation")
    .package("com.github.sifteo-scene.affine-rotation", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

class SimpleHandler : public Scene::Handler
{
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf;
	uint8_t sync = 0;
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
		// only one mode in this demo
		v.initMode(BG2);
		if(cube==1)
		{
			Scene::getElement(3).data[0] = 0;	// reset the orientation tracker
		}
		return true;					// attach immediately
	}

	void setAffine(Sifteo::VideoBuffer &v, uint16_t angle)
	{
		float f = (M_TAU*angle)/65536.0f;
		AffineMatrix m = AffineMatrix::identity();

		m.translate(64,64);			// move center to origin
		m.rotate(f);				// do the rotation
		m.translate(-64,-64);		// move it back

		v.bg2.setMatrix( m );
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		Scene::Element &e0 = Scene::getElement(0);
		uint16_t *pa = (uint16_t*)&e0.object;

		switch(el.type)
		{
		case 0:
			v.bg2.image(vec(0,0), Sully);
			break;
		case 1:
			// affine rotation, simple (bad) method.
			setAffine(v, *pa);
			break;
		case 2:
			// affine rotation, better method with coordinate repositioning, essentially gets the orientation from the multiple of 0x4000
			// note the very real possibility of a glitch here if the orientation change and the matrix get downloaded separately and the
			// cube begins painting with one downloaded and not the other. We do a cheesy hack here and just call System::finish(), but
			// really you should be locking memory if you're doing this kind of thing.
			uint16_t scratch = *pa + 0x2000;
			// use el.data[0] to determine if the orientation needs to be changed.
			uint8_t newOrientation = scratch>>14;
			if(newOrientation != el.data[0])
			{
				el.data[0] = newOrientation;
				System::finish();	// how bad is this? Possibly very.
				v.setOrientation((Sifteo::Side)newOrientation);
			}
			setAffine(v, (scratch & 0x3FFF) - 0x2000);
			break;
		}
		// we keep track of whether everything has been rendered here. We don't start the animation until both screens have rendered the rotation at least once.
		sync |= el.type;
		if(sync==0x03)
		{
			//LOG("Animation starting, current angle %d\n", *pa);
			sync = 0xFF;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		// wait until both screens have started their render cycle before we update
		if(sync==0xFF)
		{
			// the only element to update is the rotation. We treat el.object as a 16-bit unsigned for this
			uint16_t *pa = (uint16_t*)&el.object;				// use the object field as a uint16.
			(*pa) += fc*0x0100;									// full rotation takes 256 frames

			// tell items 2 and 3 (the two rotations) to repaint
			Scene::getElement(2).repaint();
			Scene::getElement(3).repaint();
		}
		return 0;
	}

	void cubeCount(uint8_t cubes) {}
	void neighborAlert() {}
};

namespace Scene
{
	extern uint8_t frameThreshold;
	extern bool scenelog;
	extern bool scenetime;
};

void main()
{
	// set a frame threshold to log if ever the update value drops too low.
	// Note that the screen refresh rate isn't that great when running BG2.
	Scene::frameThreshold = 0x08;
	Scene::scenelog = true;
	Scene::scenetime = true;

	// initialize scene
	Scene::initialize();

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	Scene::addElement(0, 0,0, Scene::FULL_UPDATE);			// add a sully to cube 0 that updates
	Scene::addElement(0, 1,0, Scene::NO_UPDATE);			// don't bother updating the guy on cube 1, use the cube 0 guy always

	Scene::addElement(1, 0,0, Scene::NO_UPDATE);			// the affine matrix renderer on cube 0
	Scene::addElement(2, 1,0, Scene::NO_UPDATE);			// the affine matrix renderer on cube 1

	Scene::endScene();			// complete the scene build

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("An event was returned: %d\n", code);
	}
}
