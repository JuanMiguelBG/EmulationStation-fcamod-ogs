#include "guis/GuiSystemHotkeyEventsOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"


GuiSystemHotkeyEventsOptions::GuiSystemHotkeyEventsOptions(Window* window) : GuiSettings(window, _("SYSTEM HOTKEY EVENTS SETTINGS").c_str())
{
	initializeMenu(window);
}

GuiSystemHotkeyEventsOptions::~GuiSystemHotkeyEventsOptions()
{

}

void GuiSystemHotkeyEventsOptions::initializeMenu(Window* window)
{
	// brightness events
	bool brightness_value = ApiSystem::getInstance()->isSystemHotkeyBrightnessEvent();
	auto brightness = std::make_shared<SwitchComponent>(window, brightness_value);
	addWithLabel(_("BRIGHTNESS"), brightness);

	// Brightness step
	auto brightness_step = std::make_shared<SliderComponent>(mWindow, 1.0f, 25.f, 1.0f, "%");
	int brightness_step_value = ApiSystem::getInstance()->getSystemHotkeyBrightnessStep();
	brightness_step->setValue((float) brightness_step_value);
	addWithLabel(_("BRIGHTNESS STEP"), brightness_step);

	// volume events
	bool volume_value = ApiSystem::getInstance()->isSystemHotkeyVolumeEvent();
	auto volume = std::make_shared<SwitchComponent>(window, volume_value);
	addWithLabel(_("VOLUME"), volume);

	// Brightness step
	auto volume_step = std::make_shared<SliderComponent>(mWindow, 1.0f, 25.f, 1.0f, "%");
	int volume_step_value = ApiSystem::getInstance()->getSystemHotkeyVolumeStep();
	volume_step->setValue((float) volume_step_value);
	addWithLabel(_("VOLUME STEP"), volume_step);

	// wifi events
	bool wifi_value = ApiSystem::getInstance()->isSystemHotkeyWifiEvent();
	auto wifi = std::make_shared<SwitchComponent>(window, wifi_value);
	addWithLabel(_("WIFI"), wifi);

	// bluetooth events
	bool bluetooth_value = ApiSystem::getInstance()->isSystemHotkeyBluetoothEvent();
	auto bluetooth = std::make_shared<SwitchComponent>(window, bluetooth_value);
	addWithLabel(_("BLUETOOTH"), bluetooth);

	// suspend events
	bool suspend_value = ApiSystem::getInstance()->isSystemHotkeySuspendEvent();
	auto suspend = std::make_shared<SwitchComponent>(window, suspend_value);
	addWithLabel(_("SUSPEND"), suspend);

	addSaveFunc([brightness, brightness_value, brightness_step, brightness_step_value, volume, volume_value, volume_step, volume_step_value, wifi, wifi_value, bluetooth, bluetooth_value, suspend, suspend_value]
		{
			bool brightness_new_value = brightness->getState();
			int brightness_step_new_value = (int)Math::round( brightness_step->getValue() );
			bool volume_new_value = volume->getState();
			int volume_step_new_value = (int)Math::round( volume_step->getValue() );
			bool wifi_new_value = wifi->getState();
			bool bluetooth_new_value = bluetooth->getState();
			bool suspend_new_value = suspend->getState();

			if ( (brightness_value != brightness_new_value) || (brightness_step_value != brightness_step_new_value)
				|| (volume_value != volume_new_value) || (volume_step_value != volume_step_new_value)
				|| (wifi_value != wifi_new_value) || (bluetooth_value != bluetooth_new_value) || (suspend_value != suspend_new_value) )
			{
				ApiSystem::getInstance()->setSystemHotkeysValues( brightness_new_value, brightness_step_new_value, volume_new_value, volume_step_new_value, wifi_new_value, bluetooth_new_value, suspend_new_value );
			}
		});
}
