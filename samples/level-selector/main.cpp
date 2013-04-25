#include <sifteo.h>
#include <scene.h>
#include <propfont.h>

#include "assets.gen.h"

using namespace Sifteo;

static Metadata M = Metadata()
    .title("level-selector")
    .package("com.github.sifteo-scene.level-selector", "0.1")
//    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot bootstrap = AssetSlot::allocate();
AssetSlot slot_1 = AssetSlot::allocate();
AssetSlot slot_2 = AssetSlot::allocate();
AssetSlot slot_3 = AssetSlot::allocate();

Scene::Element *scrollRegister;
Scene::Element *scrollers;

class TextField
{
public:
	String<64> text;
};

// how big an entry is in the window
const int8_t windowSize = 12;
const int16_t scrollMax = 96;
const int16_t scrollHalf = scrollMax >> 1;
// how many rows by default we try to buffer in both directions
const int8_t bidiLookahead = 2;
// how many rows we aim to buffer when scrolling
const int8_t scrollLookback = 1;
const int8_t scrollLookforward = 4;

class ScrollListener
{
public:
	virtual void onBegin(Scene::Element &el, int16_t direction) = 0;
	virtual void onUpdate(Scene::Element &el, int16_t position, int16_t direction) = 0;
	virtual void onHalfway(Scene::Element &el, int16_t direction) = 0;
	virtual void onComplete(Scene::Element &el, int16_t direction) = 0;
};

class ScrollingSelector : public ScrollListener
{
public:
	int16_t windowOffset = 0;
	int16_t baseOffset = 0;		// scrolling base offset

	int8_t windowStart = 0;
	int8_t windowEnd = 0;

	uint16_t itemBuffer[3] = {0,0,0};		// previous, current, next

	virtual void doRender(Sifteo::VideoBuffer &v, uint32_t bufferLine, uint16_t item, uint32_t itemLine) = 0;
	virtual int xoffset() = 0;
	virtual int yoffset() = 0;

	void renderLine(Sifteo::VideoBuffer &v, int16_t l)
	{
		uint32_t bufferLine = Sifteo::umod(windowOffset + l, 18);
		int16_t imageOffset = Sifteo::umod(l, windowSize);
		int16_t imageIndex = ((l - imageOffset) / windowSize) + 1;

		//LOG("Rendering line %d image offset %d image index %d to buffer %d\n", l, imageOffset, imageIndex, bufferLine);

		if((imageIndex<0)||(imageIndex>2))
		{
			v.bg0.fill( vec(0U, bufferLine), vec(17U, 1U), Black);
		}
		else
		{
			doRender(v, bufferLine, itemBuffer[imageIndex], imageOffset);
		}
	}

	void draw(Sifteo::VideoBuffer &v, int16_t scrollOffset = 0)
	{
		scrollOffset += baseOffset;

		//LOG("Drawing item with scroll offset %d\n", scrollOffset);
		// calculate the first and last lines needed on the screen
		int16_t startLine = (scrollOffset >> 3);	// rounded down
		int16_t endLine = (scrollOffset + windowSize*8 +7) >> 3;
		if(scrollOffset > 0)
		{
			// scrolling down
			startLine -= scrollLookback;
			endLine += scrollLookforward;
		}
		else if(scrollOffset < 0)
		{
			startLine -= scrollLookforward;
			endLine += scrollLookback;
		}
		else
		{
			startLine -= bidiLookahead;
			endLine += bidiLookahead;
		}
		//LOG("Current buffer status: %d,%d\n", windowStart, windowEnd);
		//LOG("Requested buffer status: %d,%d\n", startLine, endLine);

		// if there's no window overlap at all, reset the window (typically we are empty or reset)
		if((windowEnd<=startLine)||(windowStart>=endLine))
		{
			windowStart = windowEnd = startLine;
		}
		// render the lines at the beginning you don't already have
		for(int16_t l = startLine; l<windowStart; l++)
		{
			renderLine(v, l);
		}
		// render the lines at the end you don't already have
		for(int16_t l=windowEnd; l<endLine; l++)
		{
			renderLine(v, l);
		}
		// remember what you've done
		windowStart = startLine;
		windowEnd = endLine;
		// set the pan register
		v.bg0.setPanning( vec(xoffset(), (int)Sifteo::umod(windowOffset*8+scrollOffset+yoffset(), 144) ) );
	}

