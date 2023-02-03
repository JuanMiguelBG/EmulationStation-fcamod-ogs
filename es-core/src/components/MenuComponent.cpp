#include "components/MenuComponent.h"

#include "components/ButtonComponent.h"
#include "components/MultiLineMenuEntry.h"
#include "components/HelpComponent.h"
#include "Settings.h"
#include "resources/Font.h"
#include "Window.h"
#include "Log.h"

#define BUTTON_GRID_VERT_PADDING  (Renderer::getScreenHeight()*0.0296296) //32
#define BUTTON_GRID_HORIZ_PADDING (Renderer::getScreenWidth()*0.0052083333) //10

#define TITLE_HEIGHT (mTitle->getFont()->getLetterHeight() + (mSubtitle ? TITLE_WITHSUB_VERT_PADDING : TITLE_VERT_PADDING) + (mSubtitle ? mSubtitle->getSize().y() + SUBTITLE_VERT_PADDING : 0))

MenuComponent::MenuComponent(Window* window, 
		const std::string title, bool computeHelpComponentSize) : MenuComponent(window, title, Font::get(FONT_SIZE_LARGE), "", computeHelpComponentSize) {}

MenuComponent::MenuComponent(Window* window, 
	const std::string title, const std::shared_ptr<Font>& titleFont,
	const std::string subTitle, bool computeHelpComponentSize) 
	: GuiComponent(window),
	mBackground(window), mGrid(window, Vector2i(1, 3)), mComputeHelpComponentSize(computeHelpComponentSize)
{
	mMaxHeight = 0;

	auto theme = ThemeData::getMenuTheme();

	addChild(&mBackground);
	addChild(&mGrid);
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	// set up title
	mTitle = std::make_shared<TextComponent>(mWindow);
	mTitle->setHorizontalAlignment(ALIGN_CENTER);
	mTitle->setColor(theme->Title.color); // 0x555555FF

	if (theme->Title.selectorColor != 0x555555FF)
	{
		mTitle->setBackgroundColor(theme->Title.selectorColor);
		mTitle->setRenderBackground(true);
	}

	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 2));
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 0), false, true);

	setTitle(title, theme->Title.font); //  titleFont
	setSubTitle(subTitle);

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);
	//mGrid.setEntry(mTitle, Vector2i(0, 0), false);

	// set up list which will never change (externally, anyway)
	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 1), true);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool {
		if (config->isMappedLike("down", input)) {
			mGrid.setCursorTo(mList);
			mList->setCursorIndex(0);
			return true;
		}
		if (config->isMappedLike("up", input)) {
			mList->setCursorIndex(mList->size() - 1);
			if (mButtons.size()) {
				mGrid.moveCursor(Vector2i(0, 1));
			}
			else {
				mGrid.setCursorTo(mList);
			}
			return true;
		}
		return false;
	});

	updateGrid();
	updateSize();

	mGrid.resetCursor();
}

void MenuComponent::addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp, const std::string iconName, bool setCursorHere, bool invert_when_selected)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;
	
	ComponentListRow row;

	if (!iconName.empty())
	{
		std::string iconPath = theme->getMenuIcon(iconName);
		if (!iconPath.empty())
		{
			// icon
			auto icon = std::make_shared<ImageComponent>(mWindow);
			icon->setImage(iconPath);
			icon->setColorShift(color);
			icon->setResize(0, font->getLetterHeight() * 1.25f);
			row.addElement(icon, false);

			// spacer between icon and text
			auto spacer = std::make_shared<GuiComponent>(mWindow);
			spacer->setSize(10, 0);
			row.addElement(spacer, false);
		}
	}

	auto text_comp = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), font, color);
	text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(text_comp, true);
	row.addElement(comp, false, invert_when_selected);
	addRow(row, setCursorHere);
}

