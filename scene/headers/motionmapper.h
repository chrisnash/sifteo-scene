/*
 * motionmapper.h
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */

#ifndef MOTIONMAPPER_H_
#define MOTIONMAPPER_H_

namespace Scene
{
	class MotionMapper
	{
	public:
		virtual void attachMotion(uint8_t cube, Sifteo::CubeID parameter);
	};
}


#endif /* MOTIONMAPPER_H_ */
