/*
 * BallObj.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */
#include "public.h"
#include "BallObj.h"
#include "BatObj.h"
#include "GameLogic.h"
#include "assets.gen.h"

using namespace Sifteo;

bool BallObj::inPlay()
{
	return (server == 2);
}

void BallObj::drawOn(Sifteo::VideoBuffer &v, uint8_t cube)
{
	float xmin = ((cube == 0) ? 0.0f : 128.0) - ballRadius;
	float xmax = ((cube == 0) ? 128.0f : 256.0) + ballRadius;
	if ((x > -xmin) && (x <= xmax) && inPlay())
	{
		int _x = round(x - xmin - 2 * ballRadius);
		int _y = round(y - ballRadius);
		v.sprites[0].setImage(Ball);
		v.sprites[0].move(vec(_x, _y));
	}
	else
	{
		v.sprites[0].hide();
	}
}

void BallObj::performServe()
{
	x = (server == 0) ?
			(ballRadius + batWidth) : (256.0 - ballRadius - batWidth);
	y = pBats[server].obj<BatObj>().y;
	angle = Sifteo::umod(((server == 0) ? 0 : 36) + rng.randint(0, 9) * 2 - 9, 72);
	server = 2;
}

void BallObj::doMotion(int a, float xv, float yv)
{
	angle = a;
	x += xv;
	y += yv;
}

bool BallObj::offLeft()
{
	return (x < batWidth + ballRadius);
}
bool BallObj::offRight()
{
	return (x > 256.0 - batWidth - ballRadius);
}
bool BallObj::offTop()
{
	return (y < wallWidth + ballRadius);
}
bool BallObj::offBottom()
{
	return (y > 128.0 - wallWidth - ballRadius);
}

void BallObj::update(Scene::Element &el)
{
	if (!inPlay())
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
		if (offLeft() && (xv < 0.0))
		{
			// hit bat 0?
			if (pBats[0].obj<BatObj>().missed(y))
			{
				GameLogic::scorePoint(1);
			}
			else
			{
				GameLogic::verticalMirror(a, xv, yv, 0);
			}
		}
		else if (offRight() && (xv > 0.0))
		{
			// hit bat 1?
			if (pBats[1].obj<BatObj>().missed(y))
			{
				GameLogic::scorePoint(0);
			}
			else
			{
				GameLogic::verticalMirror(a, xv, yv, 36);
			}
		}

		if (offTop() && (yv < 0.0))
		{
			GameLogic::horizontalMirror(a, xv, yv);
		}
		else if (offBottom() && (yv > 0.0))
		{
			GameLogic::horizontalMirror(a, xv, yv);
		}
		if (inPlay()) // still in play
		{
			doMotion(a, xv, yv);
		}
	}
	el.repaint().next().repaint();
}

