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
	if (!Renderer::isSmallScreen())
	{
		// Center menus on the screen
		auto center_menus = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("CenterMenus"));
		addWithLabel(_("CENTER MENUS ON SCREEN"), center_menus);
		addSaveFunc([this, center_menus, window]
			{
				if (Settings::getInstance()->getBool("CenterMenus") != center_menus->getState())
				{
					Settings::getInstance()->setBool("CenterMenus", center_menus->getState());
					setVariable("reloadGuiMenu", true);
				}
			});


		// Auto adjust menu with by font size
		auto menu_auto_width = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("AutoMenuWidth"));
		addWithLabel(_("AUTO SIZE WIDTH BASED ON FONT SIZE"), menu_auto_width);
		addSaveFunc([this, menu_auto_width, window]
			{
				if (Settings::getInstance()->getBool("AutoMenuWidth") != menu_auto_width->getState())
				{
					Settings::getInstance()->setBool("AutoMenuWidth", menu_auto_width->getState());
					setVariable("reloadGuiMenu", true);
				}
			});
	}

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

	// Kodi
	addGroup(_("KODI SETTINGS"));

	auto kodi_enabled = std::make_shared<SwitchComponent>(window, SystemConf::getInstance()->getBool("kodi.enabled"));
	addWithLabel(_("ENABLE KODI"), kodi_enabled);
	addSaveFunc([this, kodi_enabled]
		{
			if (SystemConf::getInstance()->getBool("kodi.enabled") != kodi_enabled->getState())
			{
				SystemConf::getInstance()->setBool("kodi.enabled", kodi_enabled->getState());
				setVariable("reloadGuiMenu", true);
			}
		});

	auto kodi_boot = std::make_shared<SwitchComponent>(window, SystemConf::getInstance()->getBool("kodi.atstartup"));
	addWithLabel(_("LAUNCH KODI AT BOOT"), kodi_boot);
	addSaveFunc([kodi_boot]
		{
			SystemConf::getInstance()->setBool("kodi.atstartup", kodi_boot->getState());
		});

}
