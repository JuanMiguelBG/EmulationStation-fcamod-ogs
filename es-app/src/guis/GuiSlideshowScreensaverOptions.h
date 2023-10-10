#pragma once
#ifndef ES_APP_GUIS_GUI_SLIDESHOW_SCREENSAVER_OPTIONS_H
#define ES_APP_GUIS_GUI_SLIDESHOW_SCREENSAVER_OPTIONS_H

#include "GuiScreensaverOptions.h"

class GuiSlideshowScreensaverOptions : public GuiScreensaverOptions
{
public:
	GuiSlideshowScreensaverOptions(Window* window, const char* title);
	virtual ~GuiSlideshowScreensaverOptions();
};

#endif // ES_APP_GUIS_GUI_SLIDESHOW_SCREENSAVER_OPTIONS_H
