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
		// only one mode in this demo
		v.initMode(BG2);
		setAffine(v, 0);				// make sure the default matrix is set (not all zero!)
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

	void syncMatrixAndOrientation(Sifteo::VideoBuffer &v, const AffineMatrix &m, Sifteo::Side orientation)
	{
		// 1) set lock bits
		v.lock(0x200);	// locks bytes 0x200-0x21F (matrix is 0x200-0x020B)
		v.lock(0x3FF);	// locks bytes 0x3E0-0x3FF (orientation is 0x3FF)

		// 2) Update VRAM
		// :matrix - from Sifteo headers, only difference is no _SYS call to vram write (we need to manage change bits)
        _SYSAffine a = {
        		 256.0f * m.cx + 0.5f,
        		 256.0f * m.cy + 0.5f,
        		 256.0f * m.xx + 0.5f,
        		 256.0f * m.xy + 0.5f,
        		 256.0f * m.yx + 0.5f,
        		 256.0f * m.yy + 0.5f,
        };
        Sifteo::memcpy16(&v.sys.vbuf.vram.words[0x100], (const uint16_t *)&a, 6);

        // :rotation - from Sifteo headers, except we can't use xorb(), again we need to manage our own change bits
        const uint32_t sideToRotation =
        		(ROT_NORMAL     << 0)  |
        		(ROT_LEFT_90    << 8)  |
        		(ROT_180        << 16) |
        		(ROT_RIGHT_90   << 24) ;
        uint8_t r = sideToRotation >> (orientation * 8);
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = v.peekb(offsetof(_SYSVideoRAM, flags));
        // the __sync xor method only comes in widths 32 bits and up, so we xor into the last 32 bits of VRAM
        __sync_xor_and_fetch( ((int32_t*)(&v.sys.vbuf.vram.words)) + 0xFF, ((int32_t)((r ^ flags) & mask))<<24 );

        // 3) set cm1 change bits
		__sync_or_and_fetch(&v.sys.vbuf.cm1[8], 0xFC000000);	// first six words of chunk starting at 0x200
		__sync_or_and_fetch(&v.sys.vbuf.cm1[15], 0x00000001);	// last word of chunk starting at byte address 0x3C0

        // 4) set cm16 to get things going
		__sync_or_and_fetch(&v.sys.vbuf.cm16, 0x00008001);		// locked regions 0x200 and 0x3E0. start streaming now

        // 5) unlock (paint will do this), but let's jump the gun
		v.unlock();
	}

	void setAffineAndOrientation(Sifteo::VideoBuffer &v, uint16_t angle, Sifteo::Side orientation)
	{
		float f = (M_TAU*angle)/65536.0f;
		AffineMatrix m = AffineMatrix::identity();

		m.translate(64,64);			// move center to origin
		m.rotate(f);				// do the rotation
		m.translate(-64,-64);		// move it back
		syncMatrixAndOrientation(v, m, orientation);
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
			setAffineAndOrientation(v, (scratch & 0x3FFF) - 0x2000, (Sifteo::Side)(scratch>>14));
			break;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		// the only element to update is the rotation. We treat el.object as a 16-bit unsigned for this
		uint16_t *pa = (uint16_t*)&el.object;				// use the object field as a uint16.
		(*pa) += fc*0x0100;									// full rotation takes 256 frames

		// tell items 2 and 3 (the two rotations) to repaint
		Scene::getElement(2).repaint();
		Scene::getElement(3).repaint();
		return 0;
	}

	void cubeCount(uint8_t cubes) {}
	void neighborAlert() {}
	void attachMotion(uint8_t cube, Sifteo::CubeID parameter) {}
	void updateAllMotion(const Sifteo::BitArray<CUBE_ALLOCATION> &cubeMap) {}
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

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("An event was returned: %d\n", code);
	}
}
