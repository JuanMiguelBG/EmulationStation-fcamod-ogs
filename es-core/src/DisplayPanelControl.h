#pragma once
#ifndef ES_CORE_DISPLAY_PANEL_CONTROL_H
#define ES_CORE_DISPLAY_PANEL_CONTROL_H

#include <memory>
#include <unistd.h>
#include <fcntl.h>

/*!
Singleton pattern. Call getInstance() to get an object.
*/
class DisplayPanelControl
{
	static std::weak_ptr<DisplayPanelControl> sInstance;

	DisplayPanelControl();	

public:
	static std::shared_ptr<DisplayPanelControl> & getInstance();

	void init();
	void deinit();

	bool isAvailable();

	void setGammaLevel(int gammaLevel);
	int getGammaLevel();
	void setContrastLevel(int contrastLevel);
	int getContrastLevel();
	void setSaturationLevel(int saturationLevel);
	int getSaturationLevel();
	void setHueLevel(int hueLevel);
	int getHueLevel();
	void resetDisplayPanelSettings();

	~DisplayPanelControl();

private:
	int checkValue(int value);

	bool mExistPanelDrmTool;
};

#endif // ES_CORE_DISPLAY_PANEL_CONTROL_H
