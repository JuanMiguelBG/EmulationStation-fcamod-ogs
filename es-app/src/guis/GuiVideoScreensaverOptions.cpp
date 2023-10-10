#include "guis/GuiVideoScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Settings.h"

GuiVideoScreensaverOptions::GuiVideoScreensaverOptions(Window* window, const char* title) : GuiScreensaverOptions(window, title)
{
	// timeout to swap videos
	auto swap = std::make_shared<SliderComponent>(mWindow, 10.f, 1000.f, 1.f, "s");
	swap->setValue((float)(Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") / (1000)));
	addWithLabel(_("SWAP VIDEO AFTER (SECS)"), swap);
	addSaveFunc([swap] {
		int playNextTimeout = (int)Math::round(swap->getValue()) * (1000);
		if (Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") != playNextTimeout)
		{
			Settings::getInstance()->setInt("ScreenSaverSwapVideoTimeout", playNextTimeout);
			PowerSaver::updateTimeouts();
		}
	});

	// Render Video Game Name as subtitles
	auto ss_info = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SHOW GAME INFO"), false);
	std::vector<std::string> info_type;
	info_type.push_back("always");
	info_type.push_back("start & end");
	info_type.push_back("never");

	for (auto it = info_type.cbegin(); it != info_type.cend(); it++)
		ss_info->add(_(it->c_str()), *it, Settings::getInstance()->getString("ScreenSaverGameInfo") == *it);

	addWithLabel(_("SHOW GAME INFO ON SCREENSAVER"), ss_info);
	addSaveFunc([ss_info, this] { Settings::getInstance()->setString("ScreenSaverGameInfo", ss_info->getSelected()); });

	auto marquee_screensaver = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("ScreenSaverMarquee"));
	addWithLabel(_("USE MARQUEE AS GAME INFO"), marquee_screensaver);
	addSaveFunc([marquee_screensaver] { Settings::getInstance()->setBool("ScreenSaverMarquee", marquee_screensaver->getState()); });
/*
	auto decoration_screensaver = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("ScreenSaverDecoration"));
	addWithLabel(_("USE RANDOM DECORATION"), decoration_screensaver);
	addSaveFunc([decoration_screensaver] { Settings::getInstance()->setBool("ScreenSaverDecoration", decoration_screensaver->getState()); });
*/

	auto stretch_screensaver = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("StretchVideoOnScreenSaver"));
	addWithLabel(_("STRETCH VIDEO ON SCREENSAVER"), stretch_screensaver);
	addSaveFunc([stretch_screensaver] { Settings::getInstance()->setBool("StretchVideoOnScreenSaver", stretch_screensaver->getState()); });

	addGroup(_("CUSTOM VIDEOS SETTINGS"));
	// image source
	auto sss_custom_source = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("VideoScreenSaverCustomVideoSource"));
	addWithLabel(_("ENABLED"), sss_custom_source);
	addSaveFunc([sss_custom_source] { Settings::getInstance()->setBool("VideoScreenSaverCustomVideoSource", sss_custom_source->getState()); });

	// custom image directory
	std::shared_ptr<TextComponent> sss_video_dir = nullptr;

	if (Settings::getInstance()->getBool("ShowFileBrowser"))
		sss_video_dir = addBrowsablePath(_("DIRECTORY"), Settings::getInstance()->getString("VideoScreenSaverVideoDir"));
	else
		sss_video_dir = addEditableTextComponent(_("DIRECTORY"), Settings::getInstance()->getString("VideoScreenSaverVideoDir"));

	addSaveFunc([sss_video_dir] { Settings::getInstance()->setString("VideoScreenSaverVideoDir", sss_video_dir->getValue()); });

	// recurse custom video directory
	auto sss_recurse = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("VideoScreenSaverRecurse"));
	addWithLabel(_("RECURSIVE SEARCH"), sss_recurse);
	addSaveFunc([sss_recurse] {
		Settings::getInstance()->setBool("VideoScreenSaverRecurse", sss_recurse->getState());
	});

	// custom video filter
	auto sss_video_filter = addEditableTextComponent(_("FILTER"), Settings::getInstance()->getString("VideoScreenSaverVideoFilter"));
	addSaveFunc([sss_video_filter] { Settings::getInstance()->setString("VideoScreenSaverVideoFilter", sss_video_filter->getValue()); });
}

GuiVideoScreensaverOptions::~GuiVideoScreensaverOptions()
{
}

void GuiVideoScreensaverOptions::save()
{
	bool startingStatusNotRisky = (Settings::getInstance()->getString("ScreenSaverGameInfo") == "never");
	GuiScreensaverOptions::save();
}

