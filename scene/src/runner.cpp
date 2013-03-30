#include <sifteo.h>
#include "runner.h"

using namespace Scene;


int32_t Runner::execute()
{
	int32_t exitCode = 0;

	while(1)
	{
		// update loop
		uint8_t fc = Scene::updateCount();
		for(uint16_t i = 0; (i<sceneSize); i++)
		{
			Element *element = sceneData + i;
			if(element->update == Scene::FULL_UPDATE)
			{
				exitCode = Scene::doUpdate(element, fc);
				if(exitCode !=0) return exitCode;
			}
			else if(element->update != 0)
			{
				uint8_t ur = fc;
				while(ur >= element->update)
				{
					ur -= element->update;
					element->update = 0;
					exitCode = Scene::doUpdate(element);
					if(exitCode !=0) return exitCode;
				}
				element->update -= ur;
			}
		}

		// redraw loop. Note redraw and repaint events may force graphics initialization
		unsigned index;
		while(redraw.clearFirst(index))
		{
			Element *element = sceneData + index;
			if(Scene::doRedraw(element))
			{
				redraw = initialDraw;
			}
		}

		// repaint
		if(Scene::doRepaint())
		{
			redraw = initialDraw;
		}
	}
	return exitCode;
}