void MenuComponent::addWithDescription(const std::string& label, const std::string& description, const std::shared_ptr<GuiComponent>& comp, const std::function<void()>& func, const std::string iconName, bool setCursorHere, bool invert_when_selected, bool multiLine)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	ComponentListRow row;

	if (!iconName.empty())
	{
		std::string iconPath = theme->getMenuIcon(iconName);
		if (!iconPath.empty())
		{
			// icon
			auto icon = std::make_shared<ImageComponent>(mWindow, true);
			icon->setImage(iconPath);
			icon->setColorShift(color);
			icon->setResize(0, font->getLetterHeight() * 1.25f);
			row.addElement(icon, false);

			// spacer between icon and text
			auto spacer = std::make_shared<GuiComponent>(mWindow);
			spacer->setSize(10, 0);
			row.addElement(spacer, false);
		}
	}

	if (!description.empty())
		row.addElement(std::make_shared<MultiLineMenuEntry>(mWindow, Utils::String::toUpper(label), description, multiLine), true);
	else
	{
		auto text_comp = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), font, color);
		text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
		row.addElement(text_comp, true);
	}

	if (comp != nullptr)
		row.addElement(comp, false, invert_when_selected);

	if (func != nullptr)
		row.makeAcceptInputHandler(func);

	addRow(row, setCursorHere);
}

void MenuComponent::addEntry(const std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName, bool setCursorHere, bool invert_when_selected, bool onButtonRelease, const std::string userData, bool doUpdateSize)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// populate the list
	ComponentListRow row;

	if (!iconName.empty())
	{
		std::string iconPath = theme->getMenuIcon(iconName);
		if (!iconPath.empty())
		{
			// icon
			auto icon = std::make_shared<ImageComponent>(mWindow);
			icon->setImage(iconPath);
			icon->setColorShift(color);
			icon->setResize(0, font->getLetterHeight() * 1.25f);
			row.addElement(icon, false);

			// spacer between icon and text
			auto spacer = std::make_shared<GuiComponent>(mWindow);
			spacer->setSize(10, 0);
			row.addElement(spacer, false);
		}
	}

	auto text_comp = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(name), font, color);
	text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(text_comp, true, invert_when_selected);

	if (add_arrow)
		row.addElement(makeArrow(mWindow), false);

	row.makeAcceptInputHandler(func, onButtonRelease);

	addRow(row, setCursorHere, doUpdateSize, userData);
}

void MenuComponent::setTitle(const std::string title, const std::shared_ptr<Font>& font)
{
	mTitle->setText(Utils::String::toUpper(title));
	
	if (font != nullptr)
		mTitle->setFont(font);
}

void MenuComponent::setSubTitle(const std::string text)
{
	if (text.empty())
	{
		if (mSubtitle != nullptr)
		{
			mHeaderGrid->removeEntry(mSubtitle);
			mSubtitle = nullptr;			
		}

		mHeaderGrid->setRowHeightPerc(0, 1);
		mHeaderGrid->setRowHeightPerc(1, 0);

		return;
	}
	
	if (mSubtitle == nullptr)
	{
		auto theme = ThemeData::getMenuTheme();

		mSubtitle = std::make_shared<TextComponent>(mWindow, 
			Utils::String::toUpper(Utils::FileSystem::getFileName(text)),
			theme->TextSmall.font, theme->TextSmall.color, ALIGN_CENTER);

		mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 1), false, true);
	}
	
	mSubtitle->setText(Utils::String::toUpper(text));
	mSubtitle->setVerticalAlignment(Alignment::ALIGN_TOP);
	mSubtitle->setSize(Renderer::getScreenWidth() * 0.88f, 0);
	mSubtitle->setLineSpacing(1.1);

	updateSize();

	const float titleHeight = mTitle->getFont()->getLetterHeight() + (mSubtitle ? TITLE_WITHSUB_VERT_PADDING : TITLE_VERT_PADDING);
	const float subtitleHeight = mSubtitle->getSize().y() + SUBTITLE_VERT_PADDING;

	mHeaderGrid->setRowHeightPerc(0, titleHeight / TITLE_HEIGHT);
}

