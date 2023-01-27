#pragma once
#ifndef ES_APP_GUIS_GUI_MENUS_OPTIONS_H
#define ES_APP_GUIS_GUI_MENUS_OPTIONS_H

#include "GuiComponent.h"
#include "guis/GuiSettings.h"

class GuiMenusOptions : public GuiSettings
{
public:
	GuiMenusOptions(Window* window);
	~GuiMenusOptions();

private:
	void initializeMenu(Window* window);
};

#endif // ES_APP_GUIS_GUI_MENUS_OPTIONS_H
