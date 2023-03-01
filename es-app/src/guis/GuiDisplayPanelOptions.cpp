#include "guis/GuiDisplayPanelOptions.h"

#include "components/ButtonComponent.h"
#include "components/ComponentList.h"
#include "components/MenuComponent.h"
#include "components/TextComponent.h"
#include "components/SliderComponent.h"
#include "components/ButtonComponent.h"
#include "components/ImageComponent.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "Window.h"
#include "ApiSystem.h"


#define BUTTON_GRID_VERT_PADDING  (Renderer::getScreenHeight()*0.0296296) //32

GuiDisplayPanelOptions::GuiDisplayPanelOptions(Window* window) : GuiComponent(window),
	mBackground(window, ":/frame.png"),
	mGrid(window, Vector2i(1, 4))
{
    mNeedResetValues = false;

	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	addChild(&mBackground);
	addChild(&mGrid);

	// Header Grid
	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 1));
	mTitle = std::make_shared<TextComponent>(mWindow, _("PANEL SETTINGS"), theme->Title.font, theme->Title.color, ALIGN_CENTER);
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 0), false, true);
   	mHeaderGrid->setRowHeightPerc(0, 1);
	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

	// Options list
	mList = std::make_shared<ComponentList>(mWindow);
	mGrid.setEntry(mList, Vector2i(0, 1), true, true);

    // Panel Gamma
	mGamma = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
	mGamma->setValue((float) ApiSystem::getInstance()->getGammaLevel());
	mGamma->setOnValueChanged([](const float &newVal)
	{
		ApiSystem::getInstance()->setGammaLevel((int)Math::round(newVal));
	});
	addWithLabel(_("GAMMA"), mGamma);

	//Panel Contrast
	mContrast = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
	mContrast->setValue((float) ApiSystem::getInstance()->getContrastLevel());
	mContrast->setOnValueChanged([](const float &newVal)
	{
		ApiSystem::getInstance()->setContrastLevel((int)Math::round(newVal));
	});
	addWithLabel(_("CONTRAST"), mContrast);

	//Panel Saturation
	mSaturation = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
	mSaturation->setValue((float) ApiSystem::getInstance()->getSaturationLevel());
	mSaturation->setOnValueChanged([](const float &newVal)
	{
		ApiSystem::getInstance()->setSaturationLevel((int)Math::round(newVal));
	});
	addWithLabel(_("SATURATION"), mSaturation);

	//Panel Hue
	mHue = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
	mHue->setValue((float) ApiSystem::getInstance()->getHueLevel());	
	mHue->setOnValueChanged([](const float &newVal)
	{
		ApiSystem::getInstance()->setHueLevel((int)Math::round(newVal));
	});
	addWithLabel(_("HUE"), mHue);

	addEntry(_("DEFAULT VALUES").c_str(), [this] { resetDisplayPanelSettings(); });


	// Image Grid
	mImagesGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, 1));

	auto imageGame = std::make_shared<ImageComponent>(mWindow);
	imageGame->setImage(ResourceManager::getInstance()->getResourcePath(":/panel/game.jpg"));

	auto imageBars = std::make_shared<ImageComponent>(mWindow);
	imageBars->setImage(ResourceManager::getInstance()->getResourcePath(":/panel/bars.jpg"));

	mImagesGrid->setEntry(imageGame, Vector2i(0, 0), false, true);
	mImagesGrid->setEntry(imageBars, Vector2i(1, 0), false, true);
    mImagesGrid->setColWidthPerc(0, 0.5f);
    mImagesGrid->setColWidthPerc(1, 0.5f);

	//imageGame->setResize(Vector2f(0, mImagesGrid->getRowHeight(0)));
	//imageBars->setResize(Vector2f(0, mImagesGrid->getRowHeight(0)));

	mGrid.setEntry(mImagesGrid, Vector2i(0, 2), true, true);


	// Buttons Grid
	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("BACK"), _("BACK"), [&] { delete this; }));
	mButtonGrid = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 3), true, false);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool {
		if (config->isMappedLike("down", input)) {
			mGrid.setCursorTo(mList);
			mList->setCursorIndex(0);
			return true;
		}
		if (config->isMappedLike("up", input)) {
			mList->setCursorIndex(mList->size() - 1);
			mGrid.moveCursor(Vector2i(0, 1));
			return true;
		}
		return false;
	});

	// resize & position
	float width_ratio = 0.95f,
		  height_ratio = 0.85f,
		  width = Renderer::getScreenWidth(),
		  height = Renderer::getScreenHeight(),
		  new_x = 0.f,
		  new_y = 0.f;

	if (Renderer::isSmallScreen() || !Settings::getInstance()->getBool("CenterMenus"))
	{
		width_ratio = 1.0f;
		height_ratio = 1.0f;
	}
	setSize(width * width_ratio, height * height_ratio);

	if (!Renderer::isSmallScreen() && Settings::getInstance()->getBool("CenterMenus"))
	{
		new_x = (Renderer::getScreenWidth() - mSize.x()) / 2,  // center
		new_y = (Renderer::getScreenHeight() - mSize.y()) / 2; // center
	}
	setPosition(new_x, new_y);
}

float GuiDisplayPanelOptions::getButtonGridHeight() const
{
	auto menuTheme = ThemeData::getMenuTheme();

	return (mButtonGrid ? mButtonGrid->getSize().y() : menuTheme->Text.font->getHeight() + BUTTON_GRID_VERT_PADDING);
}

void GuiDisplayPanelOptions::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mGrid.setSize(mSize);

	const float titleHeight = mTitle->getFont()->getLetterHeight();

	mGrid.setRowHeightPerc(0, (titleHeight + TITLE_VERT_PADDING) / mSize.y());
    mGrid.setRowHeightPerc(3, getButtonGridHeight() / mSize.y());
}

bool GuiDisplayPanelOptions::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;


	if ((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		auto pthis = this;
		if (pthis)
			delete pthis;
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiDisplayPanelOptions::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}

void GuiDisplayPanelOptions::updateSize()
{
	if (Renderer::isSmallScreen() || !Settings::getInstance()->getBool("CenterMenus"))
	{
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
		return;
	}
}

void GuiDisplayPanelOptions::addRow(const ComponentListRow& row, bool setCursorHere, bool doUpdateSize)
{
	mList->addRow(row, setCursorHere, true, "");
	if (doUpdateSize)
		updateSize();
}

void GuiDisplayPanelOptions::addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp, bool setCursorHere, bool invert_when_selected)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;
	
	ComponentListRow row;

	auto text_comp = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), font, color);
	text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(text_comp, true);
	row.addElement(comp, false, invert_when_selected);
	addRow(row, setCursorHere);
}

void GuiDisplayPanelOptions::addEntry(const std::string name, const std::function<void()>& func, bool setCursorHere, bool invert_when_selected, bool doUpdateSize)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// populate the list
	ComponentListRow row;

	auto text_comp = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(name), font, color);
	text_comp->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(text_comp, true, invert_when_selected);

	row.makeAcceptInputHandler(func);

	addRow(row, setCursorHere, doUpdateSize);
}

void GuiDisplayPanelOptions::resetDisplayPanelSettings()
{
	ApiSystem::getInstance()->resetDisplayPanelSettings();
    mNeedResetValues = true;
}

void GuiDisplayPanelOptions::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

    if (mNeedResetValues)
    {
	    mGamma->setValue(50.f);
	    mContrast->setValue(50.f);
	    mSaturation->setValue(50.f);
	    mHue->setValue(50.f);

        mNeedResetValues = false;
    }
}
