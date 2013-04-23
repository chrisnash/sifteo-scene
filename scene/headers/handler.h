/*
 * modehandler.h
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_HANDLER_H_
#define SCENE_HANDLER_H_

namespace Scene
{
	class Handler
	{
		friend int32_t execute(Handler &h);
		friend int32_t doRedraw(Handler &h);
		friend class CubeMapping;
	protected:
		// mode functions
		virtual Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode) = 0;
		virtual bool switchMode(uint8_t cube, uint8_t mode, Sifteo::VideoBuffer &v) = 0;
		// element functions
		virtual void drawElement(Element &el, Sifteo::VideoBuffer &v) = 0;
		virtual int32_t updateElement(Element &el, uint8_t fc=1) = 0;
		// callbacks
		virtual void cubeCount(uint8_t cubes) = 0;
		virtual void neighborAlert() = 0;
		// motion mapping
		virtual void attachMotion(uint8_t cube, Sifteo::CubeID parameter) = 0;
		virtual void updateAllMotion(const Sifteo::BitArray<CUBE_ALLOCATION> &cubeMap) = 0;
	};
}
#endif /* HANDLER_H_ */
