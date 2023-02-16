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
	mTitle = title;
	mWaitingLoad = false;
	hasDevices = false;
	mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	mWindow->postToUiThread([this]() { onScan(); });

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onScan(); });
	mMenu.addButton(_("BACK"), _("BACK"), [&] { delete this; });
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
		{
			std::string device_name = btDevice.name,
						device_value = btDevice.name,
						device_id;
			device_value.append(" - ").append(btDevice.id);

			if (Settings::getInstance()->getBool("bluetooth.use.alias") && !btDevice.alias.empty() && (btDevice.name != btDevice.alias))
			{
				device_name = btDevice.alias;
				device_value = btDevice.alias;
				device_value.append(" - ").append(btDevice.id).append(SystemConf::getInstance()->get("already.connection.exist.flag"));
				device_id.append(btDevice.name).append(" - ");
			}
			device_id.append(btDevice.id);
			mMenu.addWithDescription(device_name, device_id, [this, btDevice]() { GuiBluetoothScan::onConnectDevice(btDevice); }, btDevice.type, device_value);
		}
	}

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	updateHelpPrompts();
	mWaitingLoad = false;
}

void GuiBluetoothScan::handleAlias(Window *window, bool deleteWindow, bool restarES, const std::string deviceData)
{
	std::string title = _("ADD ALIAS"),
				initial_data,
				device_id;
	
	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - deviceData: " << deviceData;

	std::vector<std::string> data = Utils::String::split(deviceData, " - ", true);
	initial_data = data[0];
	device_id = data[1];

	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - data.size(): " << std::to_string(data.size());
	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - initial_data: " << initial_data;
	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - device_id: " << device_id;
	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - flag: '" << SystemConf::getInstance()->get("already.connection.exist.flag") << "'";
	Log::flush();

	device_id = Utils::String::replace(device_id, SystemConf::getInstance()->get("already.connection.exist.flag"), "");

	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - after replace device_id: " << device_id;
	Log::flush();
	std::function<bool(const std::string /*&newVal*/)> updateVal = [this, window, deleteWindow, restarES, device_id](const std::string &newVal)
	{	
		LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - updateVal() -  device_id: " << device_id;
		LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - updateVal() -  newVal: " << newVal;
		Log::flush();
		
		if (ApiSystem::getInstance()->setBluetoothDeviceAlias(device_id, newVal))
		{
			displayRestartDialog(window, deleteWindow, restarES);
			return true;
		}
		
		return false;
	}; // ok callback (apply new value to ed)

	std::function<void(const std::string /*&newVal*/)> cancelVal = [this, window, deleteWindow, restarES, device_id](const std::string &newVal)
	{	
		LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - cancelVal() -  device_id: " << device_id;
		LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - cancelVal() -  newVal: " << newVal;
		Log::flush();
		
		displayRestartDialog(window, deleteWindow, restarES);
	}; // cancel callback (apply new value to ed)

	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - open keyborad";
	Log::flush();
	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, initial_data, updateVal, false, cancelVal));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, title, initial_data, updateVal, false, cancelVal));

	LOG(LogDebug) << "GuiBluetoothScan::handleAlias() - exit keyborad";
	Log::flush();
}

void GuiBluetoothScan::displayRestartDialog(Window *window, bool deleteWindow, bool restarES)
{
	// checking if a reset audio configuration is needed
	if (restarES)
	{
		//LOG(LogDebug) << "GuiBluetoothScan::displayRestartDialog() - Audio BT device changed, restarting ES.";
		//Log::flush();
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
			delete this;
		else
			GuiBluetoothScan::onScan();
	}
}

bool GuiBluetoothScan::onConnectDevice(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	Window* window = mWindow;

	std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
	//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - actual BT audio device: '" << audio_device << "'";
	//Log::flush();

	std:: string msg(_("CONNECTING BLUETOOTH DEVICE"));
	window->pushGui(new GuiLoading<bool>(window, msg.append(" '").append(getDeviceName(btDevice)).append("'..."), 
		[this, btDevice, audio_device]
		{
			mWaitingLoad = true;

			bool result = false;

			//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - calling --> ApiSystem::getInstance()->pairBluetoothDevice(" << btDevice.id << ')';
			//Log::flush();
			result = ApiSystem::getInstance()->pairBluetoothDevice(btDevice.id);

			// fail paired
			if (!result)
				return false;

			bool isAudioDevice = btDevice.isAudioDevice;
			if (!isAudioDevice && (btDevice.type == "unknown"))
				isAudioDevice = ApiSystem::getInstance()->isBluetoothAudioDevice(btDevice.id);

			//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - successfully paired device ID: '" << btDevice.id << "', type: '" << btDevice.type << "', isAudioDevice: '" << Utils::String::boolToString(isAudioDevice) << "'";
			//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - calling --> ApiSystem::getInstance()->connectBluetoothDevice(" << btDevice.id << ')';
			//Log::flush();
			result = ApiSystem::getInstance()->connectBluetoothDevice(btDevice.id);
			
			//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - tried to connect to '" << getDeviceName(btDevice) << "' device, result: " << Utils::String::boolToString(result);
			//Log::flush();

			// successfully connected
			if (result)
			{
				// BT 4.2, only one audio device
				// reload BT audio device info
				std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
				//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - new BT audio device: '" << new_audio_device << "'";
				//Log::flush();

				SystemConf::getInstance()->setBool("bluetooth.audio.connected", !new_audio_device.empty());
				SystemConf::getInstance()->set("bluetooth.audio.device", new_audio_device);
			}

			return result;
		},
		[this, window, btDevice, audio_device](bool result)
		{
			bool restar_ES = false;
            std::string msg;

			mWaitingLoad = false;
			if (result)
 			{
				msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE SUCCESSFULLY PAIRED AND CONNECTED"));

				// audio bluetooth connecting changes
				restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );

				window->pushGui(new GuiMsgBox(window, msg,
					_("OK"),
						[this, window, result, restar_ES]
						{
							displayRestartDialog(window, result, restar_ES);
						},
					_("ADD ALIAS"),
						[this, window, result, restar_ES]
						{
							handleAlias(window, result, restar_ES, getMenu()->getSelected());
						})
				);
			}
			else
			{
				msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE FAILED TO PAIR AND CONNECT"));

				window->pushGui(new GuiMsgBox(window, msg,
					_("OK"),
						[this, window]
						{
							displayRestartDialog(window, false, false);
						})
				);
			}
			//LOG(LogDebug) << "GuiBluetoothScan::onConnectDevice() - message: " << msg;
			//Log::flush();
		}));

	return true;
}

bool GuiBluetoothScan::input(InputConfig* config, Input input)
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
			mWaitingLoad = false;
			load(btDevices);
		}));
}

std::string GuiBluetoothScan::getDeviceName(const BluetoothDevice& btDevice) const
{
	std::string name = btDevice.name;
	if (Settings::getInstance()->getBool("bluetooth.use.alias") && !btDevice.alias.empty())
		name = btDevice.alias;

	return name;
}
