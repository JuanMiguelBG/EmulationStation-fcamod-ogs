#pragma once
#ifndef ES_APP_GUIS_GUI_BLUETOOTH_ALIAS_H
#define ES_APP_GUIS_GUI_BLUETOOTH_ALIAS_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "ApiSystem.h"

#include <thread>

class GuiBluetoothAlias : public GuiComponent
{
public:
	GuiBluetoothAlias(Window* window, const std::string title, const std::string subtitle = "");

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

protected:

	MenuComponent* getMenu() { return &mMenu; }

private:
	void	load(std::vector<BluetoothDevice> bt_devices);

	bool	onManageDeviceAlias(const BluetoothDevice& btDeviced);
	void	onScan();

	MenuComponent mMenu;

	std::string mTitle;

	bool	mWaitingLoad;
	bool	hasDevices;
};

#endif // ES_APP_GUIS_GUI_BLUETOOTH_ALIAS_H
