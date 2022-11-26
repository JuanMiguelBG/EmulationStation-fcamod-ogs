#pragma once
#ifndef ES_APP_GUIS_GUI_BLUETOOTH_PAIRED_H
#define ES_APP_GUIS_GUI_BLUETOOTH_PAIRED_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "ApiSystem.h"

#include <thread>

class GuiBluetoothPaired : public GuiComponent
{
public:
	GuiBluetoothPaired(Window* window, const std::string title, const std::string subtitle = "");

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

protected:

	MenuComponent* getMenu() { return &mMenu; }

private:
	void	load(std::vector<BluetoothDevice> bt_devices);

	bool	onAction(const BluetoothDevice& btDeviced);
	bool	onConnectDevice(const BluetoothDevice& btDeviced);
	bool	onDisconnectDevice(const BluetoothDevice& btDeviced);
	void	onRefresh();

	void	onDeleteAll();

	void	displayRestartDialog(Window *window, const std::string message, bool deleteWindow, bool restarES);

	MenuComponent mMenu;

	std::string mTitle;

	bool	mWaitingLoad;
	bool	hasDevices;
};

#endif // ES_APP_GUIS_GUI_BLUETOOTH_PAIRED_H
