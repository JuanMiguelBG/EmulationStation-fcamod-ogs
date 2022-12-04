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
	SystemHotkeyValues hk_values = ApiSystem::getInstance()->getSystemHotkeyValues();

	// brightness events
	auto brightness = std::make_shared<SwitchComponent>(window, hk_values.brightness);
	addWithLabel(_("BRIGHTNESS"), brightness);

	// brightness step
	auto brightness_step = std::make_shared<SliderComponent>(mWindow, 1.0f, 25.f, 1.0f, "%");
	brightness_step->setValue((float) hk_values.brightness_step);
	addWithLabel(_("BRIGHTNESS STEP"), brightness_step);

	// volume events
	auto volume = std::make_shared<SwitchComponent>(window, hk_values.volume);
	addWithLabel(_("VOLUME"), volume);

	// volume step
	float volume_start = 1.0f;
	int volume_step_value = hk_values.volume_step;
	if (Settings::getInstance()->getBool("BluetoothAudioConnected"))
	{
		volume_start = 2.0f;
		if (hk_values.volume_step == 1)
			hk_values.volume_step = 2;
	}

	auto volume_step = std::make_shared<SliderComponent>(mWindow, volume_start, 25.f, 1.0f, "%");
	
	volume_step->setValue((float) hk_values.volume_step);
	addWithLabel(_("VOLUME STEP"), volume_step);

	// wifi events
	auto wifi = std::make_shared<SwitchComponent>(window, hk_values.wifi);
	addWithLabel(_("WIFI"), wifi);

	// bluetooth events
	auto bluetooth = std::make_shared<SwitchComponent>(window, hk_values.bluetooth);
	addWithLabel(_("BLUETOOTH"), bluetooth);

	// speaker events
	auto speaker = std::make_shared<SwitchComponent>(window, hk_values.speaker);
	addWithLabel(_("SPEAKER"), speaker);

	// suspend events
	auto suspend = std::make_shared<SwitchComponent>(window, hk_values.suspend);
	addWithLabel(_("SUSPEND"), suspend);

	addSaveFunc([&hk_values, brightness, brightness_step, volume, volume_step, wifi, bluetooth, speaker, suspend]
		{
			bool brightness_new_value = brightness->getState();
			int brightness_step_new_value = (int)Math::round( brightness_step->getValue() );
			bool volume_new_value = volume->getState();
			int volume_step_new_value = (int)Math::round( volume_step->getValue() );
			bool wifi_new_value = wifi->getState();
			bool bluetooth_new_value = bluetooth->getState();
			bool speaker_new_value = speaker->getState();
			bool suspend_new_value = suspend->getState();

			if ( (hk_values.brightness != brightness_new_value) || (hk_values.brightness_step != brightness_step_new_value)
				|| (hk_values.volume != volume_new_value) || (hk_values.volume_step != volume_step_new_value)
				|| (hk_values.wifi != wifi_new_value) || (hk_values.bluetooth != bluetooth_new_value)
				|| (hk_values.speaker != speaker_new_value) || (hk_values.suspend != suspend_new_value) )
			{
				hk_values.brightness = brightness_new_value;
				hk_values.brightness_step = brightness_step_new_value;
				hk_values.volume = volume_new_value;
				hk_values.volume_step = volume_step_new_value;
				hk_values.wifi = wifi_new_value;
				hk_values.bluetooth = bluetooth_new_value;
				hk_values.speaker = speaker_new_value;
				hk_values.suspend = suspend_new_value;

				ApiSystem::getInstance()->setSystemHotkeysValues( hk_values );
			}
		});
}
