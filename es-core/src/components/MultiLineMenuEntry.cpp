#include "MultiLineMenuEntry.h"
#include "Window.h"
#include "components/TextComponent.h"
#include "components/ComponentGrid.h"
#include "math/Vector2i.h"
#include "math/Vector2f.h"
#include "ThemeData.h"

MultiLineMenuEntry::MultiLineMenuEntry(Window* window, const std::string& text, const std::string& substring, bool multiLine) :
	ComponentGrid(window, Vector2i(1, 2))
{
	auto theme = ThemeData::getMenuTheme();

	mSubText = std::make_shared<TextComponent>(mWindow, substring.c_str(), theme->Text.font, theme->Text.color);
	init(text, multiLine, true);	
}

MultiLineMenuEntry::MultiLineMenuEntry(Window* window, const std::string& text, const std::shared_ptr<TextComponent>& comp_substring, bool multiLine) :
	ComponentGrid(window, Vector2i(1, 2))
{
	mSubText = comp_substring;
	init(text, multiLine, false);
}

void MultiLineMenuEntry::init(const std::string& text, bool multiLine, bool updateSubText)
{
	mMultiLine = multiLine;
	mSizeChanging = false;

	auto theme = ThemeData::getMenuTheme();

	mText = std::make_shared<TextComponent>(mWindow, text.c_str(), theme->Text.font, theme->Text.color);
	mText->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));

	mText->setVerticalAlignment(ALIGN_TOP);

	if (updateSubText)
	{
		mSubText->setFont(theme->TextSmall.font);
		mSubText->setColor(theme->Text.color);
	}
	mSubText->setVerticalAlignment(ALIGN_TOP);
	mSubText->setOpacity(192);

	if (Settings::getInstance()->getBool("AutoscrollMenuEntries"))
	{
		mText->setAutoScroll(true);
		mSubText->setAutoScroll(true);
	}

	setEntry(mText, Vector2i(0, 0), true, true);
	setEntry(mSubText, Vector2i(0, 1), false, true);

	float th = mText->getSize().y();
	float sh = mSubText->getSize().y();
	float h = th + sh;

	setRowHeightPerc(0, (th * 0.9) / h);
	setRowHeightPerc(1, (sh * 1.1) / h);

	setSize(Vector2f(0, h));
}

void MultiLineMenuEntry::setColor(unsigned int color)
{
	mText->setColor(color);
	mSubText->setColor(color);
}

void MultiLineMenuEntry::onSizeChanged()
{		
	ComponentGrid::onSizeChanged();

	if (mMultiLine && mSubText && mSize.x() > 0 && !mSizeChanging)
	{
		mSizeChanging = true;

		mText->setSize(mSize.x(), 0);
		mSubText->setSize(mSize.x(), 0);

		float th = mText->getSize().y();
		float sh = mSubText->getSize().y();
		float h = th + sh;

		setRowHeightPerc(0, (th * 0.9) / h);
		setRowHeightPerc(1, (sh * 1.1) / h);

		setSize(Vector2f(mSize.x(), h));

		mSizeChanging = false;
	}
}
