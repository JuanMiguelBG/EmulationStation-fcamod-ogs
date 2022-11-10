#include "guis/GuiBluetooth.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "Settings.h"
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiLoading.h"
#include "SystemConf.h"
#include "AudioManager.h"
#include "VolumeControl.h"
#include "Log.h"


GuiBluetooth::GuiBluetooth(Window* window, const std::string title, const std::string subtitle, bool listPairedDevices)
	: GuiComponent(window), mMenu(window, title.c_str())
{
	mTitle = title;
	mWaitingLoad = false;
	mListPairedDevices = listPairedDevices;

	if (!subtitle.empty())
		mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	mWindow->postToUiThread([this]() { onRefresh(); });

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onRefresh(); });
	if (mListPairedDevices)
    	mMenu.addButton(_("DELETE"), _("DELETE"), [&] { onDeleteConnection(); });

	mMenu.addButton(_("BACK"), _("BACK"), [&] { delete this; });
}

void GuiBluetooth::load(std::vector<std::string> btDevices)
{
	mMenu.clear();

	if (btDevices.size() == 0)
	{
		std::string msg;
		if (mListPairedDevices)
			msg = "NO BLUETOOTH DEVICES FOUND";
		else
			msg = "NO BLUETOOTH PAIRED DEVICES FOUND";
    
		mMenu.addEntry(_(msg), false, std::bind(&GuiBluetooth::onRefresh, this));
	}
	else
	{
		for (auto bt_id : btDevices)
			mMenu.addEntry(bt_id, false, [this, bt_id]() { GuiBluetooth::onSave(bt_id); });
	}

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	mWaitingLoad = false;
}

bool GuiBluetooth::onSave(const std::string& value)
{
	if (mWaitingLoad)
		return false;

	bool device_connected = isDeviceConnected(value);
	std::string device = Utils::String::replace(value, Settings::getInstance()->getString("already.connection.exist.flag"), "");
	// device --> DEVICE_ID DEVICE_NAME
	std::vector<std::string> device_info = Utils::String::split(device, ' ', true);
	std::string device_id = device_info[0],
				device_name = device_info[0];
	if (device_info.size() > 1)
		device_name = Utils::String::trim( Utils::String::replace(device, device_id, "") );

	std::string msg;
	if (mListPairedDevices && device_connected)
		msg.append(_("DISCONNECTING BLUETOOTH DEVICE")).append(" '").append(device_name).append("' ...");
	else
		msg.append(_("CONNECTING BLUETOOTH DEVICE")).append(" '").append(device_name).append("' ...");

	bool action_result = false;
	Window* window = mWindow;
	mWindow->pushGui(new GuiLoading<bool>(mWindow, msg, 
		[this, device_id, device_connected]
		{
			mWaitingLoad = true;

			bool result = false;
			if (mListPairedDevices && device_connected)
			{
				LOG(LogDebug) << "GuiBluetooth::onSave() - calling --> ApiSystem::getInstance()->disconnectBluetoothDevice(" << device_id << ')';
				Log::flush();
				result = ApiSystem::getInstance()->disconnectBluetoothDevice(device_id);
			}
			else
			{
				LOG(LogDebug) << "GuiBluetooth::onSave() - calling --> ApiSystem::getInstance()->connectBluetoothDevice(" << device_id << ')';
				Log::flush();
				result = ApiSystem::getInstance()->connectBluetoothDevice(device_id);
			}
			LOG(LogDebug) << "GuiBluetooth::onSave() - result: " << Utils::String::boolToString(result);
			Log::flush();

			// cheking audio bluetooth connecting changes
			std::string audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
			bool sys_btadc = !audio_device.empty();
			bool es_btadc = SystemConf::getInstance()->getBool("bluetooth.audio.connected");
			SystemConf::getInstance()->setBool("bluetooth.audio.connected", sys_btadc);
			SystemConf::getInstance()->set("bluetooth.audio.device", audio_device);

			LOG(LogDebug) << "GuiBluetooth::onSave() - audio_device: '" << audio_device << "', sys_btadc: " << Utils::String::boolToString(sys_btadc) << ", es_btadc: " << Utils::String::boolToString(es_btadc);
			Log::flush();
			// checking if a reset audio configuration is needed
			if ((sys_btadc && !es_btadc) || (!sys_btadc && es_btadc))
			{
				LOG(LogDebug) << "GuiBluetooth::onSave() - Audio BT device changed, restart ES audio components.";
				Log::flush();
				AudioManager::getInstance()->deinit();
				VolumeControl::getInstance()->deinit();

				VolumeControl::getInstance()->init();
				AudioManager::getInstance()->init();
/*
				std::string msg = _("THE AUDIO INTERFACE HAS CHANGED.") + "\n" + _("THE EMULATIONSTATION WILL NOW RESTART.");
				window->pushGui(new GuiMsgBox(window, msg,
					_("OK"),
					[] {
						if (quitES(QuitMode::RESTART) != 0)
							LOG(LogWarning) << "GuiMenu::openAdvancedSettings() - Restart terminated with non-zero result!";
					}));
*/
			}

			return result;
		},
		[this, window, device_name, &action_result, device_connected](bool result)
		{
			mWaitingLoad = false;
            action_result = result;
			bool deleteWindow = false;
            std::string msg;
			if (result)
 			{
				if (mListPairedDevices)
				{
					if (device_connected)
						msg.append("'").append(device_name).append("' ").append(_("DEVICE SUCCESSFULLY DISCONNECTED"));
					else
						msg.append("'").append(device_name).append("' ").append(_("DEVICE SUCCESSFULLY CONNECTED"));
				}
				else
					msg.append("'").append(device_name).append("' ").append(_("DEVICE SUCCESSFULLY PAIRED AND CONNECTED"));
				
				deleteWindow = true;
			}
			else
			{
				if (mListPairedDevices)
				{
					if (device_connected)
						msg.append("'").append(device_name).append("' ").append(_("DEVICE FAILED TO UNPAIR"));
					else
						msg.append("'").append(device_name).append("' ").append(_("DEVICE FAILED TO CONNECT"));
				}
				else
					msg.append("'").append(device_name).append("' ").append(_("DEVICE FAILED TO PAIR AND CONNECT"));
			}
			LOG(LogDebug) << "GuiBluetooth::onSave() - message: " << msg;
			Log::flush();
			window->pushGui(new GuiMsgBox(window, msg,	_("OK"), [this, deleteWindow] { if (deleteWindow) delete this; }));

		}));

//	if (action_result)
//		delete this;

	return action_result;
}

