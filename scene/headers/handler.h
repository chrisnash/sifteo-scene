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
		friend int32_t doRedraw(Handler &h);
	protected:
		// mode functions
		virtual Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode) = 0;
		virtual bool switchMode(uint8_t cube, uint8_t mode, Sifteo::VideoBuffer &v) = 0;
		// element functions
		virtual void drawElement(Element &el, Sifteo::VideoBuffer &v) = 0;
		virtual int32_t updateElement(Element &el, uint8_t fc=0) = 0;

	};
}
#endif /* HANDLER_H_ */
