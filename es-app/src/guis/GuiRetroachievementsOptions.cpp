#include "guis/GuiRetroachievementsOptions.h"

#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"
#include "SystemConf.h"
#include "RetroAchievements.h"


GuiRetroachievementsOptions::GuiRetroachievementsOptions(Window* window) : GuiSettings(window, _("RETROACHIEVEMENTS SETTINGS").c_str())
{
	initializeMenu();
}

GuiRetroachievementsOptions::~GuiRetroachievementsOptions()
{
}

void GuiRetroachievementsOptions::initializeMenu()
{
	Window *window = mWindow;

	//addGroup(_("SETTINGS"));

	std::string username = ApiSystem::getInstance()->getRetroachievementsUsername();
	std::string password = ApiSystem::getInstance()->getRetroachievementsPassword();

	// retroachievements_enable
	bool retroachievementsEnabled = ApiSystem::getInstance()->getRetroachievementsEnabled();
	auto retroachievements_enabled = std::make_shared<SwitchComponent>(mWindow, retroachievementsEnabled);
	addWithLabel(_("RETROACHIEVEMENTS"), retroachievements_enabled);

	// retroachievements_hardcore_mode
	bool retroachievements_hardcore_enabled_value = ApiSystem::getInstance()->getRetroachievementsHardcoreEnabled();
	auto retroachievements_hardcore_enabled = std::make_shared<SwitchComponent>(mWindow, retroachievements_hardcore_enabled_value);
	addWithLabel(_("HARDCORE MODE"), retroachievements_hardcore_enabled);

	// retroachievements_leaderboards
	bool retroachievements_leaderboards_enabled_value = ApiSystem::getInstance()->getRetroachievementsLeaderboardsEnabled();
	auto retroachievements_leaderboards_enabled = std::make_shared<SwitchComponent>(mWindow, retroachievements_leaderboards_enabled_value);
	addWithLabel(_("LEADERBOARDS"), retroachievements_leaderboards_enabled);

	// retroachievements_challenge_indicators
	bool retroachievements_challenge_indicators_value = ApiSystem::getInstance()->getRetroachievementsChallengeIndicators();
	auto retroachievements_challenge_indicators = std::make_shared<SwitchComponent>(mWindow, retroachievements_challenge_indicators_value);
	addWithLabel(_("CHALLENGE INDICATORS"), retroachievements_challenge_indicators);

	// retroachievements_richpresence_enable
	bool retroachievements_richpresence_enable_value = ApiSystem::getInstance()->getRetroachievementsRichpresenceEnable();
	auto retroachievements_richpresence_enable = std::make_shared<SwitchComponent>(mWindow, retroachievements_richpresence_enable_value);
	addWithLabel(_("RICH PRESENCE"), retroachievements_richpresence_enable);

	// retroachievements_badges_enable
	bool retroachievements_badges_enable_value = ApiSystem::getInstance()->getRetroachievementsBadgesEnable();
	auto retroachievements_badges_enable = std::make_shared<SwitchComponent>(mWindow, retroachievements_badges_enable_value);
	addWithLabel(_("BADGES"), retroachievements_badges_enable);

	// retroachievements_test_unofficial
	bool retroachievements_test_unofficial_value = ApiSystem::getInstance()->getRetroachievementsTestUnofficial();
	auto retroachievements_test_unofficial = std::make_shared<SwitchComponent>(mWindow, retroachievements_test_unofficial_value);
	addWithLabel(_("TEST UNOFFICIAL ACHIEVEMENTS"), retroachievements_test_unofficial);

	// retroachievements_verbose_mode
	bool retroachievements_verbose_enabled_value = ApiSystem::getInstance()->getRetroachievementsVerboseEnabled();
	auto retroachievements_verbose_enabled = std::make_shared<SwitchComponent>(mWindow, retroachievements_verbose_enabled_value);
	addWithLabel(_("VERBOSE MODE"), retroachievements_verbose_enabled);

	// retroachievements_automatic_screenshot
	bool retroachievements_screenshot_enabled_value = ApiSystem::getInstance()->getRetroachievementsAutomaticScreenshotEnabled();
	auto retroachievements_screenshot_enabled = std::make_shared<SwitchComponent>(mWindow, retroachievements_screenshot_enabled_value);
	addWithLabel(_("AUTOMATIC SCREENSHOT"), retroachievements_screenshot_enabled);

	// retroachievements_start_active
	bool retroachievements_start_active_value = ApiSystem::getInstance()->getRetroachievementsStartActive();
	auto retroachievements_start_active = std::make_shared<SwitchComponent>(mWindow, retroachievements_start_active_value);
	addWithLabel(_("ENCORE MODE (LOCAL RESET OF ACHIEVEMENTS)"), retroachievements_start_active);

	// Unlock sound
	auto installed_sounds = ApiSystem::getInstance()->getRetroachievementsSoundsList();
	std::shared_ptr<OptionListComponent<std::string> > rsounds_choices;
	std::string currentSound = SystemConf::getInstance()->get("global.retroachievements.sound");

	if (installed_sounds.size() > 0)
	{
		rsounds_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("RETROACHIEVEMENT UNLOCK SOUND"), false);
		rsounds_choices->add(_(Utils::String::toUpper("none")), "none", currentSound.empty() || currentSound == "none");

		for (auto snd : installed_sounds)
			rsounds_choices->add(_(Utils::String::toUpper(snd).c_str()), snd, currentSound == snd);

		if (!rsounds_choices->hasSelection())
			rsounds_choices->selectFirstItem();

		addWithLabel(_("UNLOCK SOUND"), rsounds_choices);
	}

	// retroachievements, username, password
	addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
	addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);

	addSaveFunc([window, retroachievementsEnabled, retroachievements_enabled, username, password,
				retroachievements_hardcore_enabled, retroachievements_hardcore_enabled_value,
				retroachievements_leaderboards_enabled, retroachievements_leaderboards_enabled_value,
				retroachievements_challenge_indicators, retroachievements_challenge_indicators_value,
				retroachievements_richpresence_enable, retroachievements_richpresence_enable_value,
				retroachievements_badges_enable, retroachievements_badges_enable_value,
				retroachievements_test_unofficial, retroachievements_test_unofficial_value,
				retroachievements_verbose_enabled, retroachievements_verbose_enabled_value,
				retroachievements_screenshot_enabled, retroachievements_screenshot_enabled_value,
				retroachievements_start_active, retroachievements_start_active_value,
				rsounds_choices, currentSound]
	{
		bool newState = retroachievements_enabled->getState();

		std::string newUsername = SystemConf::getInstance()->get("global.retroachievements.username");
		std::string newPassword = SystemConf::getInstance()->get("global.retroachievements.password");

		if (newState && ( !retroachievementsEnabled || (username != newUsername) || (password != newPassword) ))
		{
			std::string error;
			if (!RetroAchievements::testAccount(newUsername, newPassword, error))
			{
				window->pushGui(new GuiMsgBox(window, _("UNABLE TO ACTIVATE RETROACHIEVEMENTS") + ":\n" + error, _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
					retroachievements_enabled->setState(false);
					newState = false;
			}
		}

		std::string newSound = "none";
		if (rsounds_choices)
			newSound = rsounds_choices->getSelected();

		SystemConf::getInstance()->set("global.retroachievements.sound", newSound);

		if ( (retroachievementsEnabled != newState)
				|| (retroachievements_hardcore_enabled_value != retroachievements_hardcore_enabled->getState())
				|| (retroachievements_leaderboards_enabled_value != retroachievements_leaderboards_enabled->getState())
				|| (retroachievements_challenge_indicators_value != retroachievements_challenge_indicators->getState())
				|| (retroachievements_richpresence_enable_value != retroachievements_richpresence_enable->getState())
				|| (retroachievements_badges_enable_value != retroachievements_badges_enable->getState())
				|| (retroachievements_test_unofficial_value != retroachievements_test_unofficial->getState())
				|| (retroachievements_verbose_enabled_value != retroachievements_verbose_enabled->getState())
				|| (retroachievements_screenshot_enabled_value != retroachievements_screenshot_enabled->getState())
				|| (retroachievements_start_active_value != retroachievements_start_active->getState())
				|| (currentSound != newSound)
				|| (username != newUsername)
				|| (password != newPassword) )
		{
			ApiSystem::getInstance()->setRetroachievementsValues(newState, retroachievements_hardcore_enabled->getState(), retroachievements_leaderboards_enabled->getState(), retroachievements_verbose_enabled->getState(), retroachievements_screenshot_enabled->getState(), retroachievements_challenge_indicators->getState(), retroachievements_richpresence_enable->getState(), retroachievements_badges_enable->getState(), retroachievements_test_unofficial->getState(), retroachievements_start_active->getState(), newSound, newUsername, newPassword);
		}
	});

}