bool GuiBluetooth::input(InputConfig* config, Input input)
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

	return false;
}

std::vector<HelpPrompt> GuiBluetooth::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));

	if (mListPairedDevices && isDeviceConnected(getMenuItemSelected()))
	{
		LOG(LogDebug) << "GuiBluetooth::getHelpPrompts() - Paired Devices, getMenuItemSelected(): " << getMenuItemSelected();
		Log::flush();
		prompts.push_back(HelpPrompt(BUTTON_OK, _("DISCONNECT")));
	}
	else
	{
		LOG(LogDebug) << "GuiBluetooth::getHelpPrompts() - Paired Devices, getMenuItemSelected(): " << getMenuItemSelected();
		Log::flush();
   		prompts.push_back(HelpPrompt(BUTTON_OK, _("CONNECT")));
	}
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	return prompts;
}

void GuiBluetooth::onRefresh()
{
	Window* window = mWindow;

	std::string msg;
	if (mListPairedDevices)
		msg = "SEARCHING BLUETOOTH PAIRED DEVICES...";
	else
		msg = "SEARCHING BLUETOOTH DEVICES...";

	window->pushGui(new GuiLoading<std::vector<std::string>>(window, _(msg), 
		[this]
		{
			mWaitingLoad = true;

			if (mListPairedDevices)
				return ApiSystem::getInstance()->getBluetoothPairedDevices();
			else
				return ApiSystem::getInstance()->getBluetoothDevices();
		},
		[this](std::vector<std::string> btDevices)
		{
			mWaitingLoad = false;
			load(btDevices);
		}));
}

void GuiBluetooth::onDeleteConnection()
{
	Window* window = mWindow;

	std::string entry = getMenuItemSelected();

	if (entry.empty())
		return;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO DELETE THIS CONNECTION?"),
		_("OK"),
    		[this, window] {
				std::string entry = getMenuItemSelected(),
                            device_id;
				
				if (!entry.empty())
				{                
					std::vector<std::string> device_info = Utils::String::split(entry, ' ', true);
					device_id = device_info[0];
					LOG(LogDebug) << "GuiBluetooth::onDeleteConnection() - calling --> ApiSystem::getInstance()->deleteBluetoothDeviceConnection(" << device_id << ')';
					Log::flush();
					if (ApiSystem::getInstance()->deleteBluetoothDeviceConnection(device_id))
					{
						LOG(LogDebug) << "GuiBluetooth::onDeleteConnection() - Successfully deleted device '" << device_id << "', refreshing devices";
						Log::flush();
						window->postToUiThread([this]() { GuiBluetooth::onRefresh(); });
					}
				}
			},
        _("CANCEL"), [] {}));
}

bool GuiBluetooth::isDeviceConnected(const std::string device)
{
	return Utils::String::endsWith(device, Settings::getInstance()->getString("already.connection.exist.flag"));
}

std::string GuiBluetooth::getMenuItemSelected()
{
	if (mMenu.size() > 0)
		return mMenu.getSelected();
	
	return "";
}
