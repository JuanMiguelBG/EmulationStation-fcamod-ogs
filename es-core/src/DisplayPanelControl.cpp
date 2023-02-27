#include "DisplayPanelControl.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "platform.h"
#include <cstring>


const char* PANEL_DRM_TOOL_PATH = "/usr/local/bin/panel_drm_tool";

std::weak_ptr<DisplayPanelControl> DisplayPanelControl::sInstance;


DisplayPanelControl::DisplayPanelControl()
{
	mExistPanelDrmTool = false;
	init();
	LOG(LogInfo) << "DisplayPanelControl::DisplayPanelControl() - Initialized, command '" << PANEL_DRM_TOOL_PATH << "' exist: " << Utils::String::boolToString(mExistPanelDrmTool);
}

DisplayPanelControl::~DisplayPanelControl()
{
	//set original volume levels for system
	//setVolume(originalVolume);

	deinit();
	LOG(LogInfo) << "DisplayPanelControl::DisplayPanelControl() - Deinitialized";
}

std::shared_ptr<DisplayPanelControl> & DisplayPanelControl::getInstance()
{
	//check if an DisplayPanelControl instance is already created, if not create one
	static std::shared_ptr<DisplayPanelControl> sharedInstance = sInstance.lock();
	if (sharedInstance == nullptr) {
		sharedInstance.reset(new DisplayPanelControl);
		sInstance = sharedInstance;
	}
	return sharedInstance;
}

void DisplayPanelControl::init()
{
	//initialize audio mixer interface
	mExistPanelDrmTool = Utils::FileSystem::exists(PANEL_DRM_TOOL_PATH);
}

void DisplayPanelControl::deinit()
{
	mExistPanelDrmTool = false;
}

int DisplayPanelControl::checkValue(int value)
{
	int checked_value = 50;
	if (value < 1)
		return 1;

	if (value > 100)
		return 100;
	
	return value;
}

void DisplayPanelControl::setGammaLevel(int gammaLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript("es-display set_panel gamma " + std::to_string(checkValue(gammaLevel)));
}

int DisplayPanelControl::getGammaLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(R"(es-display get_panel gamma)").c_str() ));
}

void DisplayPanelControl::setContrastLevel(int contrastLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript("es-display set_panel contrast " + std::to_string(checkValue(contrastLevel)));
}

int DisplayPanelControl::getContrastLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(R"(es-display get_panel contrast)").c_str() ));
}

void DisplayPanelControl::setSaturationLevel(int saturationLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript("es-display set_panel saturation " + std::to_string(checkValue(saturationLevel)));
}

int DisplayPanelControl::getSaturationLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(R"(es-display get_panel saturation)").c_str() ));
}

void DisplayPanelControl::setHueLevel(int hueLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript("es-display set_panel hue " + std::to_string(checkValue(hueLevel)));
}

int DisplayPanelControl::getHueLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(R"(es-display get_panel hue)").c_str() ));
}

void DisplayPanelControl::resetDisplayPanelSettings()
{
	if (!mExistPanelDrmTool)
		return;
	
	executeSystemScript(R"(es-display reset_panel_settings)");
}

bool DisplayPanelControl::isAvailable()
{
	return true;
}
