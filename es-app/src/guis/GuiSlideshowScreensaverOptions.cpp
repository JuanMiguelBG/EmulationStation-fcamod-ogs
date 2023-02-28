#include "guis/GuiSlideshowScreensaverOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "utils/StringUtil.h"
#include "Settings.h"
#include "Window.h"

GuiSlideshowScreensaverOptions::GuiSlideshowScreensaverOptions(Window* window, const char* title) : GuiScreensaverOptions(window, title)
{
	ComponentListRow row;

	// image duration (seconds)
	auto sss_image_sec = std::make_shared<SliderComponent>(mWindow, 1.f, 60.f, 1.f, "s");
	sss_image_sec->setValue((float)(Settings::getInstance()->getInt("ScreenSaverSwapImageTimeout") / (1000)));
	addWithLabel(row, _("SWAP IMAGE AFTER (SECS)"), sss_image_sec);
	addSaveFunc([sss_image_sec] {
		int playNextTimeout = (int)Math::round(sss_image_sec->getValue()) * (1000);
		Settings::getInstance()->setInt("ScreenSaverSwapImageTimeout", playNextTimeout);
		PowerSaver::updateTimeouts();
	});
	
	// SHOW GAME NAME
	auto ss_controls = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("SlideshowScreenSaverGameName"));
	addWithLabel(row, _("SHOW GAME INFO"), ss_controls);
	addSaveFunc([ss_controls] { Settings::getInstance()->setBool("SlideshowScreenSaverGameName", ss_controls->getState()); });

	auto marquee_screensaver = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("ScreenSaverMarquee"));
	addWithLabel(row, _("USE MARQUEE AS GAME INFO"), marquee_screensaver);
	addSaveFunc([marquee_screensaver] { Settings::getInstance()->setBool("ScreenSaverMarquee", marquee_screensaver->getState()); });
	/*
	auto decoration_screensaver = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("ScreenSaverDecoration"));
	addWithLabel(row, _("USE RANDOM DECORATION"), decoration_screensaver);
	addSaveFunc([decoration_screensaver] { Settings::getInstance()->setBool("ScreenSaverDecoration", decoration_screensaver->getState()); });
	*/
	// stretch
	auto sss_stretch = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("SlideshowScreenSaverStretch"));
	addWithLabel(row, _("STRETCH IMAGES"), sss_stretch);
	addSaveFunc([sss_stretch] {
		Settings::getInstance()->setBool("SlideshowScreenSaverStretch", sss_stretch->getState());
	});

	/*
	// background audio file
	auto sss_bg_audio_file = std::make_shared<TextComponent>(mWindow, "", ThemeData::getMenuTheme()->TextSmall.font, ThemeData::getMenuTheme()->TextSmall.color);
	addEditableTextComponent(row, "BACKGROUND AUDIO", sss_bg_audio_file, Settings::getInstance()->getString("SlideshowScreenSaverBackgroundAudioFile"));
	addSaveFunc([sss_bg_audio_file] {
		Settings::getInstance()->setString("SlideshowScreenSaverBackgroundAudioFile", sss_bg_audio_file->getValue());
	});
	*/
	// image source
	auto sss_custom_source = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("SlideshowScreenSaverCustomImageSource"));
	addWithLabel(row, _("USE CUSTOM IMAGES"), sss_custom_source);
	addSaveFunc([sss_custom_source] { Settings::getInstance()->setBool("SlideshowScreenSaverCustomImageSource", sss_custom_source->getState()); });

	// custom image directory
	std::shared_ptr<TextComponent> sss_image_dir = nullptr;

	if (Settings::getInstance()->getBool("ShowFileBrowser"))
		sss_image_dir = addBrowsablePath(_("CUSTOM IMAGE DIR"), Settings::getInstance()->getString("SlideshowScreenSaverImageDir"));
	else
		sss_image_dir = addEditableTextComponent(_("CUSTOM IMAGE DIR"), Settings::getInstance()->getString("SlideshowScreenSaverImageDir"));

	addSaveFunc([sss_image_dir] { Settings::getInstance()->setString("SlideshowScreenSaverImageDir", sss_image_dir->getValue()); });

	// recurse custom image directory
	auto sss_recurse = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("SlideshowScreenSaverRecurse"));
	addWithLabel(row, _("CUSTOM IMAGE DIR RECURSIVE"), sss_recurse);
	addSaveFunc([sss_recurse] {
		Settings::getInstance()->setBool("SlideshowScreenSaverRecurse", sss_recurse->getState());
	});

	// custom image filter
	auto sss_image_filter = addEditableTextComponent(_("CUSTOM IMAGE FILTER"), Settings::getInstance()->getString("SlideshowScreenSaverImageFilter"));
	addSaveFunc([sss_image_filter] { Settings::getInstance()->setString("SlideshowScreenSaverImageFilter", sss_image_filter->getValue()); });
}

GuiSlideshowScreensaverOptions::~GuiSlideshowScreensaverOptions()
{
}

void GuiSlideshowScreensaverOptions::addWithLabel(ComponentListRow row, const std::string label, std::shared_ptr<GuiComponent> component)
{
	row.elements.clear();

	auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
	lbl->setAutoScroll(Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	row.addElement(lbl, true); // label

	row.addElement(component, false, true);

	addRow(row);
}
