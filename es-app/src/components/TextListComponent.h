#pragma once
#ifndef ES_APP_COMPONENTS_TEXT_LIST_COMPONENT_H
#define ES_APP_COMPONENTS_TEXT_LIST_COMPONENT_H

#include "components/IList.h"
#include "math/Misc.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Sound.h"
#include "Settings.h"
#include <memory>

class TextCache;

struct TextListData
{
	unsigned int colorId;
	std::shared_ptr<TextCache> textCache;
};

//A graphical list. Supports multiple colors for rows and scrolling.
template <typename T>
class TextListComponent : public IList<TextListData, T>
{
protected:
	using IList<TextListData, T>::mEntries;
	using IList<TextListData, T>::listUpdate;
	using IList<TextListData, T>::listInput;
	using IList<TextListData, T>::listRenderTitleOverlay;
	using IList<TextListData, T>::getTransform;
	using IList<TextListData, T>::mSize;
	using IList<TextListData, T>::mCursor;
	using IList<TextListData, T>::Entry;

public:
	using IList<TextListData, T>::size;
	using IList<TextListData, T>::isScrolling;
	using IList<TextListData, T>::stopScrolling;

	TextListComponent(Window* window);
	
	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

	void add(const std::string& name, const T& obj, unsigned int colorId);
	
	enum Alignment
	{
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	inline void setAlignment(Alignment align) { mAlignment = align; }

	inline void setCursorChangedCallback(const std::function<void(CursorState state)>& func) { mCursorChangedCallback = func; }

	inline void setFont(const std::shared_ptr<Font>& font)
	{
		mFont = font;
		for(auto it = mEntries.begin(); it != mEntries.end(); it++)
			it->data.textCache.reset();
	}

	inline void setUppercase(bool /*uppercase*/) 
	{
		mUppercase = true;
		for(auto it = mEntries.begin(); it != mEntries.end(); it++)
			it->data.textCache.reset();
	}

	inline void setSelectorHeight(float selectorScale) { mSelectorHeight = selectorScale; }
	inline void setSelectorOffsetY(float selectorOffsetY) { mSelectorOffsetY = selectorOffsetY; }
	inline void setSelectorColor(unsigned int color) { mSelectorColor = color; }
	inline void setSelectorColorEnd(unsigned int color) { mSelectorColorEnd = color; }
	inline void setSelectorColorGradientHorizontal(bool horizontal) { mSelectorColorGradientHorizontal = horizontal; }
	inline void setSelectedColor(unsigned int color) { mSelectedColor = color; }
	inline void setColor(unsigned int id, unsigned int color) { mColors[id] = color; }
	inline void setLineSpacing(float lineSpacing) { mLineSpacing = lineSpacing; }

protected:
	virtual void onScroll(int /*amt*/) { if(!mScrollSound.empty()) Sound::get(mScrollSound)->play(); }
	virtual void onCursorChanged(const CursorState& state);

private:
	int mMarqueeOffset;
	int mMarqueeOffset2;
	int mMarqueeTime;

	Alignment mAlignment;
	float mHorizontalMargin;

	int getFirstVisibleEntry();
	std::function<void(CursorState state)> mCursorChangedCallback;

	std::shared_ptr<Font> mFont;
	bool mUppercase;
	float mLineSpacing;
	float mSelectorHeight;
	float mSelectorOffsetY;
	unsigned int mSelectorColor;
	unsigned int mSelectorColorEnd;
	bool mSelectorColorGradientHorizontal = true;
	unsigned int mSelectedColor;
	std::string mScrollSound;
	static const unsigned int COLOR_ID_COUNT = 2;
	unsigned int mColors[COLOR_ID_COUNT];
	unsigned int mScreenCount;
	int mStartEntry = 0;
	unsigned int mCursorPrev = -1;
	bool mOneEntryUpDn = true;

	ImageComponent mSelectorImage;
};

template <typename T>
TextListComponent<T>::TextListComponent(Window* window) : 
	IList<TextListData, T>(window), mSelectorImage(window)
{
	mMarqueeOffset = 0;
	mMarqueeOffset2 = 0;
	mMarqueeTime = 0;

	mHorizontalMargin = 0;
	mAlignment = ALIGN_CENTER;

	mFont = Font::get(FONT_SIZE_MEDIUM);
	mUppercase = false;
	mLineSpacing = 1.5f;
	mSelectorHeight = mFont->getSize() * mLineSpacing;
	mSelectorOffsetY = 0;
	mSelectorColor = 0x000000FF;
	mSelectorColorEnd = 0x000000FF;
	mSelectorColorGradientHorizontal = true;
	mSelectedColor = 0;
	mColors[0] = 0x0000FFFF;
	mColors[1] = 0x00FF00FF;
}

template <typename T>
void TextListComponent<T>::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();
	
