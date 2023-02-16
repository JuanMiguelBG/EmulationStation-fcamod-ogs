#pragma once
#ifndef ES_APP_GUIS_GUI_BLUETOOTH_SCAN_H
#define ES_APP_GUIS_GUI_BLUETOOTH_SCAN_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "ApiSystem.h"

#include <thread>

class GuiBluetoothScan : public GuiComponent
{
public:
	GuiBluetoothScan(Window* window, const std::string title, const std::string subtitle = "");

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

protected:

	MenuComponent* getMenu() { return &mMenu; }

private:
	void	load(std::vector<BluetoothDevice> bt_devices);

	bool	onConnectDevice(const BluetoothDevice& btDeviced);
	void	onScan();

	void	displayRestartDialog(Window *window, bool deleteWindow, bool restarES);

	std::string getDeviceName(const BluetoothDevice& btDevice) const;
	void handleAlias(Window *window, bool deleteWindow, bool restarES, const std::string deviceData);

	MenuComponent mMenu;

	std::string mTitle;

	bool	mWaitingLoad;
	bool	hasDevices;
};

#endif // ES_APP_GUIS_GUI_BLUETOOTH_SCAN_H