	void clearCache()
	{
		// mark everything as dirty
		windowStart = windowEnd = 0;
	}

	void onBegin(Scene::Element &el, int16_t direction)
	{
		baseOffset = 0;
	}

	void onUpdate(Scene::Element &el, int16_t position, int16_t direction)
	{
		el.repaint();
	}

	virtual uint16_t nextItem(uint16_t item) = 0;
	virtual uint16_t prevItem(uint16_t item) = 0;

	void onHalfway(Scene::Element &el, int16_t direction)
	{
		// once halfway, change the current image and the windowing parameters
		if(direction==1)
		{
			itemBuffer[0] = itemBuffer[1];
			itemBuffer[1] = itemBuffer[2];
			itemBuffer[2] = nextItem(itemBuffer[2]);
		}
		else
		{
			itemBuffer[2] = itemBuffer[1];
			itemBuffer[1] = itemBuffer[0];
			itemBuffer[0] = prevItem(itemBuffer[0]);
		}
		// move the base address of the window down and the cached status in the opposite direction
		windowOffset = Sifteo::umod(windowOffset + direction * windowSize, 18);
		windowStart -= windowSize*direction;
		windowEnd -= windowSize*direction;
		baseOffset = -windowSize*8*direction;
	}

	void onComplete(Scene::Element &el, int16_t direction)
	{
		baseOffset = 0;
	}
};

class ChapterSelector : public ScrollingSelector
{
	virtual void doRender(Sifteo::VideoBuffer &v, uint32_t bufferLine, uint16_t item, uint32_t itemLine)
	{
		if((itemLine>=4)||(itemLine<=8))
		{
			v.bg0.fill( vec(0U, bufferLine), vec(7U, 1U), Black);
			v.bg0.image( vec(7U, bufferLine), vec(3,1), Digits, vec(0U, itemLine - 4), (item >> 8) & 0x00FF);
			v.bg0.fill( vec(10U, bufferLine), vec(7U, 1U), Black);
		}
		else
		{
			v.bg0.fill( vec(0U, bufferLine), vec(17U, 1U), Black);
		}
	}

	virtual uint16_t nextItem(uint16_t item)
	{
		return 0x0000;
	}

	virtual uint16_t prevItem(uint16_t item)
	{
		return 0x0000;
	}

	virtual int xoffset()
	{
		return 4;
	}

	virtual int yoffset()
	{
		return 4;
	}
};

class LevelSelector : public ScrollingSelector
{
	virtual void doRender(Sifteo::VideoBuffer &v, uint32_t bufferLine, uint16_t item, uint32_t itemLine)
	{
		if((itemLine>=4)||(itemLine<=8))
		{
			// draw the digit indicated by item high byte (7 spaces before
			// yes, I know I'm overdrawing here
			v.bg0.fill( vec(0U, bufferLine), vec(3U, 1U), Black);
			v.bg0.image( vec(3U, bufferLine), vec(3,1), Digits, vec(0U, itemLine - 4), (item >> 8) & 0x00FF);
			v.bg0.fill( vec(6U, bufferLine), vec(1U, 1U), Black);
			v.bg0.image( vec(7U, bufferLine), vec(3,1), Digits, vec(0U, itemLine - 4), 10); // dash
			v.bg0.fill( vec(10U, bufferLine), vec(1U, 1U), Black);
			v.bg0.image( vec(11U, bufferLine), vec(3,1), Digits, vec(0U, itemLine - 4), (item) & 0x00FF);
			v.bg0.fill( vec(14U, bufferLine), vec(3U, 1U), Black);
		}
		else
		{
			v.bg0.fill( vec(0U, bufferLine), vec(17U, 1U), Black);
		}
	}

	virtual int xoffset()
	{
		return 4;
	}

	virtual int yoffset()
	{
		return 4;
	}

	virtual uint16_t nextItem(uint16_t item)
	{
		return 0x0000;
	}

	virtual uint16_t prevItem(uint16_t item)
	{
		return 0x0000;
	}
};

class Debounce
{
	CubeID id;
	bool debounce;
public:
	void setCube(CubeID c)
	{
		id = c;
		debounce = id.isTouching();
	}

