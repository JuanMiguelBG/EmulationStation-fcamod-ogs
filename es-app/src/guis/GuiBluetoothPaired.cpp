#include "guis/GuiBluetoothPaired.h"

#include "guis/GuiMsgBox.h"
#include "Window.h"
#include <string>
#include "SystemConf.h"
#include "ApiSystem.h"
#include "EsLocale.h"
#include "guis/GuiLoading.h"
#include "SystemConf.h"
#include "Log.h"
#include "utils/FileSystemUtil.h"


GuiBluetoothPaired::GuiBluetoothPaired(Window* window, const std::string title, const std::string subtitle)
	: GuiComponent(window), mMenu(window, title.c_str(), true), mOKButton("OK")
{
	mTitle = title;
	mWaitingLoad = false;
	mMenu.setSubTitle(subtitle);

	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);

	mWindow->postToUiThread([this]() { onRefresh(); });
}

void GuiBluetoothPaired::load(std::vector<BluetoothDevice> btDevices)
{
	LOG(LogDebug) << "GuiBluetoothPaired::load() - before execute 'mMenu.clear()'";
	Log::flush();
	mMenu.clear();
	LOG(LogDebug) << "GuiBluetoothPaired::load() - after execute 'mMenu.clear()'";

	LOG(LogDebug) << "GuiBluetoothPaired::load() - before execute 'mMenu.clearButtons()'";
	Log::flush();
	mMenu.clearButtons();
	LOG(LogDebug) << "GuiBluetoothPaired::load() - after execute 'mMenu.clearButtons()'";

	LOG(LogDebug) << "GuiBluetoothPaired::load() - before execute 'mMapDevices.clear()'";
	Log::flush();
	mMapDevices.clear();
	LOG(LogDebug) << "GuiBluetoothPaired::load() - after execute 'mMapDevices.clear()'";
	Log::flush();

	if (btDevices.size() == 0)
		mMenu.addEntry(_("NO BLUETOOTH DEVICES FOUND"), false, std::bind(&GuiBluetoothPaired::onRefresh, this));
	else
	{
		for (auto btDevice : btDevices)
		{
			LOG(LogDebug) << "GuiBluetoothPaired::load() - inside 'for (auto btDevice : btDevices)'";
			Log::flush();
			mMapDevices[btDevice.id] = btDevice;
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

			LOG(LogDebug) << "GuiBluetoothPaired::load() - before execute 'mMenu.addWithDescription()'";
			Log::flush();
			mMenu.addWithDescription(device_name, device_id, nullptr, btDevice.type, btDevice.id);
		}
	}

	mMenu.addButton(_("REFRESH"), _("REFRESH"), [&] { onRefresh(); });
	if (isHasDevices())
		mMenu.addButton(_("UNPAIR ALL"), _("UNPAIR ALL"), [&] { onUnpairAll(); });

	mMenu.addButton(_("BACK"), _("BACK"), [&] { onClose(); });

	mMenu.updateSize();

//	if (Renderer::isSmallScreen())
//		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	updateHelpPrompts();
	mWaitingLoad = false;
}

void GuiBluetoothPaired::onAction()
{
	if (mWaitingLoad || !isHasDevices())
		return;

	std::string selected = mMenu.getSelected();
	if (selected.empty() || (mMapDevices.find(selected) == mMapDevices.end()))
		return;

	BluetoothDevice	btDevice = mMapDevices[selected];
	if (btDevice.connected)
		onDisconnectDevice(btDevice);
	else
		onConnectDevice(btDevice);
}

void GuiBluetoothPaired::displayRestartDialog(Window *window, const std::string message, bool deleteWindow, bool restarES)
{
	window->pushGui(new GuiMsgBox(window, message, _("OK"),
		[this, window, deleteWindow, restarES]
		{
			// checking if a reset audio configuration is needed
			if (restarES)
			{
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
					onClose();
				else
					GuiBluetoothPaired::onRefresh();
			}
		}));
}

void GuiBluetoothPaired::onConnectDevice(const BluetoothDevice& btDevice)
{
	std::string msg = _("CONNECTING BLUETOOTH DEVICE") + " '" + getDeviceName(btDevice) + "'...";

	Window* window = mWindow;

	std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
	window->pushGui(new GuiLoading<bool>(window, msg, 
		[this, btDevice, audio_device]
		{
			mWaitingLoad = true;

			bool result = ApiSystem::getInstance()->connectBluetoothDevice(btDevice.id);
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
            std::string msg;

			if (result)
 			{
				msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE SUCCESSFULLY CONNECTED"));

				// audio bluetooth connecting changes
				restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );
			}
			else
				msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE FAILED TO CONNECT"));

			mWaitingLoad = false;
			GuiBluetoothPaired::displayRestartDialog(window, msg, result, restar_ES);
		}));
}

void GuiBluetoothPaired::onDisconnectDevice(const BluetoothDevice& btDevice)
{
	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO DISCONNECT THE DEVICE?"),
		_("YES"), [this, window, btDevice]
		{
			std::string msg = _("DISCONNECTING BLUETOOTH DEVICE") + " '" + getDeviceName(btDevice) + "'...";
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
			window->pushGui(new GuiLoading<bool>(window, msg, 
				[this, btDevice, audio_device]
				{
					mWaitingLoad = true;

					bool result = ApiSystem::getInstance()->disconnectBluetoothDevice(btDevice.id);				
					// successfully connected
					if (result)
					{
						// BT 4.2, only one audio device allowed
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
					std::string msg;

					if (result)
					{
						msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE SUCCESSFULLY DISCONNECTED"));

						// audio bluetooth connecting changes
						restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );
					}
					else
						msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE FAILED TO DISCONNECT"));

					mWaitingLoad = false;
					GuiBluetoothPaired::displayRestartDialog(window, msg, result, restar_ES);
				}));
		},
		_("NO"), nullptr));
}

