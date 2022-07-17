#include "guis/GuiMenusOptions.h"

#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "ApiSystem.h"


GuiMenusOptions::GuiMenusOptions(Window* window) : GuiSettings(window, _("MENUS SETTINGS").c_str())
{
	initializeMenu(window);
}

GuiMenusOptions::~GuiMenusOptions()
{
}

void GuiMenusOptions::initializeMenu(Window* window)
{
	// menus on top
	auto menusOnTop = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("MenusOnDisplayTop"));
	addWithLabel(_("ON THE TOP OF THE DISPLAY"), menusOnTop);
	addSaveFunc([this, menusOnTop, window]
		{
			bool old_value = Settings::getInstance()->getBool("MenusOnDisplayTop");
			if (old_value != menusOnTop->getState())
			{
				Settings::getInstance()->setBool("MenusOnDisplayTop", menusOnTop->getState());
				if (menusOnTop->getState() && Settings::getInstance()->getBool("MenusAllWidth"))
				{
					
					window->pushGui(new GuiMsgBox(window, _("YOU SELECTED TO SHOW ON THE TOP OF THE DISPLAY, THE MENU \"FILLS THE ALL HEIGHT\" OPTION IS DISABLED."),
						_("OK"), [] { Settings::getInstance()->setBool("MenusAllHeight", false); }));
				}
				setVariable("reloadGuiMenu", true);
			}
		});

	// Auto adjust menu with by font size
	auto menu_auto_width = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("AutoMenuWidth"));
	addWithLabel(_("AUTO SIZE WIDTH BASED ON FONT SIZE"), menu_auto_width);
	addSaveFunc([this, menu_auto_width, window]
		{
			bool old_value = Settings::getInstance()->getBool("AutoMenuWidth");
			if (old_value != menu_auto_width->getState())
			{
				Settings::getInstance()->setBool("AutoMenuWidth", menu_auto_width->getState());
				if (menu_auto_width->getState() && Settings::getInstance()->getBool("MenusAllWidth"))
				{
					window->pushGui(new GuiMsgBox(window, _("YOU SELECTED TO AUTO SIZE WIDTH, THE MENU \"FILLS THE ALL WIDTH\" OPTION IS DISABLED."),
						_("OK"), [] { Settings::getInstance()->setBool("MenusAllWidth", false); }));
				}
				setVariable("reloadGuiMenu", true);
			}
		});

	// menus all height
	auto menusAllHeight = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("MenusAllHeight"));
	addWithLabel(_("FILLS THE ALL HEIGHT"), menusAllHeight);
	addSaveFunc([this, menusAllHeight, window]
		{
			bool old_value = Settings::getInstance()->getBool("MenusAllHeight");
			if (old_value != menusAllHeight->getState())
			{
				Settings::getInstance()->setBool("MenusAllHeight", menusAllHeight->getState());
				if (menusAllHeight->getState() && Settings::getInstance()->getBool("MenusOnDisplayTop"))
				{
					window->pushGui(new GuiMsgBox(window, _("YOU SELECTED TO FILLS THE ALL HEIGHT, THE MENU \"ON THE TOP OF THE DISPLAY\" OPTION IS DISABLED."),
						_("OK"), [] { Settings::getInstance()->setBool("MenusOnDisplayTop", false); }));
				}
				setVariable("reloadGuiMenu", true);
			}
		});

	// menus all width
	auto menusAllWith = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("MenusAllWidth"));
	addWithLabel(_("FILLS THE ALL WIDTH"), menusAllWith);
	addSaveFunc([this, menusAllWith, window]
		{
			bool old_value = Settings::getInstance()->getBool("MenusAllWidth");
			if (old_value != menusAllWith->getState())
			{
				Settings::getInstance()->setBool("MenusAllWidth", menusAllWith->getState());
				if (menusAllWith->getState() && Settings::getInstance()->getBool("AutoMenuWidth"))
				{
					
					window->pushGui(new GuiMsgBox(window, _("YOU SELECTED TO FILLS THE ALL WIDTH, THE MENU \"AUTO SIZE WIDTH BASED ON FONT SIZE\" OPTION IS DISABLED."),
						_("OK"), [] { Settings::getInstance()->setBool("AutoMenuWidth", false); }));
				}
				setVariable("reloadGuiMenu", true);
			}
		});

	// animated main menu
	auto animated_main_menu = std::make_shared<SwitchComponent>(window, Settings::getInstance()->getBool("AnimatedMainMenu"));
	addWithLabel(_("OPEN MAIN MENU WITH ANIMATION"), animated_main_menu);
	addSaveFunc([animated_main_menu]
		{
			bool old_value = Settings::getInstance()->getBool("AnimatedMainMenu");
			if (old_value != animated_main_menu->getState())
			{
				Settings::getInstance()->setBool("AnimatedMainMenu", animated_main_menu->getState());
			}
		});

}
