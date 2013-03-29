#include <sifteo.h>
#include <scene.h>

using namespace Sifteo;

static Metadata M = Metadata()
    .title("simple-image")
    .package("com.github.sifteo-scene.simple-image", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

void main()
{
	// initialize scene
	Scene::initialize();

	// use the scene builder API
	Scene::Element *p = Scene::beginScene();
	LOG("Initial scene pointer %p\n", p);

	// build the sully element
	bzero(*p);					// type 0 update 0 mode 0 cube 0
	p->mode |= Scene::ATTACHED;	// can't draw tiles unless the cube is attached
	p++;						// next element

	Scene::endScene(p);			// complete the scene build

	Scene::execute();			// this call never returns because we have no update methods.
}
