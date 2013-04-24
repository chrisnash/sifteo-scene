/*
 * element.h
 *
 *  Created on: Mar 29, 2013
 *      Author: chrisnash
 */

#ifndef SCENE_ELEMENT_H_
#define SCENE_ELEMENT_H_

#include <sifteo.h>

namespace Scene
{
	class Element
	{
	public:
		uint8_t type;			// the element type
		uint8_t cube;			// the cube for this entity (or undefined)
		uint8_t update;			// framecount before next update
		uint8_t autoupdate;		// framecount to put back in update register for automatic looping
		union
		{
			void *object;		// pointer to user defined object

			// some handy union offsets to treat the data as 8, 16, or 32 bit
			uint8_t data[4];	// user defined data
			uint16_t data16[2];
			int8_t sdata[4];
			int16_t sdata16[2];

			uint8_t d8;
			uint16_t d16;
			uint32_t d32;

			int8_t s8;
			int16_t s16;
			int32_t s32;
		};

		Element &repaint();			// repaint this element
		Element &clearUpdate();
		Element &setUpdate(uint8_t update=1);
		Element &fullUpdate();
		Element &setAutoupdate(uint8_t autoupdate);
		Element &show();
		Element &hide();
		Element &setMode(uint8_t m);
		Element &setCube(uint8_t c);
		Element &setType(uint8_t t);
		Element &setObject(void *o);

		uint8_t &mode();
		uint16_t index();
		bool visible();

		Element *shadow(uint8_t count);
		Element *duplicate(uint8_t count);
		Element *fromTemplate(uint8_t count, void *objects[]);

		Element &offset(int16_t offset);
		Element &next();
		Element &prev();

		template<typename T>
		T *objptr() {return (T*)object;}

		template<typename T>
		T& obj() {return *objptr<T>();}
	};
}
#endif /* ELEMENT_H_ */