	std::shared_ptr<Font>& font = mFont;

	if(size() == 0)
		return;

	Vector2f clipPos(trans.translation().x(), trans.translation().y());
	if (!Renderer::isVisibleOnScreen(clipPos.x(), clipPos.y(), mSize.x(), mSize.y()))
		return;

	if (Settings::getInstance()->getBool("DebugGrid"))
	{
		Renderer::setMatrix(trans);
		Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0xFF000033);
		Renderer::setMatrix(parentTrans);
	}


	const float entrySize = Math::max(font->getHeight(1.0), (float)font->getSize()) * mLineSpacing;

	//number of entries that can fit on the screen simultaniously
	mScreenCount = Math::round(mSize.y() / entrySize); // (int)(mSize.y() / entrySize); //  + 0.5f -> avoid partial items
	
	if(mCursor != mCursorPrev)
	{
		mStartEntry = (size() > mScreenCount) ? getFirstVisibleEntry() : 0;
		mCursorPrev = mCursor;
	}

	unsigned int listCutoff = mStartEntry + mScreenCount;
	if(listCutoff > size())
		listCutoff = size();

	float y = (mSize.y() - (mScreenCount * entrySize)) * 0.5f;

	// draw selector bar
	if(mStartEntry < listCutoff)
	{
		if (mSelectorImage.hasImage()) {
			mSelectorImage.setPosition(0.f, y + (mCursor - mStartEntry)*entrySize + mSelectorOffsetY, 0.f);
			mSelectorImage.render(trans);
		} else {
			Renderer::setMatrix(trans);
			Renderer::drawRect(0.0f, y + (mCursor - mStartEntry)*entrySize + mSelectorOffsetY, mSize.x(),
					mSelectorHeight, mSelectorColor, mSelectorColorEnd, mSelectorColorGradientHorizontal);
		}
	}

	// clip to inside margins
	Vector3f dim(mSize.x(), mSize.y(), 0);
	dim = trans * dim - trans.translation();
	Renderer::pushClipRect(Vector2i((int)(trans.translation().x() + mHorizontalMargin), (int)trans.translation().y()), 
		Vector2i((int)(dim.x() - mHorizontalMargin*2), (int)dim.y()));

	for(int i = mStartEntry; i < listCutoff; i++)
	{
		typename IList<TextListData, T>::Entry& entry = mEntries.at((unsigned int)i);

		unsigned int color;
		if(mCursor == i && mSelectedColor)
			color = mSelectedColor;
		else
			color = mColors[entry.data.colorId];

		if(!entry.data.textCache)
			entry.data.textCache = std::unique_ptr<TextCache>(font->buildTextCache(mUppercase ? Utils::String::toUpper(entry.name) : entry.name, 0, 0, 0x000000FF));

		entry.data.textCache->setColor(color);

		Vector3f offset(0, y, 0);

		switch(mAlignment)
		{
		case ALIGN_LEFT:
			offset[0] = mHorizontalMargin;
			break;
		case ALIGN_CENTER:
			offset[0] = (int)((mSize.x() - entry.data.textCache->metrics.size.x()) / 2);
			if(offset[0] < mHorizontalMargin)
				offset[0] = mHorizontalMargin;
			break;
		case ALIGN_RIGHT:
			offset[0] = (mSize.x() - entry.data.textCache->metrics.size.x());
			offset[0] -= mHorizontalMargin;
			if(offset[0] < mHorizontalMargin)
				offset[0] = mHorizontalMargin;
			break;
		}

		// render text
		Transform4x4f drawTrans = trans;

		// currently selected item text might be scrolling
		if((mCursor == i) && (mMarqueeOffset > 0))
			drawTrans.translate(offset - Vector3f((float)mMarqueeOffset, 0, 0));
		else
			drawTrans.translate(offset);

		Renderer::setMatrix(drawTrans);
		font->renderTextCache(entry.data.textCache.get());

		// render currently selected item text again if
		// marquee is scrolled far enough for it to repeat
		if((mCursor == i) && (mMarqueeOffset2 < 0))
		{
			drawTrans = trans;
			drawTrans.translate(offset - Vector3f((float)mMarqueeOffset2, 0, 0));
			Renderer::setMatrix(drawTrans);
			font->renderTextCache(entry.data.textCache.get());
		}

		y += entrySize;
	}

	Renderer::popClipRect();

	listRenderTitleOverlay(trans);

	GuiComponent::renderChildren(trans);
}


