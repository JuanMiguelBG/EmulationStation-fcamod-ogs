#include "components/HelpComponent.h"

#include "components/ComponentGrid.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "resources/TextureResource.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Settings.h"

#define OFFSET_X 12 // move the entire thing right by this amount (px)
#define OFFSET_Y 12 // move the entire thing up by this amount (px)

#define ICON_TEXT_SPACING 6 // space between [icon] and [text] (px)
#define ENTRY_SPACING 14 // space between [text] and next [icon] (px)

static const std::map<std::string, const char*> ICON_PATH_MAP {
	{ "up/down",            ":/help/dpad_updown_gt.svg" },
	{ "left/right",         ":/help/dpad_leftright_gt.svg" },
	{ "up/down/left/right", ":/help/dpad_all_gt.svg" },
	{ "a",                  ":/help/button_a_gt.svg" },
	{ "b",                  ":/help/button_b_gt.svg" },
	{ "x",                  ":/help/button_x_gt.svg" },
	{ "y",                  ":/help/button_y_gt.svg" },
	{ "l",                  ":/help/button_l_gt.svg" },
	{ "l1",                 ":/help/button_l_gt.svg" },
	{ "r",                  ":/help/button_r_gt.svg" },
	{ "r1",                 ":/help/button_r_gt.svg" },
	{ "lr",                 ":/help/button_lr_gt.svg" },
	{ "lt",                 ":/help/button_lt_gt.svg" },
	{ "l2",                 ":/help/button_lt_gt.svg" },
	{ "rt",                 ":/help/button_rt_gt.svg" },
	{ "r2",                 ":/help/button_rt_gt.svg" },
	{ "start",              ":/help/button_start_gt.svg" },
	{ "select",             ":/help/button_select_gt.svg" }
};

HelpComponent::HelpComponent(Window* window) : GuiComponent(window)
{
}

void HelpComponent::clearPrompts()
{
	mPrompts.clear();
	updateGrid();
}

void HelpComponent::setPrompts(const std::vector<HelpPrompt>& prompts)
{
	mPrompts = prompts;
	updateGrid();
}

void HelpComponent::setStyle(const HelpStyle& style)
{
	mStyle = style;
	updateGrid();
}

void HelpComponent::updateGrid()
{
	if (!Settings::getInstance()->getBool("ShowHelpPrompts") || mPrompts.empty())
	{
		mGrid.reset();
		setVisible(false);
		return;
	}

	std::shared_ptr<Font>& font = mStyle.font;

	mGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i((int)mPrompts.size() * 4, 1));
	// [icon] [spacer1] [text] [spacer2]

	std::vector< std::shared_ptr<ImageComponent> > icons;
	std::vector< std::shared_ptr<TextComponent> > labels;

	float width = 0;
	const float height = Math::round(font->getLetterHeight() * 1.25f);
	for (auto it = mPrompts.cbegin(); it != mPrompts.cend(); it++)
	{
		auto icon = std::make_shared<ImageComponent>(mWindow);

		if (mStyle.iconMap.find(it->first) != mStyle.iconMap.end() && Utils::FileSystem::exists(mStyle.iconMap[it->first]))
			icon->setImage(mStyle.iconMap[it->first]);
		else
			icon->setImage(getIconTexture(it->first.c_str()));

		icon->setColorShift(mStyle.iconColor);
		icon->setResize(0, height);
		icons.push_back(icon);

		auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(it->second), font, mStyle.textColor);
		labels.push_back(lbl);

		width += icon->getSize().x() + lbl->getSize().x() + ICON_TEXT_SPACING + ENTRY_SPACING;
	}

	mGrid->setSize(width, height);
	for (unsigned int i = 0; i < icons.size(); i++)
	{
		const int col = i*4;
		mGrid->setColWidthPerc(col, icons.at(i)->getSize().x() / width);
		mGrid->setColWidthPerc(col + 1, ICON_TEXT_SPACING / width);
		mGrid->setColWidthPerc(col + 2, labels.at(i)->getSize().x() / width);

		mGrid->setEntry(icons.at(i), Vector2i(col, 0), false, false);
		mGrid->setEntry(labels.at(i), Vector2i(col + 2, 0), false, false);
	}

	mGrid->setPosition(Vector3f(mStyle.position.x(), mStyle.position.y(), 0.0f));
	//mGrid->setPosition(OFFSET_X, Renderer::getScreenHeight() - mGrid->getSize().y() - OFFSET_Y);
	mGrid->setOrigin(mStyle.origin);

	setVisible(true);
}

std::shared_ptr<TextureResource> HelpComponent::getIconTexture(const char* name)
{
	auto it = mIconCache.find(name);
	if (it != mIconCache.cend())
		return it->second;

	auto pathLookup = ICON_PATH_MAP.find(name);
	if (pathLookup == ICON_PATH_MAP.cend())
	{
		LOG(LogError) << "HelpComponent::getIconTexture():119 - Unknown help icon \"" << name << "\"!";
		return nullptr;
	}
	if (!ResourceManager::getInstance()->fileExists(pathLookup->second))
	{
		LOG(LogError) << "HelpComponent::getIconTexture():124 - Help icon \"" << name << "\" - corresponding image file \"" << pathLookup->second << "\" misisng!";
		return nullptr;
	}

	std::shared_ptr<TextureResource> tex = TextureResource::get(pathLookup->second);
	mIconCache[std::string(name)] = tex;
	return tex;
}

void HelpComponent::setOpacity(unsigned char opacity)
{
	GuiComponent::setOpacity(opacity);

	for (unsigned int i = 0; i < mGrid->getChildCount(); i++)
	{
		mGrid->getChild(i)->setOpacity(opacity);
	}
}

void HelpComponent::render(const Transform4x4f& parentTrans)
{
	if (!isVisible() )
		return;

	Transform4x4f trans = parentTrans * getTransform();

	if(mGrid)
		mGrid->render(trans);
}

float HelpComponent::getHeight()
{
	if (isVisible() && mGrid)
	{
		return mGrid->getSize().y();
	}
	return 0.f;
}
