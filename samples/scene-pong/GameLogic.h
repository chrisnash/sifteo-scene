/*
 * GameLogic.h
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */

#ifndef GAMELOGIC_H_
#define GAMELOGIC_H_

#include <sifteo.h>

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

#endif /* GAMELOGIC_H_ */
