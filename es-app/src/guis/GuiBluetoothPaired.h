#pragma once
#ifndef ES_APP_GUIS_GUI_BLUETOOTH_PAIRED_H
#define ES_APP_GUIS_GUI_BLUETOOTH_PAIRED_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "ApiSystem.h"
#include "MultiStateInput.h"
#include <map>

#include <thread>

class GuiBluetoothPaired : public GuiComponent
{
public:
	GuiBluetoothPaired(Window* window, const std::string title, const std::string subtitle = "", bool onlyUnpair = false);
	~GuiBluetoothPaired();

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	void update(int deltaTime);

protected:

	MenuComponent* getMenu() { return &mMenu; }

private:
	void	load(std::vector<BluetoothDevice> bt_devices);

	void	onAction();
	void	onUnpair();
	void	onConnectDevice(const BluetoothDevice& btDeviced);
	void	onDisconnectDevice(const BluetoothDevice& btDeviced);
	void	onClose();
	void	onRefresh();
	void	onUnpairAll();

	void	displayRestartDialog(Window *window, const std::string message, bool deleteWindow, bool restarES);

	bool isHasDevices() { return mMapDevices.size() > 0; };

	std::string getDeviceName(const BluetoothDevice& btDevice) const;

	MenuComponent mMenu;

	std::string mTitle;

	bool	mWaitingLoad;
	bool	mOnlyUnpair;

	MultiStateInput mOKButton;
	std::map<std::string, BluetoothDevice> mMapDevices;
};

#endif // ES_APP_GUIS_GUI_BLUETOOTH_PAIRED_H
