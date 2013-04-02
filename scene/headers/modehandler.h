/*
 * modehandler.h
 *
 *  Created on: Apr 1, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_MODEHANDLER_H_
#define SCENE_MODEHANDLER_H_

namespace Scene
{
	class ModeHandler
	{
		friend int32_t doRedraw();
	protected:
		virtual Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode) = 0;
		virtual void switchMode(uint8_t cube, uint8_t mode, Sifteo::VideoBuffer &v) = 0;
	};
}
#endif /* MODEHANDLER_H_ */
