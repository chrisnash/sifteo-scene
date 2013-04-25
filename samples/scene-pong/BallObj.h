/*
 * BallObj.h
 *
 *  Created on: Apr 24, 2013
 *      Author: chrisnash
 */

#ifndef BALLOBJ_H_
#define BALLOBJ_H_

#include <sifteo.h>
#include <scene.h>

class BallObj
{
public:
	float x;
	float y;
	uint8_t angle;	// multiple of 5 degrees
	uint8_t server;

	bool inPlay();
	void drawOn(Sifteo::VideoBuffer &v, uint8_t cube);
	void performServe();
	void doMotion(int a, float xv, float yv);
	bool offLeft();
	bool offRight();
	bool offTop();
	bool offBottom();
	void update(Scene::Element &el);
};

#endif /* BALLOBJ_H_ */
