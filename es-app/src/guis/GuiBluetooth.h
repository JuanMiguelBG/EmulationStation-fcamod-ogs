#pragma once
#ifndef ES_APP_GUIS_GUI_BLUETOOTH_H
#define ES_APP_GUIS_GUI_BLUETOOTH_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"

#include <thread>

class GuiBluetooth : public GuiComponent
{
public:
	GuiBluetooth(Window* window, const std::string title, const std::string subtitle = "", bool listPairedDevices = false);

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

protected:

	MenuComponent* getMenu() { return &mMenu; }

private:
	void	load(std::vector<std::string> bt_devices);

	bool	onSave(const std::string& value);
	void	onRefresh();
	void	onDeleteConnection();
	bool	isDeviceConnected(const std::string device);
	
	std::string getMenuItemSelected();

	MenuComponent mMenu;

	std::string mTitle;

	bool	mWaitingLoad;
	bool    mListPairedDevices;
};

#endif // ES_APP_GUIS_GUI_BLUETOOTH_H
