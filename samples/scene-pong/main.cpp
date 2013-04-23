#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

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

// type definitions
const int TYPE_BOARD = 0;
const int TYPE_BALL = 1;
const int TYPE_BAT = 2;
const int TYPE_SCORE = 3;
const int TYPE_SPLASH = 4;

const int MODE_MAIN	= 0;
const int MODE_SPLASH = 1;

// physics parameters
const float ballRadius = 2.0;
const float batWidth = 4.0;
const float batLength = 16.0;
const float ballVelocity = 2.0;
const float wallWidth = 4.0;
const float batVelocity = 1.0;
const float batCloseness = 4.0;
const float vision = 100.0;

const uint8_t SERVE_TIME = 120;
const uint8_t SPLASH_TIME = 240;
const uint8_t MAX_SCORE = 9;

const int SCORE_1_X = 12;
const int SCORE_2_X = 1;

// element group pointers (2 elements per group)
Scene::Element *pBall;
Scene::Element *pBats;
Scene::Element *pScores;
Scene::Element *pSplashes;

// RNG for the entire game
Sifteo::Random rng;

// Object classes
class BallObj
{
public:
	float x;
	float y;
	uint8_t angle;	// multiple of 5 degrees
	uint8_t server;
};

class BatObj
{
public:
	float y;
};

