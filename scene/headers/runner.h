/*
 * runner.h
 *
 *  Created on: Mar 29, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_RUNNER_H_
#define SCENE_RUNNER_H_

namespace Scene
{
	class Runner
	{
		Element *sceneData;
		uint16_t sceneSize;
		Element sceneBuffer[SCENE_MAX_SIZE];
		Sifteo::BitArray<SCENE_MAX_SIZE> initialDraw;
		Sifteo::BitArray<SCENE_MAX_SIZE> redraw;
	public:
		void copyScene(Element *scene, uint16_t count);
		void setScene(Element *scene, uint16_t count);

		int32_t execute();
	};
}
#endif /* RUNNER_H_ */
