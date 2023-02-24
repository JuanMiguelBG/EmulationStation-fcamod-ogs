#pragma once
#ifndef ES_APP_GUIS_KNOWED_GUI_WIFIS_H
#define ES_APP_GUIS_KNOWED_GUI_WIFIS_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"

#include <thread>

class GuiKnowedWifis : public GuiComponent
{
public:
	GuiKnowedWifis(Window* window, const std::string title, const std::string subtitle, const std::function<void(bool)>& closeFunction);

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void	load(std::vector<std::string> ssids);

	bool	onForget(const std::string& ssid);
	void	onRefresh();
	void	onClose();

	void	onForgetAll();

	MenuComponent mMenu;

	std::string mTitle;

	std::function<void(bool)> mCloseFunction;

	bool	mWaitingLoad;
	bool	hasDevices;
	bool	mForgetConnectedWifi;
	std::string	mConnectedWifiSsid;
};

#endif // ES_APP_GUIS_KNOWED_GUI_WIFIS_H