	bool isTouching()
	{
		bool now = id.isTouching();
		bool then = debounce;
		debounce = now;
		return (now && !then);
	}
};
Debounce _sullyTouch[CUBE_ALLOCATION];
bool touchEvents[CUBE_ALLOCATION];

TiltShakeRecognizer tsr[2];
int8_t tiltEvents[2];

const Sifteo::AssetImage *characters[] =
{
		&None, &Sully1, &Sully2, &Sully3, &Sully4
};

class SimpleHandler : public Scene::Handler
{
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf1;
	Sifteo::AssetConfiguration<ASSET_CAPACITY> assetconf2;
public:
	SimpleHandler()
	{
		assetconf1.clear();
		assetconf1.append(slot_1, Scroller);

		assetconf2.clear();
		assetconf2.append(slot_2, Character);
	}

	Sifteo::AssetConfiguration<ASSET_CAPACITY> *requestAssets(uint8_t cube, uint8_t mode)
	{
		return (mode==0) ? ((cube!=2) ? &assetconf1 : &assetconf2) : 0;
	}

	bool switchMode(uint8_t cube, uint8_t mode, VideoBuffer &v, CubeID param)
	{
		switch(mode)
		{
		case 0:
			if(cube != 2)
			{
				v.initMode(BG0_SPR_BG1, 16, 96);
				scrollers[0].obj<ScrollingSelector>().clearCache();
				scrollers[1].obj<ScrollingSelector>().clearCache();
			}
			else
			{
				v.initMode(BG0_SPR_BG1, 0, 96);			// 96 pixels at screen top
				v.bg1.fillMask(vec(4,2), vec(8,8) );	// the character right in the middle
			}
			return true;					// attach immediately
		case 1:
			// 16 pixels at top
			v.initMode(FB128, 0, 16);
			break;
		case 2:
			// 16 pixels at bottom
			v.initMode(FB128, 112, 16);
			break;
		case 3:
			// 32 pixels at bottom
			v.initMode(FB128, 96, 32);
			break;
		}
		v.colormap[0] = RGB565::fromRGB(0x000080);
		v.colormap[1] = RGB565::fromRGB(0xFFFFFF);
		return false;
	}

	void drawElement(Scene::Element &el, Sifteo::VideoBuffer &v)
	{
		switch(el.type)
		{
		case 0:
			// the scroll register
			break;
		case 1:
			// text in a 16-pixel area
			Font::drawCentered( v, vec(0,0), vec(128,16), el.obj<TextField>().text);
			break;
		case 2:
			// text in a 32-pixel area
			Font::drawCentered( v, vec(0,0), vec(128,32), el.obj<TextField>().text);
			break;
		case 3:
			// a vertical scrolling level selector, takes some work
			el.obj<ScrollingSelector>().draw(v, scrollRegister->sdata16[0] * scrollRegister->sdata16[1]);
			break;
		case 4:
			// a sprite overlay, type, x, y, hidden
			if(el.data[3])
			{
				v.sprites[el.data[0]].hide();
			}
			else
			{
				v.sprites[el.data[0]].setImage( el.data[0] ? Down : Up);
				v.sprites[el.data[0]].move( el.data[1], el.data[2]);
			}
			break;
		case 5:
			// background of the character area
			v.bg0.image(vec(0,0), Background);
			break;
		case 6:
			// the character
			v.bg1.image(vec(4,2), *characters[el.d8]);
			break;
		case 7:
			// the character's scroll registers
			break;
		}
	}
	int32_t updateElement(Scene::Element &el, uint8_t fc=0)
	{
		switch(el.type)
		{
		case 0:
			// the scroll register
			{
				int16_t next = el.sdata16[0] + fc;
				// whether to fire the halfway event
				if((el.sdata16[0]<scrollHalf) && (next >= scrollHalf))
				{
					scrollers[0].obj<ScrollListener>().onHalfway(scrollers[0], el.sdata16[1]);
					scrollers[1].obj<ScrollListener>().onHalfway(scrollers[1], el.sdata16[1]);
				}
				// whether to fire the completion event (and for the moment automatically start another)
				if((el.sdata16[0]<scrollMax) && (next >= scrollMax))
				{
					scrollers[0].obj<ScrollListener>().onComplete(scrollers[0], el.sdata16[1]);
					scrollers[1].obj<ScrollListener>().onComplete(scrollers[1], el.sdata16[1]);

					next -= scrollMax;

					scrollers[0].obj<ScrollListener>().onBegin(scrollers[0], el.sdata16[1]);
					scrollers[1].obj<ScrollListener>().onBegin(scrollers[1], el.sdata16[1]);
				}

				el.sdata16[0] = next;
				scrollers[0].obj<ScrollListener>().onUpdate(scrollers[0], el.sdata16[0], el.sdata16[1]);
				scrollers[1].obj<ScrollListener>().onUpdate(scrollers[1], el.sdata16[0], el.sdata16[1]);
			}
			break;
		case 1:
			// text in a 16-pixel area
			break;
		case 2:
			// text in a 32-pixel area
			break;
		case 3:
			// a vertical scrolling level selector, takes some work
			break;
		case 4:
			// a sprite overlay, data is type, x, y, hidden
			break;
		case 5:
			// background of the character area
			break;
		case 6:
			// the character
			break;
		case 7:
			// the character's scroll registers
			break;
		}
		return 0;
	}

