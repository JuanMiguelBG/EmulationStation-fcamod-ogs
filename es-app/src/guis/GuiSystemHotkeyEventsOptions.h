#pragma once
#ifndef ES_APP_GUIS_SYSTEM_HOTKEY_EVENTS_OPTIONS_H
#define ES_APP_GUIS_SYSTEM_HOTKEY_EVENTS_OPTIONS_H

#include "GuiComponent.h"
#include "guis/GuiSettings.h"

class GuiSystemHotkeyEventsOptions : public GuiSettings
{
public:
	GuiSystemHotkeyEventsOptions(Window* window);
	~GuiSystemHotkeyEventsOptions();

private:
	void initializeMenu(Window* window);
};

#endif // ES_APP_GUIS_SYSTEM_HOTKEY_EVENTS_OPTIONS_H
