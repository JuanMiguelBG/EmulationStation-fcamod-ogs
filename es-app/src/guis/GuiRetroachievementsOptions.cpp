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

	std::string username = SystemConf::getInstance()->get("global.retroachievements.username");
	std::string password = SystemConf::getInstance()->get("global.retroachievements.password");

	// retroachievements_enable
	bool retroachievementsEnabled = SystemConf::getInstance()->getBool("global.retroachievements");
	auto retroachievements_enabled = addSwitch(_("RETROACHIEVEMENTS"), "global.retroachievements", false);

	// retroachievements_hardcore_mode
	addSwitch(_("HARDCORE MODE"), "global.retroachievements.hardcore", false);

	// retroachievements_leaderboards
	addSwitch(_("LEADERBOARDS"), "global.retroachievements.leaderboards", false);

	// retroachievements_challenge_indicators
	addSwitch(_("CHALLENGE INDICATORS"), "global.retroachievements.challenge", false);

	// retroachievements_richpresence_enable
	addSwitch(_("RICH PRESENCE"), "global.retroachievements.richpresence", false);

	// retroachievements_badges_enable
	addSwitch(_("BADGES"), "global.retroachievements.badges", false);

	// retroachievements_test_unofficial
	addSwitch(_("TEST UNOFFICIAL ACHIEVEMENTS"), "global.retroachievements.test", false);

	// retroachievements_verbose_mode
	addSwitch(_("VERBOSE MODE"), "global.retroachievements.verbose", false);

	// retroachievements_automatic_screenshot
	addSwitch(_("AUTOMATIC SCREENSHOT"), "global.retroachievements.screenshot", false);

	// retroachievements_start_active
	addSwitch(_("ENCORE MODE (LOCAL RESET OF ACHIEVEMENTS)"), "global.retroachievements.start", false);

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
	addSaveFunc([currentSound, rsounds_choices]
		{
			std::string newSound = "none";
			if (rsounds_choices)
				newSound = rsounds_choices->getSelected();

			SystemConf::getInstance()->set("global.retroachievements.sound", newSound);
		});

	// retroachievements, username, password
	addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
	addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);

	addSaveFunc([window, retroachievementsEnabled, retroachievements_enabled, username, password]
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
	});

}
