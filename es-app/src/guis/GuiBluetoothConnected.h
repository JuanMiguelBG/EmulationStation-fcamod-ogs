#pragma once
#ifndef ES_APP_GUIS_GUI_BLUETOOTH_CONNECTED_H
#define ES_APP_GUIS_GUI_BLUETOOTH_CONNECTED_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "ApiSystem.h"

#include <thread>

class GuiBluetoothConnected : public GuiComponent
{
public:
	GuiBluetoothConnected(Window* window, const std::string title, const std::string subtitle = "");

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

protected:

	MenuComponent* getMenu() { return &mMenu; }

private:
	void	load(std::vector<BluetoothDevice> bt_devices);

	bool	onDisconnectDevice(const BluetoothDevice& btDeviced);
	void	onRefresh();

	void	onDisconnectAll();

	void	displayRestartDialog(Window *window, const std::string message, bool deleteWindow, bool restarES);

	MenuComponent mMenu;

	std::string mTitle;

	bool	mWaitingLoad;
	bool	hasDevices;
};

#endif // ES_APP_GUIS_GUI_BLUETOOTH_CONNECTED_H
