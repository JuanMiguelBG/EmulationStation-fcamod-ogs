#include "guis/GuiBluetoothPaired.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "SystemConf.h"
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiLoading.h"
#include "SystemConf.h"
#include "Log.h"
#include "utils/FileSystemUtil.h"


GuiBluetoothPaired::GuiBluetoothPaired(Window* window, const std::string title, const std::string subtitle)
	: GuiComponent(window), mMenu(window, title.c_str(), true)
{
	mTitle = title;
	mWaitingLoad = false;
	hasDevices = false;
	mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	mWindow->postToUiThread([this]() { onRefresh(); });
}

void GuiBluetoothPaired::load(std::vector<BluetoothDevice> btDevices)
{
	mMenu.clear();
	mMenu.clearButtons();
	hasDevices = false;

	if (btDevices.size() == 0)
		mMenu.addEntry(_("NO BLUETOOTH DEVICES FOUND"), false, std::bind(&GuiBluetoothPaired::onRefresh, this));
	else
	{
		hasDevices = true;
		for (auto btDevice : btDevices)
		{
			std::string device_name = btDevice.name,
						device_id;

			if (Settings::getInstance()->getBool("bluetooth.use.alias") && !btDevice.alias.empty() && (btDevice.name != btDevice.alias))
			{
				device_name = btDevice.alias;
				device_id.append(btDevice.name).append(" - ");
			}
			if (btDevice.connected)
				device_name.append(SystemConf::getInstance()->get("already.connection.exist.flag"));

			device_id.append(btDevice.id);

			mMenu.addWithDescription(device_name, device_id, [this, btDevice]() { GuiBluetoothPaired::onAction(btDevice); },
									 btDevice.type, device_name);
		}
	}

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onRefresh(); });
	if (hasDevices)
		mMenu.addButton(_("UNPAIR ALL"), _("UNPAIR ALL"), [&] { onDeleteAll(); });

	mMenu.addButton(_("BACK"), _("BACK"), [&] { delete this; });

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	updateHelpPrompts();
	mWaitingLoad = false;
}

bool GuiBluetoothPaired::onAction(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad)
		return false;

	if (btDevice.connected)
		return onDisconnectDevice(btDevice);
	else
		return onConnectDevice(btDevice);
}

void GuiBluetoothPaired::displayRestartDialog(Window *window, const std::string message, bool deleteWindow, bool restarES)
{
	window->pushGui(new GuiMsgBox(window, message, _("OK"),
		[this, window, deleteWindow, restarES]
		{
			// checking if a reset audio configuration is needed
			if (restarES)
			{
				//LOG(LogDebug) << "GuiBluetoothPaired::displayRestartDialog() - Audio BT device changed, restarting ES.";
				//Log::flush();
				ApiSystem::getInstance()->stopBluetoothLiveScan();

				std::string msg = _("THE AUDIO INTERFACE HAS CHANGED.") + "\n" + _("THE EMULATIONSTATION WILL NOW RESTART.");
				window->pushGui(new GuiMsgBox(window, msg,
					_("YES"),
						[]
						{
							Utils::FileSystem::createFile(Utils::FileSystem::getEsConfigPath() + "/skip_auto_connect_bt_audio_device_onboot.lock");
							if (quitES(QuitMode::RESTART) != 0)
								LOG(LogWarning) << "GuiBluetoothPaired::displayRestartDialog() - Restart terminated with non-zero result!";
						},
					_("NO"), nullptr));
			}
			else
			{
				if (deleteWindow)
					delete this;
				else
					GuiBluetoothPaired::onRefresh();
			}
		}));
}

bool GuiBluetoothPaired::onConnectDevice(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	std::string msg = _("CONNECTING BLUETOOTH DEVICE") + " '" + getDeviceName(btDevice) + "'...";

	Window* window = mWindow;

	std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
	//LOG(LogDebug) << "GuiBluetoothPaired::onConnectDevice() - actual BT audio device: '" << audio_device << "'";
	//Log::flush();

	window->pushGui(new GuiLoading<bool>(window, msg, 
		[this, btDevice, audio_device]
		{
			mWaitingLoad = true;

			bool result = false;

			//LOG(LogDebug) << "GuiBluetoothPaired::onConnectDevice() - calling --> ApiSystem::getInstance()->connectBluetoothDevice('" << btDevice.id << "', type: '" << btDevice.type << "')";
			//Log::flush();
			result = ApiSystem::getInstance()->connectBluetoothDevice(btDevice.id);
			
			//LOG(LogDebug) << "GuiBluetoothPaired::onConnectDevice() - tried to connect to '" << getDeviceName(btDevice) << "' device, result: " << Utils::String::boolToString(result);
			//Log::flush();

			// successfully connected
			if (result)
			{
				// BT 4.2, only one audio device
				// reload BT audio device info
				std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
				//LOG(LogDebug) << "GuiBluetoothPaired::onConnectDevice() - new BT audio device: '" << new_audio_device << "'";
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

			if (result)
 			{
				msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE SUCCESSFULLY CONNECTED"));

				// audio bluetooth connecting changes
				restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );
			}
			else
				msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE FAILED TO CONNECT"));

			//LOG(LogDebug) << "GuiBluetoothPaired::onConnectDevice() - message: " << msg;
			//Log::flush();

			mWaitingLoad = false;
			GuiBluetoothPaired::displayRestartDialog(window, msg, result, restar_ES);
		}));

	return true;
}

