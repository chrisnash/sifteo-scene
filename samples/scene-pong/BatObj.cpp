/*
 * BatObj.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */
#include "public.h"
#include "BatObj.h"
#include "BallObj.h"
#include "assets.gen.h"

using namespace Sifteo;

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



