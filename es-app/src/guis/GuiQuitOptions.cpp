#include "guis/GuiQuitOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "Window.h"
#include "ApiSystem.h"


GuiQuitOptions::GuiQuitOptions(Window* window) : GuiSettings(window, _("\"QUIT\" SETTINGS").c_str())
{
	initializeMenu();
}

GuiQuitOptions::~GuiQuitOptions()
{
}

void GuiQuitOptions::initializeMenu()
{
	// full exit
	auto fullExitMenu = std::make_shared<SwitchComponent>(mWindow);
	fullExitMenu->setState(!Settings::getInstance()->getBool("ShowOnlyExit"));
	addWithLabel(_("VIEW MENU"), fullExitMenu);
	addSaveFunc([this, fullExitMenu]
		{
			bool old_value = !Settings::getInstance()->getBool("ShowOnlyExit");
			if (old_value != fullExitMenu->getState())
			{
				Settings::getInstance()->setBool("ShowOnlyExit", !fullExitMenu->getState());
				setVariable("reloadGuiMenu", true);
			}
		});

	// confirm to exit
	auto confirmToExit = std::make_shared<SwitchComponent>(mWindow);
	confirmToExit->setState(Settings::getInstance()->getBool("ConfirmToExit"));
	addWithLabel(_("CONFIRM TO \"QUIT\""), confirmToExit);
	addSaveFunc([this, confirmToExit]
		{
			bool old_value = Settings::getInstance()->getBool("ConfirmToExit");
			if (old_value != confirmToExit->getState())
			{
				Settings::getInstance()->setBool("ConfirmToExit", confirmToExit->getState());
				setVariable("reloadGuiMenu", true);
			}
		});

	// show quit menu with select
	auto showQuitMenu = std::make_shared<SwitchComponent>(mWindow);
	showQuitMenu->setState(Settings::getInstance()->getBool("ShowQuitMenuWithSelect"));
	addWithLabel(_("SHOW QUIT MENU WITH SELECT"), showQuitMenu);
	addSaveFunc([showQuitMenu]
		{
			Settings::getInstance()->setBool("ShowQuitMenuWithSelect", showQuitMenu->getState());
		});


	addGroup(_("NO MENU"));

	// only exit action
	std::string only_exit_action = Settings::getInstance()->getString("OnlyExitAction");
	auto only_exit_action_list = std::make_shared< OptionListComponent< std::string > >(mWindow, _("ACTION TO EXECUTE"), false);

	only_exit_action_list->add(_("shutdown"), "shutdown", only_exit_action == "shutdown");
	only_exit_action_list->add(_("suspend"), "suspend", only_exit_action == "suspend");
	only_exit_action_list->add(_("QUIT EMULATIONSTATION"), "exit_es", only_exit_action == "exit_es");

	addWithLabel(_("ACTION TO EXECUTE"), only_exit_action_list);
	addSaveFunc([this, only_exit_action_list]
		{
			std::string old_value = Settings::getInstance()->getString("OnlyExitAction");
			if (old_value != only_exit_action_list->getSelected())
			{
				Settings::getInstance()->setString("OnlyExitAction", only_exit_action_list->getSelected());
				setVariable("reloadGuiMenu", true);
			}
		});

	if (!only_exit_action_list->hasSelection())
	{
		only_exit_action_list->selectFirstItem();
		only_exit_action = only_exit_action_list->getSelected();
	}

	// confirm to exit
	auto show_action_menu = std::make_shared<SwitchComponent>(mWindow);
	show_action_menu->setState(Settings::getInstance()->getBool("ShowOnlyExitActionAsMenu"));
	addWithLabel(_("SHOW ACTION NAME AS MENU OPTION"), show_action_menu);
	addSaveFunc([this, show_action_menu]
		{
			bool old_value = Settings::getInstance()->getBool("ShowOnlyExitActionAsMenu");
			if (old_value != show_action_menu->getState())
			{
				Settings::getInstance()->setBool("ShowOnlyExitActionAsMenu", show_action_menu->getState());
				setVariable("reloadGuiMenu", true);
			}
		});
}