bool GuiBluetoothPaired::input(InputConfig* config, Input input)
{
	if (mWaitingLoad || GuiComponent::input(config, input))
		return true;

	if (mOKButton.isShortPressed(config, input))
	{
		if (isHasDevices())
			onAction();

		return true;
	}
	else if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
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
		if (isHasDevices())
			onUnpairAll();

		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiBluetoothPaired::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("REFRESH")));
	if (isHasDevices())
	{
		prompts.push_back(HelpPrompt("y", _("UNPAIR ALL")));

		std::string selected = mMenu.getSelected();
		if (!selected.empty() && (mMapDevices.find(selected) != mMapDevices.end()))
		{  // BT device
			if (mMapDevices[selected].connected)
				prompts.push_back(HelpPrompt(BUTTON_OK, _("DISCONNECT / UNPAIR (HOLD)")));
			else
				prompts.push_back(HelpPrompt(BUTTON_OK, _("CONNECT / UNPAIR (HOLD)")));
		}
	}

	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	return prompts;
}

void GuiBluetoothPaired::onRefresh()
{
	Window* window = mWindow;

	window->pushGui(new GuiLoading<std::vector<BluetoothDevice>>(window, _("SEARCHING BLUETOOTH PAIRED DEVICES..."), 
		[this]
		{
			LOG(LogDebug) << "GuiBluetoothPaired::onRefresh() - before execute 'ApiSystem::getInstance()->getBluetoothPairedDevices()'";
			Log::flush();
			mWaitingLoad = true;
			return ApiSystem::getInstance()->getBluetoothPairedDevices();
		},
		[this](std::vector<BluetoothDevice> btDevices)
		{
			LOG(LogDebug) << "GuiBluetoothPaired::onRefresh() - after execute 'ApiSystem::getInstance()->getBluetoothPairedDevices()'";
			Log::flush();
			load(btDevices);
			mWaitingLoad = false;
		}));
}

void GuiBluetoothPaired::onUnpairAll()
{
	if (mWaitingLoad || !isHasDevices())
		return;

	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO REMOVE ALL DEVICES?"),
		_("YES"), [this, window]
		{
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");

			window->pushGui(new GuiLoading<bool>(window, _("PLEASE WAIT..."),
				[this, audio_device]()
				{
					mWaitingLoad = true;
					bool result = ApiSystem::getInstance()->deleteAllBluetoothDevices();

					// reload BT audio device info
					std::string new_audio_device = ApiSystem::getInstance()->getBluetoothAudioDevice();
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
					mWaitingLoad = false;
					GuiBluetoothPaired::displayRestartDialog(window, _(msg), result, restar_ES);
				}));
		},
		_("NO"), nullptr));
}

void GuiBluetoothPaired::onClose()
{
	delete this;
}

std::string GuiBluetoothPaired::getDeviceName(const BluetoothDevice& btDevice) const
{
	std::string name = btDevice.name;
	if (Settings::getInstance()->getBool("bluetooth.use.alias") && !btDevice.alias.empty())
		name = btDevice.alias;

	return name;
}

void GuiBluetoothPaired::onUnpair()
{
	if (mWaitingLoad || !isHasDevices())
		return;

	std::string selected = mMenu.getSelected();
	if (selected.empty() || (mMapDevices.find(selected) == mMapDevices.end()))
		return;

	BluetoothDevice	btDevice = mMapDevices[selected];
	Window* window = mWindow;

	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO UNPAIR THE DEVICE?"),
		_("YES"), [this, window, btDevice]
		{
			std::string msg = _("UNPAIRING BLUETOOTH DEVICE") + " '" + getDeviceName(btDevice) + "'...";
			std::string audio_device = SystemConf::getInstance()->get("bluetooth.audio.device");
			window->pushGui(new GuiLoading<bool>(window, msg, 
				[this, btDevice, audio_device]
				{
					mWaitingLoad = true;

					bool result = ApiSystem::getInstance()->unpairBluetoothDevice(btDevice.id);				
					// successfully unpaired
					if (result)
					{
						// BT 4.2, only one audio device allowed
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
					std::string msg;

					if (result)
					{
						msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE SUCCESSFULLY UNPAIRED"));

						// audio bluetooth connecting changes
						restar_ES = (audio_device != SystemConf::getInstance()->get("bluetooth.audio.device") );
					}
					else
						msg.append("'").append(getDeviceName(btDevice)).append("' ").append(_("DEVICE FAILED TO UNPAIR"));

					mWaitingLoad = false;
					GuiBluetoothPaired::displayRestartDialog(window, msg, result, restar_ES);
				}));
		},
		_("NO"), nullptr));
}

void GuiBluetoothPaired::update(int deltaTime)
{
	if (mWaitingLoad)
		return;

	GuiComponent::update(deltaTime);

	if (mOKButton.isLongPressed(deltaTime))
	{
		if (isHasDevices())
			onUnpair();
	}
}
