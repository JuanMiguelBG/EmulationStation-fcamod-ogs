#include "guis/GuiGeneralScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSlideshowScreensaverOptions.h"
#include "guis/GuiVideoScreensaverOptions.h"
#include "Settings.h"
#include "ApiSystem.h"

GuiGeneralScreensaverOptions::GuiGeneralScreensaverOptions(Window* window, std::string title) : GuiScreensaverOptions(window, title)
{
	auto theme = ThemeData::getMenuTheme();

	// screensaver time
	auto screensaver_time = std::make_shared<SliderComponent>(mWindow, 1.f, 120.0f, 1.f, "m");
	screensaver_time->setValue((float)(Settings::getInstance()->getInt("ScreenSaverTime") / (1000 * 60)));
	addWithLabel(_("SCREENSAVER AFTER"), screensaver_time);
	addSaveFunc([screensaver_time] {
			Settings::getInstance()->setInt("ScreenSaverTime", (int)Math::round(screensaver_time->getValue()) * (1000 * 60));
			PowerSaver::updateTimeouts();
	});

	// screensaver behavior
	std::string screensaver_behavior_value = Settings::getInstance()->getString("ScreenSaverBehavior");
	bool isDisplayAutoDim = ApiSystem::getInstance()->isDisplayAutoDimByTime();
	bool isDeviceAutoSuspend = ApiSystem::getInstance()->isDeviceAutoSuspend();

	if ((screensaver_behavior_value == "suspend") && isDeviceAutoSuspend)
		screensaver_behavior_value = "none";

	if ((screensaver_behavior_value == "dim") && isDisplayAutoDim)
		screensaver_behavior_value = "none";

	auto screensaver_behavior = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SCREENSAVER BEHAVIOR"), false);
	std::vector<std::string> screensavers;

	screensavers.push_back("none");

	if (!isDisplayAutoDim)
		screensavers.push_back("dim");

	screensavers.push_back("black");
	screensavers.push_back("random video");
	screensavers.push_back("slideshow");

	if (!isDeviceAutoSuspend)
		screensavers.push_back("suspend");

	for(auto it = screensavers.cbegin(); it != screensavers.cend(); it++)
		screensaver_behavior->add(_(it->c_str()), *it, screensaver_behavior_value == *it);

	if (!screensaver_behavior->hasSelection())
		screensaver_behavior->selectFirstItem();

	addWithLabel(_("SCREENSAVER BEHAVIOR"), screensaver_behavior);
	addSaveFunc([this, screensaver_behavior] {
		if (Settings::getInstance()->getString("ScreenSaverBehavior") != "random video"
			&& screensaver_behavior->getSelected() == "random video")
		{
			// if before it wasn't risky but now there's a risk of problems, show warning
			mWindow->pushGui(new GuiMsgBox(mWindow,
				_("THE \"RANDOM VIDEO\" SCREENSAVER SHOWS VIDEOS FROM YOUR GAMELIST.\nIF YOU DON'T HAVE VIDEOS, OR IF NONE OF THEM CAN BE PLAYED AFTER A FEW ATTEMPTS, IT WILL DEFAULT TO \"BLACK\".\nMORE OPTIONS IN THE \"UI SETTINGS\" -> \"RANDOM VIDEO SCREENSAVER SETTINGS\" MENU."),
				_("OK"), [] { return; }));
		}

		Settings::getInstance()->setString("ScreenSaverBehavior", screensaver_behavior->getSelected());
		PowerSaver::updateTimeouts();
	});

	// Screensaver stops music
	if (Settings::getInstance()->getBool("audio.bgmusic"))
	{
		auto ctlStopMusic = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("StopMusicOnScreenSaver"));
		addWithLabel(_("STOP MUSIC ON SCREENSAVER"), ctlStopMusic);
		addSaveFunc([ctlStopMusic] { Settings::getInstance()->setBool("StopMusicOnScreenSaver", ctlStopMusic->getState()); });
	}

	ComponentListRow row;

	// show filtered menu
	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, _("VIDEO SCREENSAVER SETTINGS"), theme->Text.font, theme->Text.color), true);
	row.addElement(makeArrow(mWindow), false);
	row.makeAcceptInputHandler(std::bind(&GuiGeneralScreensaverOptions::openVideoScreensaverOptions, this));
	addRow(row);

	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, _("SLIDESHOW SCREENSAVER SETTINGS"), theme->Text.font, theme->Text.color), true);
	row.addElement(makeArrow(mWindow), false);
	row.makeAcceptInputHandler(std::bind(&GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions, this));
	addRow(row);

	// Allow ScreenSaver Controls - ScreenSaverControls
	auto ss_controls = std::make_shared<SwitchComponent>(mWindow, Settings::getInstance()->getBool("ScreenSaverControls"));
	addWithLabel(_("SCREENSAVER CONTROLS"), ss_controls);
	addSaveFunc([ss_controls] { Settings::getInstance()->setBool("ScreenSaverControls", ss_controls->getState()); });
}

GuiGeneralScreensaverOptions::~GuiGeneralScreensaverOptions()
{
}

void GuiGeneralScreensaverOptions::openVideoScreensaverOptions() {
	mWindow->pushGui(new GuiVideoScreensaverOptions(mWindow, _("VIDEO SCREENSAVER").c_str()));
}

void GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions() {
	mWindow->pushGui(new GuiSlideshowScreensaverOptions(mWindow, _("SLIDESHOW SCREENSAVER").c_str()));
}

