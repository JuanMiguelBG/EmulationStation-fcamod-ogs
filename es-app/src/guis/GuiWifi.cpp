#include "guis/GuiWifi.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "Settings.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiLoading.h"


GuiWifi::GuiWifi(Window* window, const std::string title, const std::string subtitle, std::string data, const std::function<bool(std::string)>& onsave)
	: GuiComponent(window), mMenu(window, title.c_str())
{
	mTitle = title;
	mInitialData = data;
	mSaveFunction = onsave;
	mWaitingLoad = false;
	mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	std::vector<std::string> ssids = ApiSystem::getInstance()->getWifiNetworks();
	if (ssids.empty())
		mWindow->postToUiThread([this]() { onRefresh(); });
	else
		load(ssids);

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onRefresh(); });
	mMenu.addButton(_("MANUAL INPUT"), _("MANUAL INPUT"), [&] { onManualInput(); });
	mMenu.addButton(_("BACK"), _("BACK"), [&] { delete this; });

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiWifi::load(std::vector<std::string> ssids)
{
	mMenu.clear();

	if (ssids.size() == 0)
		mMenu.addEntry(_("NO WIFI NETWORKS FOUND"), false, std::bind(&GuiWifi::onRefresh, this));
	else
	{
		for (auto ssid : ssids)
			mMenu.addEntry(ssid, false, [this, ssid]() { GuiWifi::onSave(ssid); });
	}

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	mWaitingLoad = false;
}

void GuiWifi::onManualInput()
{
	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, mTitle, mInitialData, [this](const std::string& value) { return onSave(value); }, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, mTitle, mInitialData, [this](const std::string& value) { return onSave(value); }, false));
}

bool GuiWifi::onSave(const std::string& value)
{
	if (mWaitingLoad)
		return false;

	std::string rep_value = Utils::String::replace(value, SystemConf::getInstance()->get("already.connection.exist.flag"), "");
	if (mSaveFunction(rep_value))
	{
		delete this;
		return true;
	}
	return false;
}

bool GuiWifi::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;

	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		if (!mWaitingLoad)
			delete this;

		return true;
	}
	else if (input.value != 0 && config->isMappedTo("x", input))
	{
		onRefresh();
		return true;
	}
	else if (input.value != 0 && config->isMappedTo("y", input))
	{
		onManualInput();
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiWifi::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));
	prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECTIONNER")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("y", _("MANUAL INPUT")));
	return prompts;
}

void GuiWifi::onRefresh()
{
	Window* window = mWindow;

	mWindow->pushGui(new GuiLoading<std::vector<std::string>>(mWindow, _("SEARCHING WIFI NETWORKS..."), 
		[this, window]
		{
			mWaitingLoad = true;
			return ApiSystem::getInstance()->getWifiNetworks(true);
		},
		[this, window](std::vector<std::string> ssids)
		{
			mWaitingLoad = false;
			load(ssids);
		}));
}
