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
const int SCORE_Y = 1;
const int SCORE_XS = 3;
const int SCORE_YS = 5;

// element group pointers (2 elements per group)
Scene::Element *pBall;
Scene::Element *pBats;
Scene::Element *pScores;
Scene::Element *pSplashes;

// RNG for the entire game
Sifteo::Random rng;

// Object classes
class BatObj
{
public:
	float y;

	void drawOn(Sifteo::VideoBuffer &v, uint8_t cube);
	bool move(uint8_t cube);
	bool missed(float ball);
};

class GameLogic
{
public:
	static void serveBall(uint8_t player);
	static void restartGame();
	static void endGame();
	static void scorePoint(uint8_t player);
	static void xvyv(int a, float &xv, float &yv);
	static void verticalMirror(int &a, float &xv, float &yv, int reference);
	static void horizontalMirror(int &a, float &xv, float &yv);
};

class BallObj
{
public:
	float x;
	float y;
	uint8_t angle;	// multiple of 5 degrees
	uint8_t server;

	bool inPlay()
	{
		return (server==2);
	}

	void drawOn(Sifteo::VideoBuffer &v, uint8_t cube)
	{
		float xmin = ((cube==0) ? 0.0f : 128.0) - ballRadius;
		float xmax = ((cube==0) ? 128.0f : 256.0) + ballRadius;
		if((x >- xmin) && (x <= xmax) && inPlay() )
		{
			int _x = round(x - xmin - 2*ballRadius);
			int _y = round(y - ballRadius);
			v.sprites[0].setImage(Ball);
			v.sprites[0].move(vec(_x,_y));
		}
		else
		{
			v.sprites[0].hide();
		}
	}

	void performServe()
	{
		x = (server==0) ? (ballRadius + batWidth) : (256.0 - ballRadius - batWidth);
		y = pBats[server].obj<BatObj>().y;
		angle = Sifteo::umod( ((server==0)?0:36) + rng.randint(0,9)*2 - 9,72);
		server = 2;
	}

	void doMotion(int a, float xv, float yv)
	{
		angle = a;
		x += xv;
		y += yv;
	}

	bool offLeft()
	{
		return (x < batWidth + ballRadius);
	}
	bool offRight()
	{
		return (x > 256.0 - batWidth - ballRadius);
	}
	bool offTop()
	{
		return (y<wallWidth+ballRadius);
	}
	bool offBottom()
	{
		return (y>128.0-wallWidth-ballRadius);
	}

	void update(Scene::Element &el)
	{
		if(!inPlay())
		{
			performServe();
			el.fullUpdate();
		}
		else
		{
			float xv, yv;
			int a = angle;
			GameLogic::xvyv(a, xv, yv);
			// bounce of left and right edges
			// left
			if( offLeft() && (xv<0.0))
			{
				// hit bat 0?
				if( pBats[0].obj<BatObj>().missed(y) )
				{
					GameLogic::scorePoint(1);
				}
				else
				{
					GameLogic::verticalMirror(a, xv, yv, 0);
				}
			}
			else if( offRight() && (xv>0.0) )
			{
				// hit bat 1?
				if( pBats[1].obj<BatObj>().missed(y) )
				{
					GameLogic::scorePoint(0);
				}
				else
				{
					GameLogic::verticalMirror(a, xv, yv, 36);
				}
			}

			if( offTop() && (yv<0.0) )
			{
				GameLogic::horizontalMirror(a, xv, yv);
			}
			else if( offBottom() && (yv>0.0) )
			{
				GameLogic::horizontalMirror(a, xv, yv);
			}
			if(inPlay())	// still in play
			{
				doMotion(a, xv, yv);
			}
		}
		el.repaint().next().repaint();
	}
};

void BatObj::drawOn(Sifteo::VideoBuffer &v, uint8_t cube)
{
	int _y = round(y - batLength/2);
	int _x = (cube==0)?0:120;
	v.sprites[1].setImage(Bats, cube);
	v.sprites[1].move(vec(_x,_y));
}

bool BatObj::move(uint8_t cube)
{
	BallObj &ball = pBall[0].obj<BallObj>();
	if(ball.inPlay())
	{
		float batx = (cube == 0) ? 0.0f : 256.0f;
		if(abs(ball.x - batx) < vision)
		{
			// move the bat
			if(y < ball.y - batCloseness) y += batVelocity;
			else if(y > ball.y + batCloseness) y -= batVelocity;
			if(y < wallWidth + batLength/2) y = wallWidth + batLength/2;
			if(y > 128.0 - wallWidth - batLength/2) y = 128.0 - wallWidth - batLength/2;
			return true;
		}
	}
	return false;	// didn't move it
};

bool BatObj::missed(float ball)
{
	return (ball < y - batLength/2 - ballRadius) || (ball > y + batLength/2 + ballRadius);
}

void GameLogic::serveBall(uint8_t player)
{
	pBall[0].setUpdate(SERVE_TIME).obj<BallObj>().server = player;
}

void GameLogic::restartGame()
{
	// set the ball to serve
	serveBall(0);

	// set the scores to zero and repaint, that will make the entire layer 0 repaint anyway
	pScores[0].repaint().d8 = 0;
	pScores[1].repaint().d8 = 0;

	pBats[0].fullUpdate().next().fullUpdate();
	pSplashes[0].hide().next().hide();
}

void GameLogic::endGame()
{
	// stop game elements updating
	pBall[0].clearUpdate();
	pBats[0].clearUpdate().next().clearUpdate();
	pSplashes[0].setUpdate(SPLASH_TIME).show().next().show();
}

void GameLogic::scorePoint(uint8_t player)
{
	// other guy serves
	serveBall(1-player);
	// repaint and increase guy's score
	if( ++(pScores[player].repaint().d8) == MAX_SCORE)
	{
		endGame();
	}
}

// convert angles to velocities
void GameLogic::xvyv(int a, float &xv, float &yv)
{
	int angle = (8192*a + 36)/72;
	int s = Sifteo::tsini(angle); int c = Sifteo::tcosi(angle);
	xv = (ballVelocity*c)/65536; yv = (ballVelocity*s)/65536;
}

// vertical mirror, add some random noise and check within 45 degrees
void GameLogic::verticalMirror(int &a, float &xv, float &yv, int reference)
{
	a = Sifteo::umod(36-a + 2*rng.randint(-1,1), 72);
	// angle must be between -9 and 9 of reference
	int offset = Sifteo::umod(a - reference, 72); if(offset>=36) offset-=72;	// -36 to + 36
	if(offset<-9) a = Sifteo::umod(reference-9, 72);
	if(offset>9) a = Sifteo::umod(reference+9, 72);
	xvyv(a, xv, yv);
}

void GameLogic::horizontalMirror(int &a, float &xv, float &yv)
{
	a = Sifteo::umod(-a,72);
	xvyv(a, xv, yv);
}

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
