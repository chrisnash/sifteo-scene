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

const uint16_t levels[] =
{
		0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x000,
		0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x000, 0x000,
		0x301, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
		0x401, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
		0x501, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
		0x601, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
		0x701, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
};
uint16_t chapterCount = 7;
uint16_t chapterWidth = 8;

const uint16_t startPoint = 0x0101;

class SelectorLogic
{
	static uint16_t findIndex(uint16_t current)
	{
		for(uint16_t i=0; i<arraysize(levels); i++)
		{
			if(levels[i] == current) return i;
		}
		return 0;
	}
	static uint16_t indexOf(uint16_t cIndex, uint16_t lIndex)
	{
		return cIndex*chapterWidth + lIndex;
	}
public:
	static uint16_t previousChapter(uint16_t current)
	{
		uint16_t index = findIndex(current);
		uint16_t cIndex = index/chapterWidth; uint16_t lIndex = index%chapterWidth;

		do
		{
			if(cIndex==0) cIndex=chapterCount;
			cIndex--; lIndex=0;
		}
		while(levels[indexOf(cIndex,lIndex)]==0);

		return levels[indexOf(cIndex, lIndex)];
	}
	static uint16_t nextChapter(uint16_t current)
	{
		uint16_t index = findIndex(current);
		uint16_t cIndex = index/chapterWidth; uint16_t lIndex = index%chapterWidth;

		do
		{
			cIndex++; lIndex=0;
			if(cIndex==chapterCount) cIndex=0;
		}
		while(levels[indexOf(cIndex,lIndex)]==0);

		return levels[indexOf(cIndex, lIndex)];
	}

	static uint16_t previousLevel(uint16_t current)
	{
		uint16_t index = findIndex(current);
		uint16_t cIndex = index/chapterWidth; uint16_t lIndex = index%chapterWidth;

		do
		{
			if(lIndex==0) lIndex = chapterWidth;
			lIndex--;
		}
		while(levels[indexOf(cIndex,lIndex)]==0);
		return levels[indexOf(cIndex, lIndex)];
	}
	static uint16_t nextLevel(uint16_t current)
	{
		uint16_t index = findIndex(current);
		uint16_t cIndex = index/chapterWidth; uint16_t lIndex = index%chapterWidth;

		do
		{
			lIndex++;
			if(lIndex==chapterWidth) lIndex = 0;
		}
		while(levels[indexOf(cIndex,lIndex)]==0);
		return levels[indexOf(cIndex, lIndex)];
	}
};