	void cubeCount(uint8_t cubes) {}
	void neighborAlert() {}

	void attachMotion(uint8_t cube, CubeID param)
	{
		_sullyTouch[cube].setCube(param);
		if(cube<2)
		{
			tsr[cube].attach(param);
		}
	}

	void updateAllMotion(const BitArray<CUBE_ALLOCATION> &map)
	{
		// correctly suppress events from cubes no longer connected
		for(uint8_t i=0; i<CUBE_ALLOCATION; i++)
		{
			touchEvents[i] = (map.test(i)) ? _sullyTouch[i].isTouching() : false;
			if(i<2)
			{
				if(map.test(i))
				{
					tsr[i].update();
					tiltEvents[i] = tsr[i].tilt.y;
				}
				else tiltEvents[i] = 0;
			}
		}
	}
};

void setupSprite(Scene::Element &el, uint8_t type, uint8_t x, uint8_t y, uint8_t hidden)
{
	el.data[0] = type;
	el.data[1] = x;
	el.data[2] = y;
	el.data[3] = hidden;
}

void main()
{
	// initialize scene
	Scene::initialize();

	SimpleHandler sh;

	// use the scene builder API
	Scene::beginScene();

	// entities
	TextField t1_top; 		t1_top.text << "Chapter 1";
	TextField t1_bottom;	t1_bottom.text << "Completed 0/9";
	TextField t2_top;		t2_top.text << "Level 1";
	TextField t2_bottom;	t2_bottom.text << "Best time 00:59.99";
	TextField t3;			t3.text << "Tilt and tap to select";

	// mode 0 body mode 1 16 top mode 2 16 bottom mode 3 32 bottom
	// put mode 0 entries first to do the asset download up front
	ChapterSelector cs;
	LevelSelector ls;

	// 0 scroll register
	scrollRegister = & (Scene::createElement(0).fullUpdate());
	scrollRegister->sdata16[1] = 1;		// set scroll direction

	// 3 level selector
	void *scrollerObjects[] = {&cs, &ls};
	scrollers = Scene::createElement(3, 0).fromTemplate(2, scrollerObjects);

	// 4 sprite
	setupSprite(Scene::createElement(4, 0), 0, 56, 0, 0);
	setupSprite(Scene::createElement(4, 0), 1, 56, 80, 0);
	setupSprite(Scene::createElement(4, 1), 0, 56, 0, 0);
	setupSprite(Scene::createElement(4, 1), 1, 56, 80, 0);

	// 1 16 px text fields
	Scene::createElement(1, 0,1, &t1_top);
	Scene::createElement(1, 0,2, &t1_bottom);
	Scene::createElement(1, 1,1, &t2_top);
	Scene::createElement(1, 1,2, &t2_bottom);

	// 2 32 px text fields
	Scene::createElement(2, 2,3, &t3);

	// 5 character background
	Scene::createElement(5, 2);
	// 6 character
	Scene::createElement(6, 2).d8 = 1;
	// 7 character scroll registers
	Scene::createElement(7, 2);

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("Selection %04x\n", code);
	}
}
