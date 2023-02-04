#include "guis/GuiBluetoothConnected.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiLoading.h"
#include "SystemConf.h"
#include "AudioManager.h"
#include "VolumeControl.h"
#include "Log.h"


GuiBluetoothConnected::GuiBluetoothConnected(Window* window, const std::string title, const std::string subtitle)
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

void GuiBluetoothConnected::load(std::vector<BluetoothDevice> btDevices)
{
	mMenu.clear();
	mMenu.clearButtons();

	hasDevices = false;
	if (btDevices.size() == 0)
		mMenu.addEntry(_("NO BLUETOOTH DEVICES FOUND"), false, std::bind(&GuiBluetoothConnected::onRefresh, this));
	else
	{
		hasDevices = true;
		for (auto btDevice : btDevices)
			mMenu.addWithDescription(btDevice.name, btDevice.id, nullptr, [this, btDevice]() { GuiBluetoothConnected::onDisconnectDevice(btDevice); }, btDevice.type);
	}

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onRefresh(); });
	if (hasDevices)
		mMenu.addButton(_("DISCONNECT ALL"), _("DISCONNECT ALL"), [&] { GuiBluetoothConnected::onDisconnectAll(); });
	
	mMenu.addButton(_("BACK"), _("BACK"), [&] { delete this; });

	mMenu.updateSize();

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	updateHelpPrompts();
	mWaitingLoad = false;
}

void GuiBluetoothConnected::displayRestartDialog(Window *window, const std::string message, bool deleteWindow, bool restarES)
{
	window->pushGui(new GuiMsgBox(window, message, _("OK"),
		[this, window, deleteWindow, restarES]
		{
			// checking if a reset audio configuration is needed
			if (restarES)
			{
				//LOG(LogDebug) << "GuiBluetoothConnected::displayRestartDialog() - Audio BT device changed, restarting ES.";
				//Log::flush();
				ApiSystem::getInstance()->stopBluetoothLiveScan();

				std::string msg = _("THE AUDIO INTERFACE HAS CHANGED.") + "\n" + _("THE EMULATIONSTATION WILL NOW RESTART.");
				window->pushGui(new GuiMsgBox(window, msg,
					_("OK"),
						[]
						{
							Utils::FileSystem::createFile(Utils::FileSystem::getEsConfigPath() + "/skip_auto_connect_bt_audio_device_onboot.lock");
							if (quitES(QuitMode::RESTART) != 0)
								LOG(LogWarning) << "GuiBluetoothConnected::displayRestartDialog() - Restart terminated with non-zero result!";
						}));
			}
			else
			{
				if (deleteWindow)
					delete this;
				else
					GuiBluetoothConnected::onRefresh();
			}
		}));
}

bool GuiBluetoothConnected::onDisconnectDevice(const BluetoothDevice& btDevice)
{
	if (mWaitingLoad || !hasDevices)
		return false;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO DISCONNECT THE DEVICE?"),
		_("YES"), [this, window, btDevice]
		{
			std::string msg = (_("DISCONNECTING BLUETOOTH DEVICE"));
			msg.append(" '").append(btDevice.name).append("' ...");
			
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
			//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - actual BT audio device: '" << audio_device << "'";
			//Log::flush();

			window->pushGui(new GuiLoading<bool>(window, msg, 
				[this, btDevice, audio_device]
				{
					mWaitingLoad = true;

					bool result = false;

					//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - calling --> ApiSystem::getInstance()->disconnectBluetoothDevice(" << btDevice.id << "', type: '" << btDevice.type << ')';
					//Log::flush();
					result = ApiSystem::getInstance()->disconnectBluetoothDevice(btDevice.id);
					
					//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - tried to disconnect from '" << btDevice.name << "' device, result: " << Utils::String::boolToString(result);
					//Log::flush();

					// successfully connected
					if (result)
					{
						// BT 4.2, only one audio device allowed
						// reload BT audio device info
						std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
						//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - new BT audio device: '" << new_audio_device << "'";
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
						msg.append("'").append(btDevice.name).append("' ").append(_("DEVICE SUCCESSFULLY DISCONNECTED"));

						// audio bluetooth connecting changes
						restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );
					}
					else
						msg.append("'").append(btDevice.name).append("' ").append(_("DEVICE FAILED TO DISCONNECT"));

					//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - message: " << msg;
					//Log::flush();

					mWaitingLoad = false;
					GuiBluetoothConnected::displayRestartDialog(window, msg, result, restar_ES);
				}));
		},
		_("NO"), nullptr));

	return true;
}

bool GuiBluetoothConnected::input(InputConfig* config, Input input)
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
			onDisconnectAll();

		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiBluetoothConnected::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));
	if (hasDevices)
	{
		prompts.push_back(HelpPrompt("y", _("DISCONNECT ALL")));
		prompts.push_back(HelpPrompt(BUTTON_OK, _("DISCONNECT")));
	}
	else
		prompts.push_back(HelpPrompt(BUTTON_OK, _("REFRESH")));

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	return prompts;
}

void GuiBluetoothConnected::onRefresh()
{
	Window* window = mWindow;

	window->pushGui(new GuiLoading<std::vector<BluetoothDevice>>(window, _("SEARCHING BLUETOOTH CONNECTED DEVICES..."), 
		[this]
		{
			mWaitingLoad = true;
			return ApiSystem::getInstance()->getBluetoothConnectedDevices();
		},
		[this](std::vector<BluetoothDevice> btDevices)
		{
			mWaitingLoad = false;
			load(btDevices);
		}));
}

void GuiBluetoothConnected::onDisconnectAll()
{
	if (mWaitingLoad || !hasDevices)
		return;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO DISCONNECT ALL DEVICES?"),
		_("YES"), [this, window]
		{
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
			//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectAll() - BT audio device connected: '" << Utils::String::boolToString(!audio_device.empty()) << "'";
			//Log::flush();

			mWindow->pushGui(new GuiLoading<bool>(window, _("PLEASE WAIT..."),
				[this, window, audio_device]()
				{
					mWaitingLoad = true;

					bool result = ApiSystem::getInstance()->disconnectAllBluetoothDevices();

					std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
					//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - not all devices are disconnected, BT audio device: '" << new_audio_device << "'";
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
						msg = "ALL BLUETOOTH DEVICES HAVE BEEN DISCONNECTED.";
					else
						msg = "NOT ALL BLUETOOTH DEVICES HAVE BEEN DISCONNECTED.";

					// audio bluetooth connecting changes
					restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );

					//LOG(LogDebug) << "GuiBluetoothConnected::onDisconnectDevice() - message: '" << msg << "'";
					//Log::flush();

					mWaitingLoad = false;
					GuiBluetoothConnected::displayRestartDialog(window, _(msg), result, restar_ES);
				}));
		},
		_("NO"), nullptr));
}
