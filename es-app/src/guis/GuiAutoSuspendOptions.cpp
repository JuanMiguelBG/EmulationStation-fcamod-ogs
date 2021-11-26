#include "guis/GuiAutoSuspendOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"
#include "PowerSaver.h"


GuiAutoSuspendOptions::GuiAutoSuspendOptions(Window* window) : GuiSettings(window, _("DEVICE AUTO SUSPEND SETTINGS").c_str())
{
	initializeMenu();
}

GuiAutoSuspendOptions::~GuiAutoSuspendOptions()
{
}

void GuiAutoSuspendOptions::initializeMenu()
{
	Window *window = mWindow;

	addGroup(_("BY TIME"));

	// enable device auto suspend by time
	auto auto_suspend_time = std::make_shared<SwitchComponent>(mWindow);
	bool auto_suspend_time_value = ApiSystem::getInstance()->isDeviceAutoSuspendByTime();
	auto_suspend_time->setState( auto_suspend_time_value );
	addWithLabel(_("ENABLE"), auto_suspend_time);

	// auto suspend timeout
	auto auto_suspend_time_timeout = std::make_shared<SliderComponent>(mWindow, 1.f, 120.f, 1.f, "m", true);
	int auto_suspend_time_timeout_value = ApiSystem::getInstance()->getAutoSuspendTimeout();
	auto_suspend_time_timeout->setValue( (float) auto_suspend_time_timeout_value );
	addWithLabel(_("SUSPEND AFTER"), auto_suspend_time_timeout);


	addGroup(_("BY BATTERY LEVEL"));

	// enable device auto suspend by battery level
	auto auto_suspend_battery = std::make_shared<SwitchComponent>(mWindow);
	bool auto_suspend_battery_value = ApiSystem::getInstance()->isDeviceAutoSuspendByBatteryLevel();
	auto_suspend_battery->setState( auto_suspend_battery_value );
	addWithLabel(_("ENABLE"), auto_suspend_battery);

	// auto suspend battery level
	auto auto_suspend_battery_level = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%", true);
	int auto_suspend_battery_level_value = ApiSystem::getInstance()->getAutoSuspendBatteryLevel();
	auto_suspend_battery_level->setValue( (float) auto_suspend_battery_level_value );
	addWithLabel(_("SUSPEND WITH LEVEL"), auto_suspend_battery_level);

	addSaveFunc([this, window, auto_suspend_time, auto_suspend_time_value, auto_suspend_time_timeout, auto_suspend_time_timeout_value, auto_suspend_battery, auto_suspend_battery_value, auto_suspend_battery_level, auto_suspend_battery_level_value]
		{
			bool auto_suspend_time_new_value = auto_suspend_time->getState();
			int auto_suspend_time_timeout_new_value = (int) Math::round( auto_suspend_time_timeout->getValue() );
			bool auto_suspend_battery_new_value = auto_suspend_battery->getState();
			int auto_suspend_battery_level_new_value = (int) Math::round( auto_suspend_battery_level->getValue() );

			if ( (auto_suspend_time_value != auto_suspend_time_new_value) || (auto_suspend_time_timeout_value != auto_suspend_time_timeout_new_value)
					|| (auto_suspend_battery_value != auto_suspend_battery_new_value) || (auto_suspend_battery_level_value != auto_suspend_battery_level_new_value))
			{
				manageSuspendScreenSaver(window, auto_suspend_time_new_value || auto_suspend_battery_new_value);
				ApiSystem::getInstance()->setDeviceAutoSuspendValues( auto_suspend_time_new_value, auto_suspend_time_timeout_new_value, auto_suspend_battery_new_value, auto_suspend_battery_level_new_value);
			}
		});

}

void GuiAutoSuspendOptions::manageSuspendScreenSaver(Window *window, bool auto_suspend_enabled)
{
	if (auto_suspend_enabled && Settings::getInstance()->getString("ScreenSaverBehavior") == "suspend")
	{
		window->pushGui(new GuiMsgBox(window,
			_("THE \"SUSPEND\" SCREENSAVER WAS DISABLED. THE SCREENSAVER BEHAVIOR WAS SETTLED TO \"DIM\"."),
			_("OK"), []
				{
					Settings::getInstance()->setString("ScreenSaverBehavior", "dim");
					PowerSaver::updateTimeouts();
				}));
	}
}
