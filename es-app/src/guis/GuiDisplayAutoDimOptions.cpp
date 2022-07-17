#include "guis/GuiDisplayAutoDimOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"
#include "PowerSaver.h"


GuiDisplayAutoDimOptions::GuiDisplayAutoDimOptions(Window* window) : GuiSettings(window, _("DISPLAY AUTO DIM SETTINGS").c_str())
{
	initializeMenu();
}

GuiDisplayAutoDimOptions::~GuiDisplayAutoDimOptions()
{
}

void GuiDisplayAutoDimOptions::initializeMenu()
{
	Window *window = mWindow;

	// enable display auto dim stay awake while charging
	bool stay_awake_charging_value = ApiSystem::getInstance()->isDisplayAutoDimStayAwakeCharging();
	auto stay_awake_charging = std::make_shared<SwitchComponent>(mWindow, stay_awake_charging_value);
	addWithLabel(_("STAY AWAKE WHILE CHARGING"), stay_awake_charging);

	// enable display auto dim by time
	bool auto_dim_time_value = ApiSystem::getInstance()->isDisplayAutoDimByTime();
	auto auto_dim_time = std::make_shared<SwitchComponent>(mWindow, auto_dim_time_value);
	addWithLabel(_("ENABLE"), auto_dim_time);

	// auto dim timeout
	auto auto_dim_time_timeout = std::make_shared<SliderComponent>(mWindow, 1.f, 120.f, 1.f, "m", true);
	int auto_dim_time_timeout_value = ApiSystem::getInstance()->getDisplayAutoDimTimeout();
	auto_dim_time_timeout->setValue( (float) auto_dim_time_timeout_value );
	addWithLabel(_("DIM AFTER"), auto_dim_time_timeout);

	// auto dim brightness level
	auto auto_dim_brightness_level = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%", true);
	int auto_dim_brightness_level_value = ApiSystem::getInstance()->getDisplayAutoDimBrightness();
	auto_dim_brightness_level->setValue( (float) auto_dim_brightness_level_value );
	addWithLabel(_("BRIGHTNESS"), auto_dim_brightness_level);

	addSaveFunc([this, window,
							stay_awake_charging, stay_awake_charging_value,
							auto_dim_time, auto_dim_time_value,
							auto_dim_time_timeout, auto_dim_time_timeout_value,
							auto_dim_brightness_level, auto_dim_brightness_level_value]
		{
			bool stay_awake_charging_new_value = stay_awake_charging->getState();
			bool auto_dim_time_new_value = auto_dim_time->getState();
			int auto_dim_time_timeout_new_value = (int) Math::round( auto_dim_time_timeout->getValue() );
			int auto_dim_brightness_level_new_value = (int) Math::round( auto_dim_brightness_level->getValue() );

			if ( (stay_awake_charging_value != stay_awake_charging_new_value)
					|| (auto_dim_time_value != auto_dim_time_new_value) || (auto_dim_time_timeout_value != auto_dim_time_timeout_new_value)
					|| (auto_dim_brightness_level_value != auto_dim_brightness_level_new_value))
			{
				manageDimScreenSaver(window, auto_dim_time_new_value);
				ApiSystem::getInstance()->setDisplayAutoDimValues( stay_awake_charging_new_value, auto_dim_time_new_value, auto_dim_time_timeout_new_value, auto_dim_brightness_level_new_value);
			}
		});

}

void GuiDisplayAutoDimOptions::manageDimScreenSaver(Window *window, bool auto_dim_enabled)
{
	if (auto_dim_enabled && Settings::getInstance()->getString("ScreenSaverBehavior") == "dim")
	{
		char strbuf[128];
		snprintf(strbuf, 128, _("THE '%s' SCREENSAVER WAS DISABLED. THE SCREENSAVER BEHAVIOR WAS SETTLED TO 'NONE'.").c_str(), Utils::String::toUpper(_("dim")).c_str());

		window->pushGui(new GuiMsgBox(window,
			strbuf,
			_("OK"), []
				{
					Settings::getInstance()->setString("ScreenSaverBehavior", "none");
					PowerSaver::updateTimeouts();
				}));
	}
}