class GameLogic
{

};

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
			{
				BallObj &ball = el.obj<BallObj>();
				float xmin = ((el.cube==0) ? 0.0f : 128.0) - ballRadius;
				float xmax = ((el.cube==0) ? 128.0f : 256.0) + ballRadius;
				if((ball.x >- xmin) && (ball.x <= xmax) && (ball.server==2) )
				{
					int x = round(ball.x - xmin - 2*ballRadius);
					int y = round(ball.y - ballRadius);
					v.sprites[0].setImage(Ball);
					v.sprites[0].move(vec(x,y));
				}
				else
				{
					v.sprites[0].hide();
				}
			}
			break;
		case 2: // bat
			{
				BatObj &bat = el.obj<BatObj>();
				int y = round(bat.y - batLength/2);
				int x = (el.cube==0)?0:120;
				v.sprites[1].setImage(Bats, el.cube);
				v.sprites[1].move(vec(x,y));
			}
			break;
		case 3: // score
			v.bg1.image(vec((el.cube==0)?12:1,1), Digits, el.data[0]);
			break;
		case 4: // game over message
			Font::drawCentered(v, vec(0,0), vec(128,32), "GAME OVER");
			break;
		}
	}

	void xvyv(int a, float &xv, float &yv)
	{
		int angle = (8192*a + 36)/72;
		int s = Sifteo::tsini(angle); int c = Sifteo::tcosi(angle);
		xv = (ballVelocity*c)/65536; yv = (ballVelocity*s)/65536;
	}

	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		switch(el.type)
		{
		case 1: // ball
			{
				BallObj &ball = *((BallObj*)el.object);
				if(ball.server !=2)
				{
					// ready to serve the ball
					BatObj &bat= pBats[ball.server].obj<BatObj>();
					ball.x = (ball.server==0) ? (ballRadius + batWidth) : (256.0 - ballRadius - batWidth);
					ball.y = bat.y;
					// -9 +9
					ball.angle = Sifteo::umod( ((ball.server==0)?0:36) + rng.randint(0,9)*2 - 9,72);
					ball.server = 2;
					el.setUpdate(Scene::FULL_UPDATE);
				}
				else
				{
					float xv, yv;
					int a = ball.angle;
					xvyv(a, xv, yv);
					// bounce of left and right edges
					// left
					if( (ball.x < batWidth + ballRadius) && (xv<0.0))
					{
						// hit bat 0?
						BatObj &bat = pBats[0].obj<BatObj>();
						if( (ball.y < bat.y - batLength/2 - ballRadius) || (ball.y > bat.y + batLength/2 + ballRadius) )
						{
							ball.server = 0;
							el.setUpdate(SERVE_TIME);	// reserve the ball in 2 seconds
							Scene::Element &sc = pScores[1];
							sc.repaint().data[0]++;
							if( ++(sc.repaint()).data[0] == MAX_SCORE)
							{
								// stop game elements updating
								pBall[0].clearUpdate();
								pBats[0].clearUpdate();
								pBats[1].clearUpdate();
								pSplashes[0].setUpdate(SPLASH_TIME).show();
								pSplashes[1].show();
							}
						}
						else
						{
							a = Sifteo::umod(36-a + 2*rng.randint(-1,1), 72);
							// angle must be between -9 and 9
							if(a>=36)
							{
								if(a<63) a=63;
							}
							else
							{
								if(a>9) a=9;
							}
							xvyv(a, xv, yv);
						}
					}
					else if( (ball.x > 256.0 - batWidth - ballRadius) && (xv>0.0) )
					{
						// hit bat 1?
						BatObj &bat = pBats[1].obj<BatObj>();
						if( (ball.y < bat.y - batLength/2 - ballRadius) || (ball.y > bat.y + batLength/2 + ballRadius) )
						{
							ball.server = 1;
							el.setUpdate(SERVE_TIME);	// reserve the ball in 2 seconds
							Scene::Element &sc = pScores[0];
							if( ++(sc.repaint()).data[0] == MAX_SCORE)
							{
								// stop game elements updating
								pBall[0].clearUpdate();
								pBats[0].clearUpdate();
								pBats[1].clearUpdate();
								pSplashes[0].setUpdate(SPLASH_TIME).show();
								pSplashes[1].show();
							}

						}
						else
						{
							a = Sifteo::umod(36-a + 2*rng.randint(-1,1), 72);
							// between 27 and 45
							if(a<27) a = 27;
							if(a>45) a = 45;
							xvyv(a, xv, yv);
						}
					}

					if( (ball.y<wallWidth+ballRadius) && (yv<0.0) )
					{
						a = Sifteo::umod(-a,72);
						xvyv(a, xv, yv);
					}
					if( (ball.y>128.0-wallWidth-ballRadius) && (yv>0.0) )
					{
						a = Sifteo::umod(-a,72);
						xvyv(a, xv, yv);
					}
					if(ball.server == 2)	// still in play
					{
						ball.angle = a;
						ball.x += xv;
						ball.y += yv;
					}
				}
				el.repaint();
				pBall[1].repaint();	// and the shadow
			}
			break;
		case 2: // bat
			{
				BallObj &ball = pBall[0].obj<BallObj>();
				if(ball.server == 2)	// only if the ball is in play
				{
					BatObj &bat = el.obj<BatObj>();
					float batx = (el.cube == 0) ? 0.0f : 256.0f;
					if(abs(ball.x - batx) < vision)
					{
						// move the bat
						if(bat.y < ball.y - batCloseness) bat.y += batVelocity;
						else if(bat.y > ball.y + batCloseness) bat.y -= batVelocity;
						if(bat.y < wallWidth + batLength/2) bat.y = wallWidth + batLength/2;
						if(bat.y > 128.0 - wallWidth - batLength/2) bat.y = 128.0 - wallWidth - batLength/2;
						el.repaint();
					}
				}
			}
			break;
		case 4:	// game over
			{
				// reset the scores and all the elements to update again
				Scene::Element &be = pBall[0];
				BallObj &ball = be.obj<BallObj>();
				ball.server = 0;
				be.setUpdate(SERVE_TIME);

				// set the scores to zero and repaint, that will make the entire layer 0 repaint anyway
				pScores[0].repaint().data[0] = 0;
				pScores[1].repaint().data[0] = 0;

				pBats[0].fullUpdate();
				pBats[1].fullUpdate();
				pSplashes[0].hide();
				pSplashes[1].hide();
			}
			break;
		}
		return 0;	// just say it's never returning
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

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("execute returned: %d\n", code);
	}
}