template <typename T>
int TextListComponent<T>::getFirstVisibleEntry()
{
	if (mCursorPrev == -1)
	{
		// init or returned from emulator
		mCursorPrev = mCursor;
		int quot = div(mCursor, mScreenCount).quot;
		mStartEntry = quot * mScreenCount;
	}
	int screenRelCursor = mCursorPrev - mStartEntry;
	bool cursorCentered = screenRelCursor == mScreenCount/2;
	int visibleEntryMax = size() - mScreenCount;
	int firstVisibleEntry = 0;

	if(Settings::getInstance()->getBool("UseFullscreenPaging") && !cursorCentered)
	{
		// keep visible cursor constant but move visible list (default)
		firstVisibleEntry = mCursor - screenRelCursor;
		if(mOneEntryUpDn)
		{
			int delta = mCursor - mCursorPrev;
			// detect rollover (== delta is more than one item)
			if(delta < -3)
				firstVisibleEntry = 0;
			else if(delta > 3)
				firstVisibleEntry = visibleEntryMax;
			else if(screenRelCursor < mScreenCount/2 && delta > 0 /*down pressed*/
				|| screenRelCursor > mScreenCount/2 && delta < 0 /*up pressed*/)
				// cases for list begin / list end
				// move visible cursor and keep visible list section constant
				firstVisibleEntry = firstVisibleEntry - delta;
		}
	} else {
		// cursor always in middle of visible list
		firstVisibleEntry = mCursor - mScreenCount/2;
	}
	// bounds check
	if(firstVisibleEntry < 0)
		firstVisibleEntry = 0;
	else if(firstVisibleEntry > visibleEntryMax)
		firstVisibleEntry = visibleEntryMax;
	return firstVisibleEntry;
}

template <typename T>
bool TextListComponent<T>::input(InputConfig* config, Input input)
{
	if(size() > 0)
	{
		if(input.value != 0)
		{
			if(config->isMappedLike("down", input))
			{
				listInput(1);
				mOneEntryUpDn = true;
				return true;
			}

			if(config->isMappedLike("up", input))
			{
				listInput(-1);
				mOneEntryUpDn = true;
				return true;
			}
			if(config->isMappedLike(BUTTON_PD, input))
			{
				int delta = Settings::getInstance()->getBool("UseFullscreenPaging") ? mScreenCount : 10;
				listInput(delta);
				mOneEntryUpDn = false;
				return true;
			}

			if(config->isMappedLike(BUTTON_PU, input))
			{
				int delta = Settings::getInstance()->getBool("UseFullscreenPaging") ? mScreenCount : 10;
				listInput(-delta);
				mOneEntryUpDn = false;
				return true;
			}
		}else{
			if(config->isMappedLike("down", input) || config->isMappedLike("up", input) || 
				config->isMappedLike(BUTTON_PD, input) || config->isMappedLike(BUTTON_PU, input))
			{
				stopScrolling();
			}
		}
	}

	return GuiComponent::input(config, input);
}

template <typename T>
void TextListComponent<T>::update(int deltaTime)
{
	listUpdate(deltaTime);

	if(!isScrolling() && size() > 0)
	{
		// always reset the marquee offsets
		mMarqueeOffset  = 0;
		mMarqueeOffset2 = 0;

		// if we're not scrolling and this object's text goes outside our size, marquee it!
		const float textLength = mFont->sizeText(mEntries.at((unsigned int)mCursor).name).x();
		const float limit      = mSize.x() - mHorizontalMargin * 2;

		if(textLength > limit)
		{
			// loop
			// pixels per second ( based on nes-mini font at 1920x1080 to produce a speed of 200 )
			const float speed        = mFont->sizeText("ABCDEFGHIJKLMNOPQRSTUVWXYZ").x() * 0.247f;
			const float delay        = 3000;
			const float scrollLength = textLength;
			const float returnLength = speed * 1.5f;
			const float scrollTime   = (scrollLength * 1000) / speed;
			const float returnTime   = (returnLength * 1000) / speed;
			const int   maxTime      = (int)(delay + scrollTime + returnTime);

			mMarqueeTime += deltaTime;
			while(mMarqueeTime > maxTime)
				mMarqueeTime -= maxTime;

			mMarqueeOffset = (int)(Math::Scroll::loop(delay, scrollTime + returnTime, (float)mMarqueeTime, scrollLength + returnLength));

			if(mMarqueeOffset > (scrollLength - (limit - returnLength)))
				mMarqueeOffset2 = (int)(mMarqueeOffset - (scrollLength + returnLength));
		}
	}

	GuiComponent::update(deltaTime);
}

