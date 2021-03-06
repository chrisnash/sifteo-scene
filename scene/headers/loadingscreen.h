/*
 * loadingscreen.h
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_LOADINGSCREEN_H_
#define SCENE_LOADINGSCREEN_H_

namespace Scene
{
	class LoadingScreen
	{
		friend int32_t doRedraw(Handler &h);
		friend void unhappyLoader();
		friend void unhappyDraw(Handler &handler, unsigned i, bool drawBan);
	protected:
		virtual bool init(uint8_t cube, Sifteo::VideoBuffer &v, uint8_t part) = 0;
		virtual void onAttach(uint8_t cube, Sifteo::VideoBuffer &v, uint8_t part) = 0;
		virtual void update(uint8_t cube, float progress, Sifteo::VideoBuffer &v) = 0;
	};
}



#endif /* LOADINGSCREEN_H_ */
