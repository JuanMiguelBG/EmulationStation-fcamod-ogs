#pragma once
#ifndef ES_APP_GUIS_GUI_WIFI_H
#define ES_APP_GUIS_GUI_WIFI_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"

#include <thread>

class GuiWifi : public GuiComponent
{
public:
	GuiWifi(Window* window, const std::string title, const std::string subtitle, std::string data, const std::function<bool(std::string)>& onsave);

	bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void	load(std::vector<std::string> ssids);

	bool	onSave(const std::string& value);
	void	onManualInput();
	void	onRefresh();

	MenuComponent mMenu;

	std::string mTitle;
	std::string mInitialData;

	std::function<bool(std::string)> mSaveFunction;

	bool		mWaitingLoad;
};

#endif // ES_APP_GUIS_GUI_WIFI_H
