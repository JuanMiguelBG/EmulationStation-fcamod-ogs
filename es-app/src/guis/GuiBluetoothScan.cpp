#include "guis/GuiBluetoothScan.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiLoading.h"
#include "SystemConf.h"
#include "Log.h"


GuiBluetoothScan::GuiBluetoothScan(Window* window, const std::string title, const std::string subtitle)
	: GuiComponent(window), mMenu(window, title.c_str(), true)
{
	ApiSystem::getInstance()->startBluetoothLiveScan();

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

GuiBluetoothScan::~GuiBluetoothScan()
{
	ApiSystem::getInstance()->stopBluetoothLiveScan();
}

void GuiBluetoothScan::load(std::vector<BluetoothDevice> btDevices)
{
	hasDevices = false;
	mMenu.clear();

	if (btDevices.size() == 0)
		mMenu.addEntry(_("NO BLUETOOTH DEVICES FOUND"), false, std::bind(&GuiBluetoothScan::onScan, this));
	else
	{
		hasDevices = true;
		for (auto btDevice : btDevices)
			mMenu.addWithDescription(btDevice.name, btDevice.id, [this, btDevice]() { GuiBluetoothScan::onConnectDevice(btDevice); }, btDevice.type);
	}

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	updateHelpPrompts();
	mWaitingLoad = false;
}

void GuiBluetoothScan::handleAlias(Window *window, bool deleteWindow, bool restarES, const BluetoothDevice& btDevice)
{
	std::string title = _("ADD ALIAS");
	
	std::function<bool(const std::string /*&newVal*/)> updateVal = [this, window, deleteWindow, restarES, btDevice](const std::string &newVal)
	{
		if (ApiSystem::getInstance()->setBluetoothDeviceAlias(btDevice.id, newVal))
		{
			if (Utils::String::startsWith(btDevice.type, "input-"))
			{
				Settings::getInstance()->setString(btDevice.name + ".bluetooth.input_gaming.alias", newVal);
				Settings::getInstance()->setString(btDevice.name + ".bluetooth.input_gaming.id", btDevice.id);
				Settings::getInstance()->saveFile();
			}

			displayRestartDialog(window, deleteWindow, restarES);
			return true;
		}

		return false;
	}; // ok callback (apply new value to ed)

	std::function<void(const std::string /*&newVal*/)> cancelVal = [this, window, deleteWindow, restarES](const std::string &newVal)
	{	
		displayRestartDialog(window, deleteWindow, restarES);
	}; // cancel callback (apply new value to ed)

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, btDevice.alias, updateVal, false, cancelVal));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, title, btDevice.alias, updateVal, false, cancelVal));
}

void GuiBluetoothScan::displayRestartDialog(Window *window, bool deleteWindow, bool restarES)
{
	// checking if a reset audio configuration is needed
	if (restarES)
	{
		ApiSystem::getInstance()->stopBluetoothLiveScan();

		std::string msg = _("THE AUDIO INTERFACE HAS CHANGED.") + "\n" + _("THE EMULATIONSTATION WILL NOW RESTART.");
		window->pushGui(new GuiMsgBox(window, msg,
			_("OK"),
			[]
			{
				Utils::FileSystem::createFile(Utils::FileSystem::getEsConfigPath() + "/skip_auto_connect_bt_audio_device_onboot.lock");
				if (quitES(QuitMode::RESTART) != 0)
					LOG(LogWarning) << "GuiBluetoothScan::displayRestartDialog() - Restart terminated with non-zero result!";
			}));
	}
	else
	{
		if (deleteWindow)
			onClose();
		else
			onScan();
	}
}

bool GuiBluetoothScan::onConnectDevice(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	Window* window = mWindow;

	std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
	std:: string msg(_("CONNECTING BLUETOOTH DEVICE"));
	window->pushGui(new GuiLoading<bool>(window, msg.append(" '").append(btDevice.name).append("'..."), 
		[this, btDevice]
		{
			mWaitingLoad = true;

			bool result = ApiSystem::getInstance()->pairBluetoothDevice(btDevice.id);

			// fail paired
			if (!result)
				return false;

			result = ApiSystem::getInstance()->connectBluetoothDevice(btDevice.id);
			// successfully connected
			if (result)
			{
				// BT 4.2, only one audio device
				// reload BT audio device info
				std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
				SystemConf::getInstance()->setBool("bluetooth.audio.connected", !new_audio_device.empty());
				SystemConf::getInstance()->set("bluetooth.audio.device", new_audio_device);
			}

			return result;
		},
		[this, window, btDevice, audio_device](bool result)
		{
			bool restar_ES = false;
            std::string message;

			mWaitingLoad = false;
			if (result)
 			{
				message.append("'").append(btDevice.name).append("' ").append(_("DEVICE SUCCESSFULLY PAIRED AND CONNECTED"));

				// audio bluetooth connecting changes
				restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );

				window->pushGui(new GuiMsgBox(window, message,
					_("OK"),
						[this, window, result, restar_ES]
						{
							displayRestartDialog(window, result, restar_ES);
						},
					_("ADD ALIAS"),
						[this, window, result, restar_ES, btDevice]
						{
							handleAlias(window, result, restar_ES, btDevice);
						})
				);
			}
			else
			{
				message.append("'").append(btDevice.name).append("' ").append(_("DEVICE FAILED TO PAIR AND CONNECT"));

				window->pushGui(new GuiMsgBox(window, message,
					_("OK"),
						[this, window]
						{
							displayRestartDialog(window, false, false);
						})
				);
			}
		}));

	return true;
}

bool GuiBluetoothScan::input(InputConfig* config, Input input)
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

std::vector<HelpPrompt> GuiBluetoothScan::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));
	if (hasDevices)
   		prompts.push_back(HelpPrompt(BUTTON_OK, _("CONNECT")));
	else
   		prompts.push_back(HelpPrompt(BUTTON_OK, _("REFRESH")));

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	return prompts;
}

void GuiBluetoothScan::onScan()
{
	Window* window = mWindow;

	window->pushGui(new GuiLoading<std::vector<BluetoothDevice>>(window, _("SEARCHING BLUETOOTH DEVICES..."), 
		[this]
		{
			mWaitingLoad = true;
			return ApiSystem::getInstance()->getBluetoothNewDevices();
		},
		[this](std::vector<BluetoothDevice> btDevices)
		{
			load(btDevices);
			mWaitingLoad = false;
		}));
}

void GuiBluetoothScan::onClose()
{
	delete this;
}
