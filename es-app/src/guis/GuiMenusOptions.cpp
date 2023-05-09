#include "guis/GuiMenusOptions.h"

#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"
#include "SystemConf.h"


GuiMenusOptions::GuiMenusOptions(Window* window) : GuiSettings(window, _("MENUS SETTINGS").c_str())
{
	initializeMenu(window);
}

GuiMenusOptions::~GuiMenusOptions()
{
}

void GuiMenusOptions::initializeMenu(Window* window)
{
	// animated main menu
	auto animated_main_menu = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("AnimatedMainMenu"));
	addWithLabel(_("OPEN MAIN MENU WITH ANIMATION"), animated_main_menu);
	addSaveFunc([animated_main_menu]
		{
			Settings::getInstance()->setBool("AnimatedMainMenu", animated_main_menu->getState());
		});

	// Autoscroll text of the menu options
	auto autoscroll_menu_entry = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("AutoscrollMenuEntries"));
	addWithLabel(_("AUTOSCROLL MENU ENTRIES"), autoscroll_menu_entry);
	addSaveFunc([this, autoscroll_menu_entry]
		{
			if (Settings::getInstance()->getBool("AutoscrollMenuEntries") != autoscroll_menu_entry->getState())
			{
				Settings::getInstance()->setBool("AutoscrollMenuEntries", autoscroll_menu_entry->getState());
				setVariable("reloadGuiMenu", true);
			}
		});

}
