/*
 * GameLogic.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */
#include "public.h"
#include "GameLogic.h"
#include "BallObj.h"

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



