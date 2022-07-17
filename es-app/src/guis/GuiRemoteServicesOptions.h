#pragma once
#ifndef ES_APP_GUIS_REMOTE_SERVICES_OPTIONS_H
#define ES_APP_GUIS_REMOTE_SERVICES_OPTIONS_H

#include "GuiComponent.h"
#include "guis/GuiSettings.h"
#include "platform.h"

class GuiRemoteServicesOptions : public GuiSettings
{
public:
	GuiRemoteServicesOptions(Window* window);
	~GuiRemoteServicesOptions();

private:
	void initializeMenu(Window* window);

	void configRemoteService(Window* window, RemoteServiceInformation service, bool isActive, bool isStartOnBoot);

	bool mPopupDisplayed;
};

#endif // ES_APP_GUIS_REMOTE_SERVICES_OPTIONS_H
