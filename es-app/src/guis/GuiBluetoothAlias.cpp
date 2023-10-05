#include "guis/GuiBluetoothAlias.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiLoading.h"
#include "SystemConf.h"


GuiBluetoothAlias::GuiBluetoothAlias(Window* window, const std::string title, const std::string subtitle)
	: GuiComponent(window), mMenu(window, title.c_str(), true)
{
	mTitle = title;
	mWaitingLoad = false;
	hasDevices = false;
	mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	mWindow->postToUiThread([this]() { onScan(); });

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onScan(); });
	mMenu.addButton(_("BACK"), _("BACK"), [&] { onClose(); });
}

void GuiBluetoothAlias::load(std::vector<BluetoothDevice> btDevices)
{
	hasDevices = false;
	mMenu.clear();

	if (btDevices.size() == 0)
		mMenu.addEntry(_("NO BLUETOOTH DEVICES FOUND"), false, std::bind(&GuiBluetoothAlias::onScan, this));
	else
	{
		auto theme = ThemeData::getMenuTheme();

		hasDevices = true;
		for (auto btDevice : btDevices)
		{
			auto alias_text = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(btDevice.alias), theme->Text.font, theme->Text.color);
			alias_text->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
			mMenu.addWithDescription(btDevice.name, btDevice.id, alias_text, [this, btDevice]() { GuiBluetoothAlias::onManageDeviceAlias(btDevice); }, btDevice.type, false, true, false, btDevice.alias);
		}
	}

	mMenu.updateSize();

//	if (Renderer::isSmallScreen())
//		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	updateHelpPrompts();
	mWaitingLoad = false;
}

bool GuiBluetoothAlias::onManageDeviceAlias(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	Window* window = mWindow;
	
	std::string title = btDevice.alias.empty() ? _("ADD ALIAS") : _("EDIT ALIAS");
	
	std::function<bool(const std::string /*&newVal*/)> updateVal = [this, window, btDevice](const std::string &newVal)
	{	
		if (ApiSystem::getInstance()->setBluetoothDeviceAlias(btDevice.id, newVal))
		{
			if (Utils::String::startsWith(btDevice.type, "input-"))
			{
				Settings::getInstance()->setString(btDevice.name + ".bluetooth.input_gaming.alias", newVal);
				Settings::getInstance()->setString(btDevice.name + ".bluetooth.input_gaming.id", btDevice.id);
				Settings::getInstance()->saveFile();
			}
			onScan();
			return true;
		}
		return false;
	}; // ok callback (apply new value to ed)

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, btDevice.alias, updateVal, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, title, btDevice.alias, updateVal, false));

	return true;
}

bool GuiBluetoothAlias::input(InputConfig* config, Input input)
{
	if (mWaitingLoad || GuiComponent::input(config, input))
		return true;

	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		onClose();
		return true;
	}
	else if (input.value != 0 && config->isMappedTo("x", input))
	{
		onScan();
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiBluetoothAlias::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));

	if (hasDevices)
	{
		if (!mMenu.getSelected().empty())
			prompts.push_back(HelpPrompt(BUTTON_OK, _("EDIT ALIAS")));
		else
			prompts.push_back(HelpPrompt(BUTTON_OK, _("ADD ALIAS")));
	}
	else
		prompts.push_back(HelpPrompt(BUTTON_OK, _("REFRESH")));

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	return prompts;
}

void GuiBluetoothAlias::onScan()
{
	Window* window = mWindow;

	window->pushGui(new GuiLoading<std::vector<BluetoothDevice>>(window, _("SEARCHING BLUETOOTH PAIRED DEVICES..."), 
		[this]
		{
			mWaitingLoad = true;
			return ApiSystem::getInstance()->getBluetoothPairedDevices();
		},
		[this](std::vector<BluetoothDevice> btDevices)
		{
			load(btDevices);
			mWaitingLoad = false;
		}));
}

void GuiBluetoothAlias::onClose()
{
	delete this;
}
