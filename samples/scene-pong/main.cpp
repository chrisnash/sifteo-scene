#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("scene-pong")
    .package("com.github.sifteo-scene.scene-pong", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

float ballRadius = 2.0;
float batWidth = 4.0;
float batLength = 16.0;
float ballVelocity = 2.0;
float wallWidth = 4.0;
float batVelocity = 1.0;
float batCloseness = 4.0;
float vision = 100.0;	// is this smart enough

uint16_t ballElement;
uint16_t ballSlave;
uint16_t bat0element;
uint16_t bat1element;
uint16_t score0element;
uint16_t score1element;
uint16_t go0element;
uint16_t go1element;

uint8_t maxScore = 9;

Sifteo::Random rng;

class BallObj
{
public:
	float x;
	float y;
	uint8_t angle;	// multiple of 5 degrees
	uint8_t server;
};

class Bat
{
public:
	float y;
};

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
				BallObj &ball = *((BallObj*)(Scene::getElement(ballElement).object));
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
				Bat *bat = (Bat*)el.object;
				int y = round(bat->y - batLength/2);
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
					Bat &bat= *((Bat *)(Scene::getElement((ball.server==0)?bat0element:bat1element).object));
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
						Bat &bat = *((Bat*)Scene::getElement(bat0element).object);
						if( (ball.y < bat.y - batLength/2 - ballRadius) || (ball.y > bat.y + batLength/2 + ballRadius) )
						{
							ball.server = 0;
							el.setUpdate(120);	// reserve the ball in 2 seconds
							Scene::Element &sc = Scene::getElement(score1element);
							sc.data[0]++;
							sc.repaint();
							if(sc.data[0] == maxScore)
							{
								// stop game elements updating
								Scene::getElement(ballElement).clearUpdate();
								Scene::getElement(bat0element).clearUpdate();
								Scene::getElement(bat1element).clearUpdate();
								Scene::Element &go = Scene::getElement(go0element);
								go.setUpdate(240);	// make the game over message disappear after 4 seconds
								go.show();
								Scene::getElement(go1element).show();
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
						Bat &bat = *((Bat*)Scene::getElement(bat1element).object);
						if( (ball.y < bat.y - batLength/2 - ballRadius) || (ball.y > bat.y + batLength/2 + ballRadius) )
						{
							ball.server = 1;
							el.setUpdate(120);	// reserve the ball in 2 seconds
							Scene::Element &sc = Scene::getElement(score0element);
							sc.data[0]++;
							sc.repaint();
							if(sc.data[0] == maxScore)
							{
								// stop game elements updating
								Scene::getElement(ballElement).clearUpdate();
								Scene::getElement(bat0element).clearUpdate();
								Scene::getElement(bat1element).clearUpdate();
								Scene::Element &go = Scene::getElement(go0element);
								go.setUpdate(240);	// make the game over message disappear after 4 seconds
								go.show();
								Scene::getElement(go1element).show();
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
				Scene::getElement(ballSlave).repaint();
			}
			break;
		case 2: // bat
			{
				BallObj &ball = *((BallObj*)(Scene::getElement(ballElement).object));
				if(ball.server == 2)	// only if the ball is in play
				{
					Bat &bat = *((Bat*)el.object);
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
				Scene::Element &be = Scene::getElement(ballElement);
				BallObj &ball = *((BallObj*)be.object);
				ball.server = 0;
				be.setUpdate(120);

				// set the scores to zero and repaint, that will make the entire layer 0 repaint anyway
				Scene::Element &s0 = Scene::getElement(score0element);
				s0.data[0] = 0;
				s0.repaint();
				Scene::Element &s1 = Scene::getElement(score1element);
				s1.data[0] = 0;
				s1.repaint();

				Scene::getElement(bat0element).setUpdate(Scene::FULL_UPDATE);
				Scene::getElement(bat1element).setUpdate(Scene::FULL_UPDATE);
				Scene::getElement(go0element).hide();
				Scene::getElement(go1element).hide();
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
	Scene::addElement(0, 	0,0);
	Scene::addElement(0, 	1,0);

	// ball
	BallObj ballobj;
	ballobj.server = 0;	// player 1 will serve in 120 frames
	ballElement = Scene::addElement(1,	0,0, 120, Scene::NO_UPDATE, &ballobj);
	ballSlave = Scene::addElement(1,	1,0);

	// bat
	Bat bat1; bat1.y = 64.0;
	Bat bat2; bat2.y = 64.0;
	bat0element = Scene::addElement(2,	0,0, Scene::FULL_UPDATE, Scene::NO_UPDATE, &bat1);
	bat1element = Scene::addElement(2,	1,0, Scene::FULL_UPDATE, Scene::NO_UPDATE, &bat2);

	// score
	score0element = Scene::addElement(3,	0,0);
	score1element = Scene::addElement(3,	1,0);

	// game over
	go0element = Scene::addElement(4,	0|Scene::HIDE,1);
	go1element = Scene::addElement(4,	1|Scene::HIDE,1);

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("execute returned: %d\n", code);
	}
}