bool GuiBluetoothPaired::onDisconnectDevice(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO DISCONNECT THE DEVICE?"),
		_("YES"), [this, window, btDevice]
		{
			std::string msg = _("DISCONNECTING BLUETOOTH DEVICE") + " '" + getDeviceName(btDevice) + "'...";
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
			//LOG(LogDebug) << "GuiBluetoothPaired::onDisconnectDevice() - actual BT audio device: '" << audio_device << "'";
			//Log::flush();

			window->pushGui(new GuiLoading<bool>(window, msg, 
				[this, btDevice, audio_device]
				{
					mWaitingLoad = true;

					bool result = false;

					//LOG(LogDebug) << "GuiBluetoothPaired::onDisconnectDevice() - calling --> ApiSystem::getInstance()->disconnectBluetoothDevice(" << btDevice.id << "', type: '" << btDevice.type << ')';
					//Log::flush();
					result = ApiSystem::getInstance()->disconnectBluetoothDevice(btDevice.id);
					
					//LOG(LogDebug) << "GuiBluetoothPaired::onDisconnectDevice() - tried to disconnect from '" << getDeviceName(btDevice) << "' device, result: " << Utils::String::boolToString(result);
					//Log::flush();

					// successfully connected
					if (result)
					{
						// BT 4.2, only one audio device allowed
						// reload BT audio device info
						std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
						//LOG(LogDebug) << "GuiBluetoothPaired::onDisconnectDevice() - new BT audio device: '" << new_audio_device << "'";
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

					if (result)
					{
						msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE SUCCESSFULLY DISCONNECTED"));

						// audio bluetooth connecting changes
						restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );
					}
					else
						msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE FAILED TO DISCONNECT"));

					//LOG(LogDebug) << "GuiBluetoothPaired::onDisconnectDevice() - message: '" << msg << "'";
					//Log::flush();

					mWaitingLoad = false;
					GuiBluetoothPaired::displayRestartDialog(window, msg, result, restar_ES);
				}));
		},
		_("NO"), nullptr));

	return true;
}

bool GuiBluetoothPaired::input(InputConfig* config, Input input)
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
		if (hasDevices)
			onDeleteAll();

		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiBluetoothPaired::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));
	if (hasDevices)
	{
		prompts.push_back(HelpPrompt("y", _("UNPAIR ALL")));

		std::string selected = mMenu.getSelected();

		LOG(LogDebug) << "GuiBluetoothPaired::getHelpPrompts() - cursor position: " << std::to_string(mMenu.getCursor()) << ", selected: " << selected;
		Log::flush();

		if (!selected.empty())
		{  // BT device
			if (Utils::String::endsWith(selected, SystemConf::getInstance()->get("already.connection.exist.flag")))
				prompts.push_back(HelpPrompt(BUTTON_OK, _("DISCONNECT")));
			else
				prompts.push_back(HelpPrompt(BUTTON_OK, _("CONNECT")));
		}
	}
	else
		prompts.push_back(HelpPrompt(BUTTON_OK, _("REFRESH")));

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	return prompts;
}

void GuiBluetoothPaired::onRefresh()
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
			mWaitingLoad = false;
			load(btDevices);
		}));
}

void GuiBluetoothPaired::onDeleteAll()
{
	if (mWaitingLoad || !hasDevices)
		return;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO REMOVE ALL DEVICES?"),
		_("YES"), [this, window]
		{
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
			//LOG(LogDebug) << "GuiBluetoothPaired::onDeleteAll() - BT audio device connected: '" << Utils::String::boolToString(!audio_device.empty()) << "'";
			//Log::flush();

			window->pushGui(new GuiLoading<bool>(window, _("PLEASE WAIT..."),
				[this, audio_device]()
				{
					mWaitingLoad = true;
					bool result = ApiSystem::getInstance()->deleteAllBluetoothDevices();

					// reload BT audio device info
					std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
					//LOG(LogDebug) << "GuiBluetoothPaired::onDeleteAll() - new BT audio device: '" << new_audio_device << "'";
					//Log::flush();

					SystemConf::getInstance()->setBool("bluetooth.audio.connected", !new_audio_device.empty());
					SystemConf::getInstance()->set("bluetooth.audio.device", new_audio_device);
					
					return result;
				},
				[this, window, audio_device](bool result)
				{
					bool restar_ES = false;
					std::string msg;

					if (result)
						msg = "ALL BLUETOOTH DEVICES HAVE BEEN UNPAIRED.";
					else
						msg = "NOT ALL BLUETOOTH DEVICES HAVE BEEN UNPAIRED.";

					// audio bluetooth connecting changes
					restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );

					//LOG(LogDebug) << "GuiBluetoothPaired::onDeleteAll() - message: '" << msg << "'";
					//Log::flush();

					mWaitingLoad = false;
					GuiBluetoothPaired::displayRestartDialog(window, _(msg), result, restar_ES);
				}));
		},
		_("NO"), nullptr));
}

std::string GuiBluetoothPaired::getDeviceName(const BluetoothDevice& btDevice) const
{
	std::string name = btDevice.name;
	if (Settings::getInstance()->getBool("bluetooth.use.alias") && !btDevice.alias.empty())
		name = btDevice.alias;

	return name;
}
