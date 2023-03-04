#include "guis/GuiKnowedWifis.h"

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
#include "utils/StringUtil.h"


GuiKnowedWifis::GuiKnowedWifis(Window* window, const std::string title, const std::string subtitle, const std::function<void(bool)>& closeFunction)
	: GuiComponent(window), mMenu(window, title.c_str(), true)
{
	mTitle = title;

	mConnectedWifiSsid = "";
	mForgetConnectedWifi = false;
	mCloseFunction = closeFunction;
	mWaitingLoad = false;
	hasDevices = false;
	mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	mWindow->postToUiThread([this]() { onRefresh(); });
}

void GuiKnowedWifis::load(std::vector<std::string> ssids)
{
	hasDevices = false;
	mMenu.clear();
	mMenu.clearButtons();

	if (ssids.size() == 0)
		mMenu.addEntry(_("NO WIFI NETWORKS FOUND"), false, std::bind(&GuiKnowedWifis::onRefresh, this));
	else
	{
		hasDevices = true;
		for (auto ssid : ssids)
		{
			if (Utils::String::endsWith(ssid, SystemConf::getInstance()->get("already.connection.exist.flag")))
				mConnectedWifiSsid = Utils::String::replace(ssid, SystemConf::getInstance()->get("already.connection.exist.flag"), "");

			mMenu.addEntry(ssid, [this, ssid]() { GuiKnowedWifis::onForget(ssid); }, ssid);
		}
	}

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onRefresh(); });
	if (hasDevices)
		mMenu.addButton(_("FORGET ALL"), _("FORGET ALL"), [&] { onForgetAll(); });

	mMenu.addButton(_("BACK"), _("BACK"), [&] { onClose(); });

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);

	updateHelpPrompts();
	mWaitingLoad = false;
}

bool GuiKnowedWifis::onForget(const std::string& ssid)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO FORGET THE WIFI NETWORK?"),
		_("YES"), [this, window, ssid]
		{
			std::string msg = (_("FORGETING WIFI NETWORK"));
			msg.append(" '").append(ssid).append("' ...");
			
			window->pushGui(new GuiLoading<bool>(window, msg, 
				[this, ssid]
				{
					mWaitingLoad = true;
					return ApiSystem::getInstance()->forgetWifiNetwork(Utils::String::replace(ssid, SystemConf::getInstance()->get("already.connection.exist.flag"), ""));
				},
				[this, window, ssid](bool result)
				{
					std::string msg;
					if (result)
					{
						msg.append("'").append(ssid).append("' ").append(_("WIFI NETWORK SUCCESSFULLY FORGOTTEN"));
						if (Utils::String::endsWith(ssid, SystemConf::getInstance()->get("already.connection.exist.flag")))
							mForgetConnectedWifi = true;
					}
					else
						msg.append("'").append(ssid).append("' ").append(_("WIFI NETWORK FAILED TO FORGOTTEN"));

					mWaitingLoad = false;
					window->pushGui(new GuiMsgBox(window, msg, _("OK"), [this] { GuiKnowedWifis::onRefresh(); }));
				}));
		},
		_("NO"), nullptr));

	return true;
}

bool GuiKnowedWifis::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;

	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		if (!mWaitingLoad)
			onClose();

		return true;
	}
	else if (input.value != 0 && config->isMappedTo("x", input))
	{
		onRefresh();
		return true;
	}
	else if (input.value != 0 && config->isMappedTo("y", input))
	{
		onForgetAll();
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiKnowedWifis::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));

	if (hasDevices)
	{
		std::string selected = mMenu.getSelected();
		if (!selected.empty())
		{  // Wifi network
			if (Utils::String::endsWith(selected, SystemConf::getInstance()->get("already.connection.exist.flag")))
				prompts.push_back(HelpPrompt(BUTTON_OK, _("DISCONNECT & FORGET")));
			else
				prompts.push_back(HelpPrompt(BUTTON_OK, _("FORGET")));
		}

		prompts.push_back(HelpPrompt("y", _("FORGET ALL")));
	}
	else
		prompts.push_back(HelpPrompt(BUTTON_OK, _("REFRESH")));

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	return prompts;
}

void GuiKnowedWifis::onRefresh()
{
	Window* window = mWindow;

	mWindow->pushGui(new GuiLoading<std::vector<std::string>>(mWindow, _("SEARCHING KNOWED WIFI NETWORKS..."), 
		[this, window]
		{
			mWaitingLoad = true;
			return ApiSystem::getInstance()->getKnowedWifiNetworks();
		},
		[this, window](std::vector<std::string> ssids)
		{
			mWaitingLoad = false;
			load(ssids);
		}));
}

void GuiKnowedWifis::onClose()
{
	if (mCloseFunction)
		mCloseFunction(mForgetConnectedWifi);

	delete this;
}

void GuiKnowedWifis::onForgetAll()
{
	if (mWaitingLoad || !hasDevices)
		return;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO FORGET ALL WIFI NETWORKS?"),
		_("YES"), [this, window]
		{
			mWindow->pushGui(new GuiLoading<bool>(window, _("PLEASE WAIT..."),
				[this, window]()
				{
					mWaitingLoad = true;

					bool result = ApiSystem::getInstance()->forgetAllKnowedWifiNetworks();
					SystemConf::getInstance()->set("wifi.ssid", ApiSystem::getInstance()->getWifiSsid());

					return result;
				},
				[this, window](bool result)
				{
					std::string msg;

					if (result)
						msg = "ALL WIFI NETWORKS HAVE BEEN FORGOTTEN.";
					else
						msg = "NOT ALL WIFI NETWORKS HAVE BEEN FORGOTTEN.";

					if (!mConnectedWifiSsid.empty() && (mConnectedWifiSsid != SystemConf::getInstance()->get("wifi.ssid")))
						mForgetConnectedWifi = true;

					mWaitingLoad = false;
					window->pushGui(new GuiMsgBox(window, _(msg)));

					GuiKnowedWifis::onRefresh();
				}));
		},
		_("NO"), nullptr));
}
