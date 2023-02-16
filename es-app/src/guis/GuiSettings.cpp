#include "guis/GuiSettings.h"

#include "views/ViewController.h"
#include "SystemConf.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiMsgBox.h"
#include "components/SwitchComponent.h"


GuiSettings::GuiSettings(Window* window, const std::string title, bool animate) : GuiComponent(window), mMenu(window, title, true)
{
	addChild(&mMenu);

	mWaitingLoad = false;
	mCloseButton = "start";
	mMenu.addButton(_("BACK"), _("BACK"), [this] { close(); });

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	float x_end = (mSize.x() - mMenu.getSize().x()) / 2,
		  y_end = Renderer::isSmallScreen() ? 0.f : Renderer::getScreenHeight() * 0.15f;

	if (animate)
	{
		float x_start = (Renderer::getScreenWidth() - mMenu.getSize().x()) / 2,
			  y_start = Renderer::getScreenHeight() * 0.5f;
		
		x_end = Renderer::isSmallScreen() ? 0.f : (Renderer::getScreenWidth() - mMenu.getSize().x()) / 2;

		animateTo(Vector2f(x_start, y_start), Vector2f(x_end, y_end));
	}
	else
		mMenu.setPosition(x_end, y_end);
}

GuiSettings::GuiSettings(Window* window, const std::string title, bool animate, const std::string closeButton, const std::function<void()>& closeButtonFunc) : 
GuiSettings(window, title, animate)
{
	mCloseButton = closeButton;
	mCloseButtonFunc = closeButtonFunc;
}

GuiSettings::~GuiSettings()
{

}

void GuiSettings::close()
{
	save();

	if (mOnFinalizeFunc != nullptr)
		mOnFinalizeFunc();

	if (mCloseButtonFunc != nullptr)
		mCloseButtonFunc();

	delete this;
}

void GuiSettings::save()
{
	if (!mDoSave)
		return;

	if (!mSaveFuncs.size())
		return;

	for (auto saveFunction : mSaveFuncs)
		saveFunction();

	Settings::getInstance()->saveFile();
}

bool GuiSettings::input(InputConfig* config, Input input)
{
	if (input.value != 0)
	{
		if (config->isMappedTo(BUTTON_BACK, input))
		{
			if (!mWaitingLoad)
				close();

			return true;
		}
		else if (config->isMappedTo(mCloseButton, input))
		{
			if (!mWaitingLoad)
			{
				if (mCloseButtonFunc != nullptr)
					mCloseButtonFunc();

				// close everything
				Window* window = mWindow;
				while (window->peekGui() && window->peekGui() != ViewController::get())
					delete window->peekGui();
			}
			return true;
		}
	}

	return GuiComponent::input(config, input);
}

HelpStyle GuiSettings::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	auto theme = ViewController::get()->getState().getSystem()->getTheme();

	if (theme != nullptr)
		style.applyTheme(theme, "system");

	return style;
}

std::vector<HelpPrompt> GuiSettings::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt(mCloseButton, _("CLOSE")));

	return prompts;
}

void GuiSettings::addSubMenu(const std::string& label, const std::function<void()>& func)
{
	ComponentListRow row;
	row.makeAcceptInputHandler(func);

	auto theme = ThemeData::getMenuTheme();
	if (theme == nullptr)
		return;

	auto entryMenu = std::make_shared<TextComponent>(mWindow, label, theme->Text.font, theme->Text.color);
	entryMenu->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(entryMenu, true);
	row.addElement(makeArrow(mWindow), false);
	mMenu.addRow(row);
};

void GuiSettings::addInputTextRow(std::string title, const char *settingsID, bool password, bool storeInSettings,
		const std::function<void(Window*, std::string/*title*/, std::string /*value*/,
		const std::function<bool(std::string)>& onsave)>& customEditor,
		const std::function<bool(std::string /*value*/)>& onValidateValue)
{
	auto theme = ThemeData::getMenuTheme();
	if (theme == nullptr)
		return;

	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// LABEL
	Window *window = mWindow;
	ComponentListRow row;

	auto lbl = std::make_shared<TextComponent>(window, title, font, color);
	lbl->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	if (EsLocale::isRTL())
		lbl->setHorizontalAlignment(Alignment::ALIGN_RIGHT);

	row.addElement(lbl, true); // label

	std::string value = storeInSettings ? Settings::getInstance()->getString(settingsID) : SystemConf::getInstance()->get(settingsID);

	std::shared_ptr<TextComponent> ed = std::make_shared<TextComponent>(window, ((password && value != "") ? "*********" : value), font, color, ALIGN_RIGHT);
	ed->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	if (EsLocale::isRTL())
		ed->setHorizontalAlignment(Alignment::ALIGN_LEFT);

	row.addElement(ed, true);

	auto spacer = std::make_shared<GuiComponent>(mWindow);
	spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
	row.addElement(spacer, false);

	auto bracket = std::make_shared<ImageComponent>(mWindow);
	bracket->setImage(theme->Icons.arrow);
	bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));

	if (EsLocale::isRTL())
		bracket->setFlipX(true);

	row.addElement(bracket, false);

	std::function<bool(const std::string /*&newVal*/)> updateVal = [ed, settingsID, password, storeInSettings, window, onValidateValue](const std::string &newVal)
	{
		if ( (onValidateValue != nullptr) && !onValidateValue(newVal))
		{
			window->pushGui(new GuiMsgBox(window, _("THE VALUE \"%s\" ISN'T VALID."), _("OK")));
			return false;
		}

		if (!password)
			ed->setValue(newVal);
		else
			ed->setValue("*********");

		if (storeInSettings)
			Settings::getInstance()->setString(settingsID, newVal);
		else
			SystemConf::getInstance()->set(settingsID, newVal);

		return true;
	}; // ok callback (apply new value to ed)

	row.makeAcceptInputHandler([this, title, updateVal, settingsID, storeInSettings, customEditor]
	{
		std::string data = storeInSettings ? Settings::getInstance()->getString(settingsID) : SystemConf::getInstance()->get(settingsID);

		if (customEditor != nullptr)
			customEditor(mWindow, title, data, updateVal);
		else if (Settings::getInstance()->getBool("UseOSK"))
			mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, data, updateVal, false, nullptr));
		else
			mWindow->pushGui(new GuiTextEditPopup(mWindow, title, data, updateVal, false, nullptr));
	});

	addRow(row);
}

std::shared_ptr<SwitchComponent> GuiSettings::addSwitch(const std::string& title, const std::string& description, const std::string& settingsID, bool storeInSettings, const std::function<void()>& onChanged)
{
	Window* window = mWindow;

	bool value = storeInSettings ? Settings::getInstance()->getBool(settingsID) : SystemConf::getInstance()->getBool(settingsID);

	auto comp = std::make_shared<SwitchComponent>(mWindow, value);

	if (!description.empty())
		addWithDescription(title, description, comp);
	else
		addWithLabel(title, comp);

	std::string localSettingsID = settingsID;
	bool localStoreInSettings = storeInSettings;

	addSaveFunc([comp, localStoreInSettings, localSettingsID, onChanged]
	{
		bool changed = localStoreInSettings ? Settings::getInstance()->setBool(localSettingsID, comp->getState()) : SystemConf::getInstance()->setBool(localSettingsID, comp->getState());
		if (changed && onChanged != nullptr)
			onChanged();
	});

	return comp;
}