//list management stuff
template <typename T>
void TextListComponent<T>::add(const std::string& name, const T& obj, unsigned int color)
{
	assert(color < COLOR_ID_COUNT);

	typename IList<TextListData, T>::Entry entry;
	entry.name = name;
	entry.object = obj;
	entry.data.colorId = color;
	static_cast<IList< TextListData, T >*>(this)->add(entry);
}

template <typename T>
void TextListComponent<T>::onCursorChanged(const CursorState& state)
{
	mMarqueeOffset = 0;
	mMarqueeOffset2 = 0;
	mMarqueeTime = 0;

	if(mCursorChangedCallback)
		mCursorChangedCallback(state);
}

template <typename T>
void TextListComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	GuiComponent::applyTheme(theme, view, element, properties);

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "textlist");
	if(!elem)
		return;

	using namespace ThemeFlags;
	if(properties & COLOR)
	{
		if(elem->has("selectorColor"))
		{
			setSelectorColor(elem->get<unsigned int>("selectorColor"));
			setSelectorColorEnd(elem->get<unsigned int>("selectorColor"));
		}
		if (elem->has("selectorColorEnd"))
			setSelectorColorEnd(elem->get<unsigned int>("selectorColorEnd"));
		if (elem->has("selectorGradientType"))
			setSelectorColorGradientHorizontal(!(elem->get<std::string>("selectorGradientType").compare("horizontal")));
		if(elem->has("selectedColor"))
			setSelectedColor(elem->get<unsigned int>("selectedColor"));
		if(elem->has("primaryColor"))
			setColor(0, elem->get<unsigned int>("primaryColor"));
		if(elem->has("secondaryColor"))
			setColor(1, elem->get<unsigned int>("secondaryColor"));
	}

	setFont(Font::getFromTheme(elem, properties, mFont));
	const float selectorHeight = Math::max(mFont->getHeight(1.0), (float)mFont->getSize()) * mLineSpacing;
	setSelectorHeight(selectorHeight);

	if(properties & SOUND && elem->has("scrollSound"))
		mScrollSound = elem->get<std::string>("scrollSound");

	if(properties & ALIGNMENT)
	{
		if(elem->has("alignment"))
		{
			const std::string& str = elem->get<std::string>("alignment");
			if(str == "left")
				setAlignment(ALIGN_LEFT);
			else if(str == "center")
				setAlignment(ALIGN_CENTER);
			else if(str == "right")
				setAlignment(ALIGN_RIGHT);
			else
				LOG(LogError) << "TextListComponent<T>::applyTheme() - ERROR: Unknown TextListComponent alignment \"" << str << "\"!";
		}
		if(elem->has("horizontalMargin"))
		{
			mHorizontalMargin = elem->get<float>("horizontalMargin") * (this->mParent ? this->mParent->getSize().x() : (float)Renderer::getScreenWidth());
		}
	}

	if(properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
		setUppercase(elem->get<bool>("forceUppercase"));

	if(properties & LINE_SPACING)
	{
		if(elem->has("lineSpacing"))
			setLineSpacing(elem->get<float>("lineSpacing"));
		if(elem->has("selectorHeight"))
		{
			setSelectorHeight(elem->get<float>("selectorHeight") * Renderer::getScreenHeight());
		}
		if(elem->has("selectorOffsetY"))
		{
			float scale = this->mParent ? this->mParent->getSize().y() : (float)Renderer::getScreenHeight();
			setSelectorOffsetY(elem->get<float>("selectorOffsetY") * scale);
		} else {
			setSelectorOffsetY(0.0);
		}
	}

	if (elem->has("selectorImagePath"))
	{
		std::string path = elem->get<std::string>("selectorImagePath");
		bool tile = elem->has("selectorImageTile") && elem->get<bool>("selectorImageTile");
		mSelectorImage.setImage(path, tile);
		mSelectorImage.setSize(mSize.x(), mSelectorHeight);
		mSelectorImage.setColorShift(mSelectorColor);
		mSelectorImage.setColorShiftEnd(mSelectorColorEnd);
	} else {
		mSelectorImage.setImage("");
	}
}

#endif // ES_APP_COMPONENTS_TEXT_LIST_COMPONENT_H