float MenuComponent::getButtonGridHeight() const
{
	auto menuTheme = ThemeData::getMenuTheme();

	return (mButtonGrid ? mButtonGrid->getSize().y() : menuTheme->Text.font->getHeight() + BUTTON_GRID_VERT_PADDING);
}
/*
void MenuComponent::setPosition(float x, float y, float z)
{
	float new_y = y;
	if (Renderer::isSmallScreen() && Settings::getInstance()->getBool("ShowHelpPrompts"))
		new_y = 0.f;

	GuiComponent::setPosition(x, new_y, z);
}
*/
void MenuComponent::updateSize()
{
	if (Renderer::isSmallScreen() || !Settings::getInstance()->getBool("CenterMenus"))
	{
		LOG(LogDebug) << "MenuComponent::updateSize() - is small screen or not center menus.";
		Log::flush();
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
		return;
	}

	LOG(LogDebug) << "MenuComponent::updateSize() - is big screen and center menus.";
	Log::flush();

	// !Renderer::isSmallScreen() && Settings::getInstance()->getBool("CenterMenus")
	float width, height;
	const float maxHeight = mMaxHeight <= 0 ? Renderer::getScreenHeight() * 0.75f : mMaxHeight;

	height = TITLE_HEIGHT + mList->getTotalRowHeight() + getButtonGridHeight() + 2;
	if (height > maxHeight)
	{
		height = TITLE_HEIGHT + getButtonGridHeight();
		int i = 0;
		while(i < mList->size())
		{
			float rowHeight = mList->getRowHeight(i);
			if(height + rowHeight < maxHeight)
				height += rowHeight;
			else
				break;
			i++;
		}
	}

	width = Renderer::getScreenWidth() * 0.90f;
	if (Settings::getInstance()->getBool("AutoMenuWidth"))
	{
		float font_size = ThemeData::getMenuTheme()->Text.font->getSize(),
			  ratio = 1.2f;

		width = (float)Math::min((int)width, Renderer::getScreenWidth());

		if ((font_size >= FONT_SIZE_SMALL) && (font_size < FONT_SIZE_MEDIUM))
			ratio = 1.4f;
		else if ((font_size >= FONT_SIZE_MEDIUM) && (font_size < FONT_SIZE_LARGE))
			ratio = 1.7f;
		else if ((font_size >= FONT_SIZE_LARGE))
			ratio = 2.0f;

		width = width * ratio;
	}

	height = (float)Math::min((int)height, Renderer::getScreenHeight());
	width = (float)Math::min((int)width, Renderer::getScreenWidth());
	setSize(width, height);
}

void MenuComponent::onSizeChanged()
{
	LOG(LogDebug) << "MenuComponent::onSizeChanged()";
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	// update grid row/col sizes
	mGrid.setRowHeightPerc(0, TITLE_HEIGHT / mSize.y());
	mGrid.setRowHeightPerc(2, getButtonGridHeight() / mSize.y());

	mGrid.setSize(mSize);
}

void MenuComponent::clearButtons()
{
	mButtons.clear();
	updateGrid();
	updateSize();
}

void MenuComponent::addButton(const std::string& name, const std::string& helpText, const std::function<void()>& callback)
{
	mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, Utils::String::toUpper(name), helpText, callback));
	updateGrid();
	updateSize();
}

void MenuComponent::updateGrid()
{
	if (mButtonGrid)
		mGrid.removeEntry(mButtonGrid);

	mButtonGrid.reset();

	if (mButtons.size())
	{
		mButtonGrid = makeButtonGrid(mWindow, mButtons, isComputeHelpComponentSize());
		mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false);
	}
}

std::vector<HelpPrompt> MenuComponent::getHelpPrompts()
{
	return mGrid.getHelpPrompts();
}

float getHelpComponentHeight(Window* window, bool addHelpComponnetHeight)
{
	return addHelpComponnetHeight ? window->getHelpComponentHeight() : 0.f;
}

