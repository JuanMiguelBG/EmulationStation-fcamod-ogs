#include "guis/GuiScreensaverOptions.h"

#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiFileBrowser.h"
#include "views/ViewController.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"

GuiScreensaverOptions::GuiScreensaverOptions(Window* window, std::string title) : GuiComponent(window), mMenu(window, title)
{
	addChild(&mMenu);

	mMenu.addButton(_("BACK"), _("BACK"), [this] { delete this; });

	// resize
	bool change_height = Renderer::isSmallScreen() && Settings::getInstance()->getBool("ShowHelpPrompts");
	float height_ratio = 1.0f;

	if ( change_height )
		height_ratio = 0.95f;

	setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight() * height_ratio);

	// center
	float new_x = (mSize.x() - mMenu.getSize().x()) / 2,
				new_y = (mSize.y() - mMenu.getSize().y()) / 2;

	if ( change_height )
		new_y = 0.f;

	mMenu.setPosition(new_x, new_y);
}

GuiScreensaverOptions::~GuiScreensaverOptions()
{
	save();
}

void GuiScreensaverOptions::save()
{
	if(!mSaveFuncs.size())
		return;

	for(auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); it++)
		(*it)();

	Settings::getInstance()->saveFile();
}

bool GuiScreensaverOptions::input(InputConfig* config, Input input)
{
	if(config->isMappedTo(BUTTON_BACK, input) && input.value != 0)
	{
		delete this;
		return true;
	}

	if(config->isMappedTo("start", input) && input.value != 0)
	{
		// close everything
		Window* window = mWindow;
		while(window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();
		return true;
	}

	return GuiComponent::input(config, input);
}

HelpStyle GuiScreensaverOptions::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	return style;
}

std::vector<HelpPrompt> GuiScreensaverOptions::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));

	return prompts;
}

std::shared_ptr<TextComponent> GuiScreensaverOptions::addEditableTextComponent(const std::string label, std::string value)
{
	auto theme = ThemeData::getMenuTheme();

	ComponentListRow row;

	auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), theme->Text.font, theme->Text.color);
	row.addElement(lbl, true); // label

	std::shared_ptr<TextComponent> ed = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->TextSmall.color);
	row.addElement(ed, true);

	auto spacer = std::make_shared<GuiComponent>(mWindow);
	spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
	row.addElement(spacer, false);

	auto bracket = std::make_shared<ImageComponent>(mWindow);
	bracket->setImage(":/arrow.svg");
	bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
	row.addElement(bracket, false);

	auto updateVal = [ed](const std::string& newVal) { ed->setValue(newVal); return true; }; // ok callback (apply new value to ed)
	row.makeAcceptInputHandler([this, label, ed, updateVal] {
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, label, ed->getValue(), updateVal, false));
	});

	ed->setValue(value);

	addRow(row);
	return ed;
}

std::shared_ptr<TextComponent> GuiScreensaverOptions::addBrowsablePath(const std::string label, std::string value)
{
	auto theme = ThemeData::getMenuTheme();

	ComponentListRow row;

	auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), theme->Text.font, theme->Text.color);
	row.addElement(lbl, true); // label

	std::shared_ptr<TextComponent> ed = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->TextSmall.color);
	row.addElement(ed, true);

	auto spacer = std::make_shared<GuiComponent>(mWindow);
	spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
	row.addElement(spacer, false);

	auto bracket = std::make_shared<ImageComponent>(mWindow);
	bracket->setImage(ThemeData::getMenuTheme()->Icons.arrow);
	bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
	row.addElement(bracket, false);

	auto updateVal = [ed](const std::string& newVal)
	{
		ed->setValue(newVal);
	}; // ok callback (apply new value to ed)

	row.makeAcceptInputHandler([this, label, ed, updateVal]
	{
		auto parent = Utils::FileSystem::getParent(ed->getValue());
		mWindow->pushGui(new GuiFileBrowser(mWindow, parent, ed->getValue(), GuiFileBrowser::DIRECTORY, updateVal, label));
	});

	ed->setValue(value);

	addRow(row);
	return ed;
}