Sifteo::BitArray<3> currentListeners;

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

	void setNextItem(uint16_t item)
	{
		if(itemBuffer[2] != item)
		{
			// if any of item 2 is in the visible area, you need to mark it as needing a redraw
			if(windowEnd > windowSize) windowEnd = windowSize;
			if(windowStart > windowEnd) windowStart = windowEnd = 0;
			itemBuffer[2] = item;
		}
	}
	void setPreviousItem(uint16_t item)
	{
		if(itemBuffer[0] != item)
		{
			// if any of item 0 is in the render window, you need to refresh it
			if(windowStart < 0) windowStart = 0;
			if(windowStart > windowEnd) windowStart = windowEnd = 0;
			itemBuffer[0] = item;
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

	void onHalfway(Scene::Element &el, int16_t direction)
	{
		// once halfway, change the current image and the windowing parameters
		if(direction==1)
		{
			itemBuffer[0] = itemBuffer[1];
			itemBuffer[1] = itemBuffer[2];
			itemBuffer[2] = 0x0000;
		}
		else
		{
			itemBuffer[2] = itemBuffer[1];
			itemBuffer[1] = itemBuffer[0];
			itemBuffer[0] = 0x0000;
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
};

class CharacterAnimator : public ScrollListener
{
	void onBegin(Scene::Element &el, int16_t direction)
	{
	}
	void onUpdate(Scene::Element &el, int16_t position, int16_t direction)
	{
	}
	void onHalfway(Scene::Element &el, int16_t direction)
	{
	}
	void onComplete(Scene::Element &el, int16_t direction)
	{
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
			if(el.cube!=2)
			{
				el.obj<ScrollingSelector>().draw(v, scrollRegister->sdata16[0] * scrollRegister->sdata16[1]);
			}
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
			if(el.sdata16[1])
			{
				int16_t next = el.sdata16[0] + fc;
				// whether to fire the halfway event
				if((el.sdata16[0]<scrollHalf) && (next >= scrollHalf))
				{
					for(uint8_t l : currentListeners)  scrollers[l].obj<ScrollListener>().onHalfway(scrollers[l], el.sdata16[1]);
				}
				// whether to fire the completion event (and for the moment automatically start another)
				if((el.sdata16[0]<scrollMax) && (next >= scrollMax))
				{
					el.sdata16[0] = scrollMax;
					for(uint8_t l : currentListeners) scrollers[l].obj<ScrollListener>().onUpdate(scrollers[l], el.sdata16[0], el.sdata16[1]);;
					el.sdata16[0] = 0;
					for(uint8_t l : currentListeners) scrollers[l].obj<ScrollListener>().onComplete(scrollers[l], el.sdata16[1]);
					el.sdata16[1] = 0;	// and stop again
					currentListeners.clear();
				}
				else
				{
					el.sdata16[0] = next;
					for(uint8_t l : currentListeners) scrollers[l].obj<ScrollListener>().onUpdate(scrollers[l], el.sdata16[0], el.sdata16[1]);;
				}
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
			if(scrollRegister->sdata16[1]==0)	// only if not currently scrolling
			{
				uint16_t current = scrollers[1].obj<ScrollingSelector>().itemBuffer[1];
				if(el.cube!=2)
				{
					// not scrolling, some action selected on the cube
					if(tiltEvents[el.cube])
					{
						currentListeners.clear();
						scrollRegister->sdata16[0] = 0;						// start with zero offset
						scrollRegister->sdata16[1] = -tiltEvents[el.cube];	// scroll in the other direction
						// start a scroll in this window
						switch(el.cube)
						{
						case 0:
							// change chapter
							currentListeners.mark(0); currentListeners.mark(1); currentListeners.mark(2);
							current = (tiltEvents[el.cube] < 0) ? SelectorLogic::nextChapter(current) : SelectorLogic::previousChapter(current);
							if(tiltEvents[el.cube] < 0)
							{
								scrollers[0].obj<ScrollingSelector>().setNextItem(current & 0xFF00);
								scrollers[1].obj<ScrollingSelector>().setNextItem(current);
							}
							else
							{
								scrollers[0].obj<ScrollingSelector>().setPreviousItem(current & 0xFF00);
								scrollers[1].obj<ScrollingSelector>().setPreviousItem(current);
							}
							break;
						case 1:
							// change level
							currentListeners.mark(1);	// just one pane listening
							current = (tiltEvents[el.cube] < 0) ? SelectorLogic::nextLevel(current) : SelectorLogic::previousLevel(current);
							if(tiltEvents[el.cube] < 0)
							{
								scrollers[1].obj<ScrollingSelector>().setNextItem(current);
							}
							else
							{
								scrollers[1].obj<ScrollingSelector>().setPreviousItem(current);
							}
							break;
						}
						// notify everyone they've started
						for(uint8_t l : currentListeners) scrollers[l].obj<ScrollListener>().onBegin(scrollers[l], el.sdata16[1]);
					}
				}
				else
				{
					if(touchEvents[el.cube]) return current;
				}
			}
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
					// tell this scroller it needs to update
					if(tiltEvents[i])
					{
						scrollers[i].setUpdate();
					}
				}
				else tiltEvents[i] = 0;
			}
			else if(i==2)
			{
				if(touchEvents[i]) scrollers[i].setUpdate();	// tell the scroller on cube 3 it's time to update
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
	SCRIPT(LUA, System():setAssetLoaderBypass(true));

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
	CharacterAnimator ca;

	// 0 scroll register
	scrollRegister = & (Scene::createElement(0).fullUpdate());
	scrollRegister->sdata16[1] = 0;		// set scroll direction (not scrolling)
	currentListeners.clear();			// nobody's listening

	// 3 level selector
	void *scrollerObjects[] = {&cs, &ls, &ca};
	scrollers = Scene::createElement(3, 0).fromTemplate(3, scrollerObjects);

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

	cs.itemBuffer[1] = startPoint & 0xFF00;
	ls.itemBuffer[1] = startPoint;

	while(1)
	{
		int32_t code = Scene::execute(sh);			// this call never returns because we have no update methods.
		LOG("Selection %04x\n", code);
	}
}
