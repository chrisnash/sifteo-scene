#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

#include "public.h"
#include "GameLogic.h"
#include "BatObj.h"
#include "BallObj.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("scene-pong")
    .package("com.github.sifteo-scene.scene-pong", "0.1")
//    .icon(Icon)
    .cubeRange(2, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

// Object instantiation
BallObj _ball;
BatObj _bats[2];

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

	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v, CubeID param)
	{
		if(mode==0)
		{
			v.initMode(BG0_SPR_BG1);
			// create the BG1 mask
			v.bg1.fillMask(vec((cube==0)?SCORE_1_X:SCORE_2_X,SCORE_Y), vec(SCORE_XS,SCORE_YS));
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
		case TYPE_BOARD:	// board
			v.bg0.image(vec(0,0), Board, el.cube);		// frame number is cube
			break;
		case TYPE_BALL: // ball
			el.obj<BallObj>().drawOn(v, el.cube);
			break;
		case TYPE_BAT: // bat
			el.obj<BatObj>().drawOn(v, el.cube);
			break;
		case TYPE_SCORE: // score
			v.bg1.image(vec((el.cube==0)?SCORE_1_X:SCORE_2_X,SCORE_Y), Digits, el.d8);
			break;
		case TYPE_SPLASH: // game over message
			Font::drawCentered(v, vec(0,0), vec(128,32), "GAME OVER");
			break;
		}
	}



	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		switch(el.type)
		{
		case TYPE_BALL: // ball
			el.obj<BallObj>().update(el);
			break;
		case TYPE_BAT: // bat
			if(el.obj<BatObj>().move(el.cube))
			{
				el.repaint();
			}
			break;
		case TYPE_SPLASH:	// game over timer expired
			GameLogic::restartGame();
			break;
		}
		return 0;	// just say it's never returning from scene
	}

	void cubeCount(uint8_t cubes) {}
	void neighborAlert() {}
	void attachMotion(uint8_t cube, Sifteo::CubeID parameter) {}
	void updateAllMotion(const Sifteo::BitArray<CUBE_ALLOCATION> &cubeMap) {}
};

void main()
{
	// initialize scene
	Scene::initialize();

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	// board
	Scene::createElement(TYPE_BOARD).duplicate(2);

	// ball
	_ball.server = 0;
	pBall = Scene::createElement(TYPE_BALL).setUpdate(SERVE_TIME).setObject(&_ball).shadow(2);

	// bats
	_bats[0].y = 64.0;
	_bats[1].y = 64.0;

	void *batPointers[] = {_bats+0, _bats+1};
	pBats = Scene::createElement(TYPE_BAT).fullUpdate().fromTemplate(2, batPointers);

	// scores
	pScores = Scene::createElement(TYPE_SCORE).duplicate(2);

	// game over
	pSplashes = Scene::createElement(TYPE_SPLASH).setMode(1).hide().duplicate(2);

	GameLogic::restartGame();
	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("execute returned: %d\n", code);
	}
}
