/*
 * BatObj.h
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */

#ifndef BATOBJ_H_
#define BATOBJ_H_

#include <sifteo.h>

class BatObj
{
public:
	float y;

	void drawOn(Sifteo::VideoBuffer &v, uint8_t cube);
	bool move(uint8_t cube);
	bool missed(float ball);
};

#endif /* BATOBJ_H_ */