std::shared_ptr<ComponentGrid> makeMultiDimButtonGrid(Window* window, const std::vector< std::vector< std::shared_ptr<ButtonComponent> > >& buttons, float outerWidth)
{

	const int sizeX = (int)buttons.at(0).size();
	const int sizeY = (int)buttons.size();
	const float buttonHeight = buttons.at(0).at(0)->getSize().y();
	const float gridHeight = (buttonHeight + BUTTON_GRID_VERT_PADDING + 2) * sizeY;

	float horizPadding = (float)BUTTON_GRID_HORIZ_PADDING;
	float gridWidth, buttonWidth;

//	do {
		gridWidth = outerWidth - horizPadding; // to get centered because size * (button size + BUTTON_GRID_VERT_PADDING) let a half BUTTON_GRID_VERT_PADDING left / right marge
		buttonWidth = (gridWidth / sizeX) - horizPadding;
	//	horizPadding -= 2;
//	} while ((buttonWidth < 100) && (horizPadding > 2));

	std::shared_ptr<ComponentGrid> grid = std::make_shared<ComponentGrid>(window, Vector2i(sizeX, sizeY));

	grid->setSize(gridWidth, gridHeight);

	for (int x = 0; x < sizeX; x++)
		grid->setColWidthPerc(x, (float)1 / sizeX);

	for (int y = 0; y < sizeY; y++)
	{
		for (int x = 0; x < sizeX; x++)
		{
			const std::shared_ptr<ButtonComponent>& button = buttons.at(y).at(x);
			button->setSize(buttonWidth, buttonHeight);
			grid->setEntry(button, Vector2i(x, y), true, false);
		}
	}

	return grid;
}

std::shared_ptr<ComponentGrid> makeButtonGrid(Window* window, const std::vector< std::shared_ptr<ButtonComponent> >& buttons, bool addHelpComponentHeight)
{
	std::shared_ptr<ComponentGrid> buttonGrid = std::make_shared<ComponentGrid>(window, Vector2i((int)buttons.size(), 2));

	float buttonGridWidth = (float)BUTTON_GRID_HORIZ_PADDING * buttons.size(); // initialize to padding
	for(int i = 0; i < (int)buttons.size(); i++)
	{
		buttonGrid->setEntry(buttons.at(i), Vector2i(i, 0), true, false);
		buttonGridWidth += buttons.at(i)->getSize().x();
	}
	for(unsigned int i = 0; i < buttons.size(); i++)
	{
		buttonGrid->setColWidthPerc(i, (buttons.at(i)->getSize().x() + BUTTON_GRID_HORIZ_PADDING) / buttonGridWidth);
	}

	float helpComponentHeight = getHelpComponentHeight(window, addHelpComponentHeight);
	LOG(LogDebug) << "MenuComponent::makeButtonGrid() - help component height: " << std::to_string(helpComponentHeight) << ", addHelpComponentHeight: " << Utils::String::boolToString(addHelpComponentHeight);
	Log::flush();
	buttonGrid->setSize(buttonGridWidth, buttons.at(0)->getSize().y() + BUTTON_GRID_VERT_PADDING + (helpComponentHeight > 0 ? helpComponentHeight : 2));
	// spacer row to deal with dropshadow to make buttons look centered
	buttonGrid->setRowHeightPerc(1, helpComponentHeight > 0.f ? 0.4f : (2 / buttonGrid->getSize().y()) );

	return buttonGrid;
}

std::shared_ptr<ImageComponent> makeArrow(Window* window)
{
	auto menuTheme = ThemeData::getMenuTheme();

	auto bracket = std::make_shared<ImageComponent>(window);
	bracket->setImage(ThemeData::getMenuTheme()->Icons.arrow); // ":/arrow.svg");
	bracket->setColorShift(menuTheme->Text.color);
	bracket->setResize(0, round(menuTheme->Text.font->getLetterHeight()));
	return bracket;
}
