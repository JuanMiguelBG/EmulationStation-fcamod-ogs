#include "ApiSystem.h"
#include <stdlib.h>
#include <sys/statvfs.h>
#include "HttpReq.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/md5.h"
#include "utils/ZipFile.h"
#include <thread>
#include <codecvt> 
#include <locale> 
#include "Log.h"
#include "Window.h"
#include "components/AsyncNotificationComponent.h"
#include "AudioManager.h"
#include "VolumeControl.h"
#include "InputManager.h"
#include "EsLocale.h"
#include <algorithm>
#include "guis/GuiMsgBox.h"
#include "Scripting.h"
#include "EmulationStation.h"

UpdateState::State ApiSystem::state = UpdateState::State::NO_UPDATE;

class ThreadedUpdater
{
public:
	ThreadedUpdater(Window* window) : mWindow(window)
	{
		ApiSystem::state = UpdateState::State::UPDATER_RUNNING;

		mWndNotification = new AsyncNotificationComponent(window, false);
		mWndNotification->updateTitle(_U("\uF019 ") + _("EMULATIONSTATION"));

		mWindow->registerNotificationComponent(mWndNotification);
		mHandle = new std::thread(&ThreadedUpdater::threadUpdate, this);
	}

	~ThreadedUpdater()
	{
		mWindow->unRegisterNotificationComponent(mWndNotification);
		delete mWndNotification;
	}

	void threadUpdate()
	{
		std::pair<std::string, int> updateStatus = ApiSystem::getInstance()->updateSystem([this](const std::string info)
		{
			auto pos = info.find(">>>");
			if (pos != std::string::npos)
			{
				std::string percent(info.substr(pos));
				percent = Utils::String::replace(percent, ">", "");
				percent = Utils::String::replace(percent, "%", "");
				percent = Utils::String::replace(percent, " ", "");

				int value = atoi(percent.c_str());

				std::string text(info.substr(0, pos));
				text = Utils::String::trim(text);

				mWndNotification->updatePercent(value);
				mWndNotification->updateText(text);
			}
			else
			{
				mWndNotification->updatePercent(-1);
				mWndNotification->updateText(info);
			}
		});

		if (updateStatus.second == 0)
		{
			ApiSystem::state = UpdateState::State::UPDATE_READY;

			mWndNotification->updateTitle(_U("\uF019 ") + _("UPDATE IS READY"));
			mWndNotification->updateText(_("RESTART EMULATIONSTATION TO APPLY"));

			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::hours(12));
		}
		else
		{
			ApiSystem::state = UpdateState::State::NO_UPDATE;

			std::string error = _("AN ERROR OCCURED") + std::string(": ") + updateStatus.first;
			mWindow->displayNotificationMessage(error);
		}

		delete this;
	}

private:
	std::thread*				mHandle;
	AsyncNotificationComponent* mWndNotification;
	Window*						mWindow;
};

ApiSystem::ApiSystem() { }

ApiSystem* ApiSystem::instance = nullptr;

ApiSystem *ApiSystem::getInstance()
{
	if (ApiSystem::instance == nullptr)
		ApiSystem::instance = new ApiSystem();

	return ApiSystem::instance;
}

std::vector<std::string> ApiSystem::executeEnumerationScript(const std::string command)
{
	return executeSystemEnumerationScript(command);
}

std::pair<std::string, int> ApiSystem::executeScript(const std::string command, const std::function<void(const std::string)>& func)
{
	return executeSystemScript(command, func);
}

bool ApiSystem::executeScript(const std::string command)
{
	return executeSystemScript(command);
}

bool ApiSystem::executeBoolScript(const std::string command)
{
	return executeSystemBoolScript(command);
}

bool ApiSystem::isScriptingSupported(ScriptId script)
{
	std::vector<std::string> executables;

	switch (script)
	{
		case TIMEZONE:
				executables.push_back("timezones");
				break;
		case DISPLAY:
				executables.push_back("es-display");
				break;
		case SYSTEM_HOTKEY_EVENTS:
				executables.push_back("es-system_hotkey");
				break;
		case WIFI:
				executables.push_back("es-wifi");
				break;
		case RETROACHIVEMENTS:
				executables.push_back("es-cheevos");
				break;
		case LANGUAGE:
				executables.push_back("es-language");
				break;
		case SYSTEM_INFORMATION:
				executables.push_back("es-system_inf");
				break;
		case AUTO_SUSPEND:
				executables.push_back("es-auto_suspend");
				break;
		case OPTMIZE_SYSTEM:
				executables.push_back("es-optimize_system");
				break;
		case SHOW_FPS:
				executables.push_back("es-show_fps");
				break;
		case OVERCLOCK:
				executables.push_back("es-overclock_system");
				break;
		case PRELOAD_VLC:
				executables.push_back("es-preload_vlc");
				break;
		case SOUND:
				executables.push_back("es-sound");
				break;
		case REMOTE_SERVICES:
				executables.push_back("es-remote_services");
				break;
		case LOG_SCRIPTS:
				executables.push_back("es-log_scripts");
				break;
		case BLUETOOTH:
				executables.push_back("es-bluetooth");
				break;
		case KODI:
				executables.push_back("Kodi.sh");
				break;
	}

	for (auto executable : executables)
		if (Utils::FileSystem::exists("/usr/bin/" + executable) || Utils::FileSystem::exists("/usr/local/bin/" + executable))
			return true;

	return false;
}

void ApiSystem::startUpdate(Window* c)
{
}

std::string ApiSystem::checkUpdateVersion()
{
	return "";
}

std::pair<std::string, int> ApiSystem::updateSystem(const std::function<void(const std::string)>& func)
{
	return std::pair<std::string, int>("error.", 1);
}

std::vector<ThemeDownloadInfo> ApiSystem::getThemesList()
{
	LOG(LogDebug) << "ApiSystem::getThemesList";

	std::vector<ThemeDownloadInfo> res;

	HttpReq httpreq("https://batocera.org/upgrades/themes.txt");
	if (httpreq.wait())
	{
		auto lines = Utils::String::split(httpreq.getContent(), '\n');
		for (auto line : lines)
		{
			auto parts = Utils::String::splitAny(line, " \t");
			if (parts.size() > 1)
			{
				auto themeName = parts[0];

				std::string themeUrl = parts[1];
				std::string themeFolder = Utils::FileSystem::getFileName(themeUrl);

				bool themeExists = false;

				std::vector<std::string> paths{
					"/etc/emulationstation/themes",
					Utils::FileSystem::getEsConfigPath() + "/themes",
					Utils::FileSystem::getUserDataPath() + "/themes"
				};

				for (auto path : paths)
				{
					themeExists = Utils::FileSystem::isDirectory(path + "/" + themeName) ||
						Utils::FileSystem::isDirectory(path + "/" + themeFolder) ||
						Utils::FileSystem::isDirectory(path + "/" + themeFolder + "-master");
					
					if (themeExists)
					{
						LOG(LogInfo) << "ApiSystem::getThemesList() - Get themes directory path '" << path << "'...";
						break;
					}
				}

				ThemeDownloadInfo info;
				info.installed = themeExists;
				info.name = themeName;
				info.url = themeUrl;

				res.push_back(info);
			}
		}
	}

	return res;
}

bool downloadGitRepository(const std::string url, const std::string fileName, const std::string label, const std::function<void(const std::string)>& func)
{
	if (func != nullptr)
		func(_("Downloading") + " " + label);

	long downloadSize = 0;

	std::string statUrl = Utils::String::replace(url, "https://github.com/", "https://api.github.com/repos/");
	if (statUrl != url)
	{
		HttpReq statreq(statUrl);
		if (statreq.wait())
		{
			std::string content = statreq.getContent();
			auto pos = content.find("\"size\": ");
			if (pos != std::string::npos)
			{
				auto end = content.find(",", pos);
				if (end != std::string::npos)
					downloadSize = atoi(content.substr(pos + 8, end - pos - 8).c_str()) * 1024;
			}
		}
	}

	HttpReq httpreq(url + "/archive/master.zip", fileName);

	int curPos = -1;
	while (httpreq.status() == HttpReq::REQ_IN_PROGRESS)
	{
		if (downloadSize > 0)
		{
			double pos = httpreq.getPosition();
			if (pos > 0 && curPos != pos)
			{
				if (func != nullptr)
				{
					std::string pc = std::to_string((int)(pos * 100.0 / downloadSize));
					func(std::string(_("Downloading") + " " + label + " >>> " + pc + " %"));
				}

				curPos = pos;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	if (httpreq.status() != HttpReq::REQ_SUCCESS)
		return false;

	return true;
}

std::pair<std::string, int> ApiSystem::installTheme(std::string themeName, const std::function<void(const std::string)>& func)
{
	return std::pair<std::string, int>(std::string("Theme not found"), 1);
}

	// Storage functions
unsigned long ApiSystem::getFreeSpaceGB(std::string mountpoint)
{
	LOG(LogInfo) << "ApiSystem::getFreeSpaceGB";

	int free = 0;

	struct statvfs fiData;
	if ((statvfs(mountpoint.c_str(), &fiData)) >= 0)
		free = (fiData.f_bfree * fiData.f_bsize) / (1024 * 1024 * 1024);

	return free;
}

std::string ApiSystem::getFreeSpaceSystemInfo() {
	return getFreeSpaceInfo("/");
}

std::string ApiSystem::getFreeSpaceBootInfo() {
	return getFreeSpaceInfo("/boot");
}

std::string ApiSystem::getFreeSpaceUserInfo() {
	return getFreeSpaceInfo("/roms");
}

std::string ApiSystem::getFreeSpaceUser2Info()
{
	return getFreeSpaceInfo("/roms2");
}

bool ApiSystem::isUser2Mounted()
{
	return isDriveMounted("/roms2");
}

std::string ApiSystem::getFreeSpaceUsbDriveInfo(const std::string mountpoint)
{
	LOG(LogInfo) << "ApiSystem::getFreeSpaceUsbDriveInfo() - mount point: " << mountpoint;
	if ( isDriveMounted(mountpoint) )
		return getFreeSpaceInfo(mountpoint);

	return "";
}

std::string ApiSystem::getFreeSpaceInfo(const std::string mountpoint)
{
	LOG(LogInfo) << "ApiSystem::getFreeSpaceInfo() - mount point: " << mountpoint;

	std::ostringstream oss;

	struct statvfs fiData;
	if ((statvfs(mountpoint.c_str(), &fiData)) < 0)
		return "";

	unsigned long long total = (unsigned long long) fiData.f_blocks * (unsigned long long) (fiData.f_bsize);
	unsigned long long free = (unsigned long long) fiData.f_bfree * (unsigned long long) (fiData.f_bsize);
	unsigned long long used = total - free;
	unsigned long percent = 0;

	if (total != 0)
	{  //for small SD card ;) with share < 1GB
		percent = used * 100 / total;
		oss << Utils::FileSystem::megaBytesToString(used / (1024L * 1024L)) << "/" << Utils::FileSystem::megaBytesToString(total / (1024L * 1024L)) << " (" << percent << " %)";
	}
	else
		oss << "N/A";

	return oss.str();
}


std::vector<std::string> ApiSystem::getUsbDriveMountPoints()
{
	return queryUsbDriveMountPoints();
}

bool ApiSystem::isFreeSpaceSystemLimit() {
	return isFreeSpaceLimit("/");
}

bool ApiSystem::isFreeSpaceBootLimit() {
	return isFreeSpaceLimit("/boot");
}

bool ApiSystem::isFreeSpaceUserLimit() {
	return isFreeSpaceLimit("/roms", 2);
}

bool ApiSystem::isFreeSpaceUser2Limit() {
	return isFreeSpaceLimit("/roms2", 2);
}

bool ApiSystem::isFreeSpaceUsbDriveLimit(const std::string mountpoint) {
	if ( isDriveMounted(mountpoint) )
		return isFreeSpaceLimit(mountpoint, 2);

	return false;
}

bool ApiSystem::isFreeSpaceLimit(const std::string mountpoint, int limit)
{
	return ((int) getFreeSpaceGB(mountpoint)) < limit;
}

bool ApiSystem::isTemperatureLimit(float temperature, float limit)
{
	return temperature > limit;
}

bool ApiSystem::isLoadCpuLimit(float load_cpu, float limit) // %
{
	return load_cpu > limit;
}

bool ApiSystem::isMemoryLimit(float total_memory, float free_memory, int limit) // %
{
	int percent = ( (int) ( (free_memory * 100) / total_memory ) );
	return percent < limit;
}

bool ApiSystem::isBatteryLimit(float battery_level, int limit) // %
{
	return battery_level < limit;
}

std::string ApiSystem::getVersion()
{
	LOG(LogInfo) << "ApiSystem::getVersion()";
	return querySoftwareInformation(true).version;
}

std::string ApiSystem::getApplicationName()
{
	LOG(LogInfo) << "ApiSystem::getApplicationName()";
	return querySoftwareInformation(true).application_name;
}

std::string ApiSystem::getHostname()
{
	LOG(LogInfo) << "ApiSystem::getHostname()";
	return queryHostname();
}

bool ApiSystem::setHostname(std::string hostname)
{
	LOG(LogInfo) << "ApiSystem::setHostname()";
	return setCurrentHostname(hostname);
}

std::string ApiSystem::getIpAddress()
{
	LOG(LogInfo) << "ApiSystem::getIpAddress()";

	std::string result = queryIPAddress(); // platform.h
	if (result.empty())
		return "___.___.___.___/__";

	return result;
}

float ApiSystem::getLoadCpu()
{
	LOG(LogInfo) << "ApiSystem::getLoadCpu()";
	return queryLoadCpu();
}

int ApiSystem::getFrequencyCpu()
{
	LOG(LogInfo) << "ApiSystem::getFrequencyCpu()";
	return queryFrequencyCpu();
}

float ApiSystem::getTemperatureCpu()
{
	LOG(LogInfo) << "ApiSystem::getTemperatureCpu()";
	return queryTemperatureCpu();
}

float ApiSystem::getTemperatureGpu()
{
	LOG(LogInfo) << "ApiSystem::getTemperatureGpu()";
	return queryTemperatureGpu();
}

int ApiSystem::getFrequencyGpu()
{
	LOG(LogInfo) << "ApiSystem::getFrequencyGpu()";
	return queryFrequencyGpu();
}

bool ApiSystem::isNetworkConnected()
{
	LOG(LogInfo) << "ApiSystem::isNetworkConnected()";
	return queryNetworkConnected();
}

NetworkInformation ApiSystem::getNetworkInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getNetworkInformation()";

	return queryNetworkInformation(summary); // platform.h
}

BatteryInformation ApiSystem::getBatteryInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getBatteryInformation()";

	return queryBatteryInformation(summary); // platform.h
}

CpuAndSocketInformation ApiSystem::getCpuAndChipsetInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getCpuAndChipsetInformation()";

	return queryCpuAndChipsetInformation(summary); // platform.h
}

RamMemoryInformation ApiSystem::getRamMemoryInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getRamMemoryInformation()";

	return queryRamMemoryInformation(summary); // platform.h
}

DisplayAndGpuInformation ApiSystem::getDisplayAndGpuInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getDisplayAndGpuInformation()";

	return queryDisplayAndGpuInformation(summary); // platform.h
}

SoftwareInformation ApiSystem::getSoftwareInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getSoftwareInformation()";

	SoftwareInformation si = querySoftwareInformation(summary); // platform.h
	si.es_version = Utils::String::toUpper(PROGRAM_VERSION_STRING);
	si.es_built = PROGRAM_BUILT_STRING;
	return si;
}

DeviceInformation ApiSystem::getDeviceInformation(bool summary)
{
	LOG(LogInfo) << "ApiSystem::getDeviceInformation()";

	return queryDeviceInformation(summary); // platform.h
}

int ApiSystem::getBrightnessLevel()
{
	LOG(LogInfo) << "ApiSystem::getBrightnessLevel()";

	return queryBrightnessLevel();
}

void ApiSystem::setBrightnessLevel(int brightnessLevel)
{
	LOG(LogInfo) << "ApiSystem::setBrightnessLevel()";

	saveBrightnessLevel(brightnessLevel);
}

int ApiSystem::getBrightness()
{
	LOG(LogInfo) << "ApiSystem::getBrightness()";

	return queryBrightness();
}

void ApiSystem::backupBrightnessLevel()
{
	LOG(LogInfo) << "ApiSystem::backupBrightnessLevel()";

	Settings::getInstance()->setInt("BrightnessBackup", ApiSystem::getInstance()->getBrightnessLevel());
}

void ApiSystem::restoreBrightnessLevel()
{
	LOG(LogInfo) << "ApiSystem::restoreBrightnessLevel()";

	ApiSystem::getInstance()->setBrightnessLevel(Settings::getInstance()->getInt("BrightnessBackup"));
}

int ApiSystem::getVolume()
{
	LOG(LogInfo) << "ApiSystem::getVolume()";

	return VolumeControl::getInstance()->getVolume();
}

void ApiSystem::setVolume(int volumeLevel)
{
	LOG(LogInfo) << "ApiSystem::setVolume()";

	VolumeControl::getInstance()->setVolume(volumeLevel);
}

void ApiSystem::backupVolume()
{
	LOG(LogInfo) << "ApiSystem::backupVolume()";

	Settings::getInstance()->setInt("VolumeBackup", ApiSystem::getInstance()->getVolume());
}

void ApiSystem::restoreVolume()
{
	LOG(LogInfo) << "ApiSystem::restoreVolume()";

	ApiSystem::getInstance()->setVolume(Settings::getInstance()->getInt("VolumeBackup"));
}

int ApiSystem::getBatteryLevel()
{
	LOG(LogInfo) << "ApiSystem::getBatteryLevel()";

	return queryBatteryLevel();
}

bool ApiSystem::isBatteryCharging()
{
	LOG(LogInfo) << "ApiSystem::isBatteryCharging()";

	return queryBatteryCharging();
}

float ApiSystem::getBatteryVoltage()
{
	LOG(LogInfo) << "ApiSystem::getBatteryVoltage()";

	return queryBatteryVoltage();
}

float ApiSystem::getTemperatureBattery()
{
	LOG(LogInfo) << "ApiSystem::getTemperatureBattery()";
	return queryBatteryTemperature();
}

std::string ApiSystem::getDeviceName()
{
	LOG(LogInfo) << "ApiSystem::getDeviceName()";

	return queryDeviceName();
}

std::string ApiSystem::getTimezones()
{
	LOG(LogInfo) << "ApiSystem::getTimezones()";

	return queryTimezones();
}

std::string ApiSystem::getCurrentTimezone()
{
	LOG(LogInfo) << "ApiSystem::getCurrentTimezone()";

	return queryCurrentTimezone();
}

bool ApiSystem::setTimezone(std::string timezone)
{
	LOG(LogInfo) << "ApiSystem::setTimezone() - TZ: " << timezone;

	return setCurrentTimezone(timezone);
}

bool ApiSystem::setDisplayBlinkLowBattery(bool state)
{
	LOG(LogInfo) << "ApiSystem::setDisplayBlinkLowBattery()";

	return executeSystemScript("es-display set blink_low_battery " + stateToString(state) + " &");
}

bool ApiSystem::isSystemHotkeyBrightnessEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyBrightnessEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get brightness)"));
}

int ApiSystem::getSystemHotkeyBrightnessStep()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyBrightnessStep()";

	int brightness_step = std::atoi(getShOutput(R"(es-system_hotkey get brightness_step)").c_str());
	if (brightness_step <= 0)
		return 1;
	else if (brightness_step > 25)
		return 25;

	return brightness_step;
}

bool ApiSystem::isSystemHotkeyVolumeEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyVolumeEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get volume)"));
}

int ApiSystem::getSystemHotkeyVolumeStep()
{
	LOG(LogInfo) << "ApiSystem::getSystemHotkeyVolumeStep()";

	int volume_step = std::atoi(getShOutput(R"(es-system_hotkey get volume_step)").c_str());
	if (volume_step <= 0)
		return 1;
	else if (volume_step > 25)
		return 25;

	return volume_step;
}

bool ApiSystem::isSystemHotkeyWifiEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyWifiEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get wifi)"));
}

bool ApiSystem::isSystemHotkeyBluetoothEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyBluetoothEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get bluetooth)"));
}

bool ApiSystem::isSystemHotkeySpeakerEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeySpeakerEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get speaker)"));
}

bool ApiSystem::isSystemHotkeySuspendEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeySuspendEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get suspend)"));
}

bool ApiSystem::setSystemHotkeysValues(SystemHotkeyValues values)
{
	LOG(LogInfo) << "ApiSystem::setSystemHotkeysValues()";

	if (values.brightness_step <= 0)
		values.brightness_step = 1;
	else if (values.brightness_step > 25)
		values.brightness_step = 25;

	if (values.volume_step <= 0)
		values.volume_step = 1;
	else if (values.volume_step > 25)
		values.volume_step = 25;

	return executeSystemScript("es-system_hotkey set_all_values " + stateToString(values.brightness) + " " + std::to_string(values.brightness_step)
			+ " " + stateToString(values.volume) + " " + std::to_string(values.volume_step) + " " + stateToString(values.wifi)
			+ " " + stateToString(values.bluetooth) + " " + stateToString(values.speaker) + " " + stateToString(values.suspend) + " &");
}

SystemHotkeyValues ApiSystem::getSystemHotkeyValues()
{
	LOG(LogInfo) << "ApiSystem::isDeviceAutoSuspendByTime()";

	std::vector<std::string> values = Utils::String::split( getShOutput(R"(es-system_hotkey get_all_values)"), ';' );
	SystemHotkeyValues result;
	if (values.size() > 0)
		result.brightness = stringToState(values[0]);
		result.brightness_step = std::atoi(values[1].c_str());
		result.volume = stringToState(values[2]);
		result.volume_step = std::atoi(values[3].c_str());
		result.wifi = stringToState(values[4]);
		result.bluetooth = stringToState(values[5]);
		result.speaker = stringToState(values[6]);
		result.suspend = stringToState(values[7]);
	return result;
}

bool ApiSystem::isDeviceAutoSuspendByTime()
{
	LOG(LogInfo) << "ApiSystem::isDeviceAutoSuspendByTime()";

	return stringToState(getShOutput(R"(es-auto_suspend get auto_suspend_time)"));
}

int ApiSystem::getAutoSuspendTimeout()
{
	LOG(LogInfo) << "ApiSystem::getAutoSuspendTimeout()";

	int timeout = std::atoi(getShOutput(R"(es-auto_suspend get auto_suspend_timeout)").c_str());
	if (timeout <= 0)
		return 5;
	else if (timeout > 120)
		return 120;

	return timeout;
}

bool ApiSystem::isDeviceAutoSuspendByBatteryLevel()
{
	LOG(LogInfo) << "ApiSystem::isDeviceAutoSuspendByBatteryLevel()";

	return stringToState(getShOutput(R"(es-auto_suspend get auto_suspend_battery)"));
}

int ApiSystem::getAutoSuspendBatteryLevel()
{
	LOG(LogInfo) << "ApiSystem::getAutoSuspendBatteryLevel()";

	int battery_level = std::atoi(getShOutput(R"(es-auto_suspend get auto_suspend_battery_level)").c_str());
	if (battery_level <= 0)
		return 10;
	else if (battery_level > 100)
		return 100;

	return battery_level;
}

bool ApiSystem::isDeviceAutoSuspend()
{
	LOG(LogInfo) << "ApiSystem::isDeviceAutoSuspend()";

	return isDeviceAutoSuspendByTime() || isDeviceAutoSuspendByBatteryLevel();
}

bool ApiSystem::isDeviceAutoSuspendStayAwakeCharging()
{
	LOG(LogInfo) << "ApiSystem::isDeviceAutoSuspendStayAwakeCharging()";

	return stringToState(getShOutput(R"(es-auto_suspend get auto_suspend_stay_awake_while_charging)"));
}

bool ApiSystem::setDeviceAutoSuspendValues(bool stay_awake_charging_state, bool time_state, int timeout, bool battery_state, int battery_level)
{
	LOG(LogInfo) << "ApiSystem::setDeviceAutoSuspendValues()";

	if (timeout <= 0)
		timeout = 5;
	else if (timeout > 120)
		timeout = 120;

	if (battery_level <= 0)
		battery_level = 10;
	else if (battery_level > 100)
		battery_level = 100;

	return executeSystemScript("es-auto_suspend set_all_values " + stateToString(stay_awake_charging_state) + " " + stateToString(time_state) + " " + std::to_string(timeout) + " " + stateToString(battery_state) + " " + std::to_string(battery_level) + " &");

}

bool ApiSystem::isDisplayAutoDimStayAwakeCharging()
{
	LOG(LogInfo) << "ApiSystem::isDisplayAutoDimStayAwakeCharging()";

	return stringToState(getShOutput(R"(es-display get auto_dim_stay_awake_while_charging)"));
}

bool ApiSystem::isDisplayAutoDimByTime()
{
	LOG(LogInfo) << "ApiSystem::isDisplayAutoDimByTime()";

	return stringToState(getShOutput(R"(es-display get auto_dim_time)"));
}

int ApiSystem::getDisplayAutoDimTimeout()
{
	LOG(LogInfo) << "ApiSystem::getDisplayAutoDimTimeout()";

	int timeout = std::atoi(getShOutput(R"(es-display get auto_dim_timeout)").c_str());
	if (timeout <= 0)
		return 5;
	else if (timeout > 120)
		return 120;

	return timeout;
}

int ApiSystem::getDisplayAutoDimBrightness()
{
	LOG(LogInfo) << "ApiSystem::getDisplayAutoDimBrightness()";

	int brightness_level = std::atoi(getShOutput(R"(es-display get auto_dim_brightness)").c_str());
	if (brightness_level <= 0)
		return 25;
	else if (brightness_level > 100)
		return 100;

	return brightness_level;
}

bool ApiSystem::setDisplayAutoDimValues(bool stay_awake_charging_state, bool time_state, int timeout, int brightness_level)
{
	LOG(LogInfo) << "ApiSystem::setDisplayAutoDimValues()";

	if (timeout <= 0)
		timeout = 5;
	else if (timeout > 120)
		timeout = 120;

	if (brightness_level <= 0)
		brightness_level = 25;
	else if (brightness_level > 100)
		brightness_level = 100;

	return executeSystemScript("es-display set_auto_dim_all_values " + stateToString(stay_awake_charging_state) + " " + stateToString(time_state) + " " + std::to_string(timeout) + " " + std::to_string(brightness_level) + " &");
}

bool ApiSystem::ping()
{
	if (!executeSystemScript("timeout 1 ping -c 1 -t 255 8.8.8.8")) // ping Google DNS
		return executeSystemScript("timeout 2 ping -c 1 -t 255 8.8.4.4"); // ping Google secondary DNS & give 2 seconds

	return true;
}

bool ApiSystem::getInternetStatus()
{
	LOG(LogInfo) << "ApiSystem::getInternetStatus()";
	if (ping())
		return true;

	return executeSystemBoolScript("es-wifi internet_status");
}

std::vector<std::string> ApiSystem::getWifiNetworks(bool scan)
{
	LOG(LogInfo) << "ApiSystem::getWifiNetworks()";

	return executeEnumerationScript(scan ? "es-wifi scanlist" : "es-wifi list");
}

bool ApiSystem::connectWifi(const std::string ssid, const std::string key)
{
	LOG(LogInfo) << "ApiSystem::connectWifi() - SSID: '" << ssid << "'";

	return executeSystemScript("es-wifi connect \"" + ssid + "\" \"" + key + '"');
}

bool ApiSystem::disconnectWifi(const std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::disconnectWifi() - SSID: '" << ssid << "'";

	return executeSystemScript("es-wifi disconnect \"" + ssid + '"');
}

bool ApiSystem::enableWifi(bool background)
{
	LOG(LogInfo) << "ApiSystem::enableWifi()";

	std::string commnad("es-wifi enable");
	commnad.append(background ? " &" : "");

	return executeSystemScript(commnad);
}

bool ApiSystem::disableWifi(bool background)
{
	LOG(LogInfo) << "ApiSystem::disableWifi()";

	std::string commnad("es-wifi disable");
	commnad.append(background ? " &" : "");

	return executeSystemScript(commnad);
}

bool ApiSystem::resetWifi(const std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::resetWifi() - SSID: '" << ssid << "'";

	return executeSystemScript("es-wifi reset \"" + ssid + '"');
}

bool ApiSystem::isWifiEnabled()
{
	LOG(LogInfo) << "ApiSystem::isWifiEnabled()";

	return queryWifiEnabled();
}

bool ApiSystem::enableManualWifiDns(const std::string ssid, const std::string dnsOne, const std::string dnsTwo)
{
	LOG(LogInfo) << "ApiSystem::enableManualWifiDns() - SSID: '" << ssid << "', DNS1: " << dnsOne << ", DNS2: " << dnsTwo;

	return executeSystemScript("es-wifi enable_manual_dns \"" + ssid + "\" \"" + dnsOne + "\" \"" + dnsTwo + '"');
}

bool ApiSystem::disableManualWifiDns(const std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::disableManualWifiDns() - SSID: '" << ssid << "'";

	return executeSystemScript("es-wifi disable_manual_dns \"" + ssid + '"');
}

std::string ApiSystem::getWifiSsid()
{
	LOG(LogInfo) << "ApiSystem::getWifiSsid()";

	return queryWifiSsid();
}

std::string ApiSystem::getWifiPsk(const std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::getWifiPsk() - SSID: '" << ssid << "'";

	return queryWifiPsk(ssid);
}

std::string ApiSystem::getDnsOne()
{
	LOG(LogInfo) << "ApiSystem::getDnsOne()";

	return queryDnsOne();
}

std::string ApiSystem::getDnsTwo()
{
	LOG(LogInfo) << "ApiSystem::getDnsTwo()";

	return queryDnsTwo();
}

std::string ApiSystem::getWifiNetworkExistFlag()
{
	LOG(LogInfo) << "ApiSystem::getDnsTwo()";

	return queryWifiNetworkExistFlag();
}

bool ApiSystem::isWifiPowerSafeEnabled()
{
	LOG(LogInfo) << "ApiSystem::isWifiPowerSafeEnabled()";

	return executeSystemBoolScript("es-wifi is_wifi_power_safe_enabled");
}

void ApiSystem::setWifiPowerSafe(bool state)
{
	LOG(LogInfo) << "ApiSystem::setWifiPowerSafe()";

	executeSystemScript("es-wifi set_wifi_power_safe " + Utils::String::boolToString(state) + " &");
}


bool ApiSystem::setLanguage(std::string language)
{
	LOG(LogInfo) << "ApiSystem::setLanguage()";

	return executeSystemScript("es-language set " + language + " &");
}

bool ApiSystem::getRetroachievementsEnabled()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsEnabled()";

	return Utils::String::toBool(getShOutput(R"(es-cheevos get cheevos_enable)"));
}

bool ApiSystem::getRetroachievementsHardcoreEnabled()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsHardcoreEnabled()";

	return Utils::String::toBool(getShOutput(R"(es-cheevos get cheevos_hardcore_mode_enable)"));
}

bool ApiSystem::getRetroachievementsLeaderboardsEnabled()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsLeaderboardsEnabled()";

	return Utils::String::toBool(getShOutput(R"(es-cheevos get cheevos_leaderboards_enable)"));
}

bool ApiSystem::getRetroachievementsVerboseEnabled()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsVerboseEnabled()";

	return Utils::String::toBool(getShOutput(R"(es-cheevos get cheevos_verbose_enable)"));
}

bool ApiSystem::getRetroachievementsAutomaticScreenshotEnabled()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsAutomaticScreenshotEnabled()";

	return Utils::String::toBool(getShOutput(R"(es-cheevos get cheevos_auto_screenshot)"));
}

bool ApiSystem::getRetroachievementsUnlockSoundEnabled()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsUnlockSoundEnabled()";

	return Utils::String::toBool(getShOutput(R"(es-cheevos get cheevos_unlock_sound_enable)"));
}

std::vector<std::string> ApiSystem::getRetroachievementsSoundsList()
{
	Utils::FileSystem::FileSystemCacheActivator fsc;

	std::vector<std::string> ret;

	LOG(LogInfo) << "ApiSystem::getRetroAchievementsSoundsList";

	std::vector<std::string> folderList = executeEnumerationScript(R"(es-cheevos get cheevos_sound_folders)");

	if (folderList.empty()) {
		folderList = {
				Utils::FileSystem::getHomePath() + "/.config/retroarch/assets/sounds",
				Utils::FileSystem::getHomePath() + "/sounds/retroachievements"
		};
	}

	for (auto folder : folderList)
	{
		for (auto file : Utils::FileSystem::getDirContent(folder, false))
		{
			auto sound = Utils::FileSystem::getFileName(file);
			if (sound.substr(sound.find_last_of('.') + 1) == "ogg")
			{
				if (std::find(ret.cbegin(), ret.cend(), sound) == ret.cend())
				  ret.push_back(sound.substr(0, sound.find_last_of('.')));
			}
		}
	}

	std::sort(ret.begin(), ret.end());
	return ret;
}

std::string ApiSystem::getRetroachievementsUsername()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsUsername()";

	return getShOutput(R"(es-cheevos get cheevos_username)");
}

bool ApiSystem::getRetroachievementsChallengeIndicators()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsChallengeIndicators()";

	return Utils::String::toBool( getShOutput(R"(es-cheevos get cheevos_challenge_indicators)") );
}

bool ApiSystem::getRetroachievementsRichpresenceEnable()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsRichpresenceEnable()";

	return Utils::String::toBool( getShOutput(R"(es-cheevos get cheevos_richpresence_enable)") );
}

bool ApiSystem::getRetroachievementsBadgesEnable()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsBadgesEnable()";

	return Utils::String::toBool( getShOutput(R"(es-cheevos get cheevos_badges_enable)") );
}

bool ApiSystem::getRetroachievementsTestUnofficial()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsTestUnofficial()";

	return Utils::String::toBool( getShOutput(R"(es-cheevos get cheevos_test_unofficial)") );
}

bool ApiSystem::getRetroachievementsStartActive()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsStartActive()";

	return Utils::String::toBool( getShOutput(R"(es-cheevos get cheevos_start_active)") );
}

std::string ApiSystem::getRetroachievementsPassword()
{
	LOG(LogInfo) << "ApiSystem::getRetroachievementsPassword()";

	return getShOutput(R"(es-cheevos get cheevos_password)");
}

bool  ApiSystem::setRetroachievementsValues(bool retroachievements_state, bool hardcore_state, bool leaderboards_state, bool verbose_state, bool automatic_screenshot_state, bool challenge_indicators_state, bool richpresence_state, bool badges_state, bool test_unofficial_state, bool start_active_state, const std::string sound, const std::string username, const std::string password)
{
	LOG(LogInfo) << "ApiSystem::setRetroachievementsValues()";

	return executeSystemScript("es-cheevos set_all_values " + Utils::String::boolToString(retroachievements_state) + " " + Utils::String::boolToString(hardcore_state) + " " + Utils::String::boolToString(leaderboards_state) + " " + Utils::String::boolToString(verbose_state) + " " + Utils::String::boolToString(automatic_screenshot_state) + " " + Utils::String::boolToString(challenge_indicators_state) + " " + Utils::String::boolToString(richpresence_state) + " " + Utils::String::boolToString(badges_state) + " " + Utils::String::boolToString(test_unofficial_state) + " " + Utils::String::boolToString(start_active_state) + " \"" + sound + "\" \"" + username + "\" \"" + password + "\" &");
}

bool ApiSystem::setOptimizeSystem(bool state)
{
	LOG(LogInfo) << "ApiSystem::setOptimizeSystem()";

	return executeSystemScript("es-optimize_system active_optimize_system " + Utils::String::boolToString(state));
}

std::vector<std::string> ApiSystem::getSuspendModes()
{
	LOG(LogInfo) << "ApiSystem::getSuspendModes()";

	return executeEnumerationScript("es-optimize_system get_sleep_modes");
}

std::string ApiSystem::getSuspendMode()
{
	LOG(LogInfo) << "ApiSystem::getSuspendMode()";

	return getShOutput(R"(es-optimize_system get_sleep_mode)");
}

bool ApiSystem::setSuspendMode(const std::string suspend_mode)
{
	LOG(LogInfo) << "ApiSystem::setSuspendMode() - " << suspend_mode;

	return executeSystemScript("es-optimize_system set_sleep_mode " + suspend_mode);
}

bool ApiSystem::isEsScriptsLoggingActivated()
{
	LOG(LogInfo) << "ApiSystem::isEsScriptsLoggingActivated()";

	return Utils::String::toBool( getShOutput(R"(es-log_scripts is_actived_scripts_log)") );
}

bool ApiSystem::setEsScriptsLoggingActivated(bool state, const std::string level, bool logWithNanoSeconds)
{
	LOG(LogInfo) << "ApiSystem::setEsScriptsLoggingActivated()";

	return executeSystemScript("es-log_scripts active_es_scripts_log " + Utils::String::boolToString(state) + " " + level + " " + Utils::String::boolToString(logWithNanoSeconds) + " &");
}

bool ApiSystem::setShowRetroarchFps(bool state)
{
	LOG(LogInfo) << "ApiSystem::setShowRetroarchFps()";

	return executeSystemScript("es-show_fps set fps_show " + Utils::String::boolToString(state) + " &");
}

bool ApiSystem::isShowRetroarchFps()
{
	LOG(LogInfo) << "ApiSystem::isShowRetroarchFps()";

	return Utils::String::toBool( getShOutput(R"(es-show_fps get fps_show)") );
}

bool ApiSystem::setOverclockSystem(bool state)
{
	LOG(LogInfo) << "ApiSystem::setOverclockSystem()";

	return executeSystemScript("es-overclock_system set " + Utils::String::boolToString(state));
}

bool ApiSystem::isOverclockSystem()
{
	LOG(LogInfo) << "ApiSystem::isOverclockSystem()";

	return Utils::String::toBool( getShOutput(R"(es-overclock_system get)") );
}


std::string ApiSystem::getMD5(const std::string fileName, bool fromZipContents)
{
	LOG(LogDebug) << "ApiSystem::getMD5() >> " << fileName;

	// 7za x -so test.7z | md5sum
	std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fileName));
	if (ext == ".zip" && fromZipContents)
	{
		Utils::Zip::ZipFile file;
		if (file.load(fileName))
		{
			std::string romName;

			for (auto name : file.namelist())
			{
				if (Utils::FileSystem::getExtension(name) != ".txt" && !Utils::String::endsWith(name, "/"))
				{
					if (!romName.empty())
					{
						romName = "";
						break;
					}

					romName = name;
				}
			}

			if (!romName.empty())
				return file.getFileMd5(romName);
		}
	}

	if (fromZipContents && ext == ".7z")
	{
		auto cmd = getSevenZipCommand() + " x -so \"" + fileName + "\" | md5sum";
		auto ret = executeEnumerationScript(cmd);
		if (ret.size() == 1 && ret.cbegin()->length() >= 32)
			return ret.cbegin()->substr(0, 32);
	}

	std::string contentFile = fileName;
	std::string ret;
	std::string tmpZipDirectory;

	if (fromZipContents && ext == ".7z")
	{
		tmpZipDirectory = Utils::FileSystem::combine(Utils::FileSystem::getTempPath(), Utils::FileSystem::getStem(fileName));
		Utils::FileSystem::deleteDirectoryFiles(tmpZipDirectory);

		if (unzipFile(fileName, tmpZipDirectory))
		{
			auto fileList = Utils::FileSystem::getDirContent(tmpZipDirectory, true);

			std::vector<std::string> res;
			std::copy_if(fileList.cbegin(), fileList.cend(), std::back_inserter(res), [](const std::string file) { return Utils::FileSystem::getExtension(file) != ".txt";  });

			if (res.size() == 1)
				contentFile = *res.cbegin();
		}

		// if there's no file or many files ? get md5 of archive
	}

	ret = Utils::FileSystem::getFileMd5(contentFile);

	if (!tmpZipDirectory.empty())
		Utils::FileSystem::deleteDirectoryFiles(tmpZipDirectory, true);

	LOG(LogDebug) << "ApiSystem::getMD5() << " << ret;

	return ret;
}

std::string ApiSystem::getCRC32(std::string fileName, bool fromZipContents)
{
	LOG(LogDebug) << "ApiSystem::getCRC32() >> " << fileName;

	std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fileName));

	if (ext == ".7z" && fromZipContents)
	{
		LOG(LogDebug) << "ApiSystem::getCRC32() is using 7z";

		std::string fn = Utils::FileSystem::getFileName(fileName);
		auto cmd = getSevenZipCommand() + " l -slt \"" + fileName + "\"";
		auto lines = executeEnumerationScript(cmd);
		for (std::string all : lines)
		{
			int idx = all.find("CRC = ");
			if (idx != std::string::npos)
				return all.substr(idx + 6);
			else if (all.find(fn) == (all.size() - fn.size()) && all.length() > 8 && all[9] == ' ')
				return all.substr(0, 8);
		}
	}
	else if (ext == ".zip" && fromZipContents)
	{
		LOG(LogDebug) << "ApiSystem::getCRC32() is using ZipFile";

		Utils::Zip::ZipFile file;
		if (file.load(fileName))
		{
			std::string romName;

			for (auto name : file.namelist())
			{
				if (Utils::FileSystem::getExtension(name) != ".txt" && !Utils::String::endsWith(name, "/"))
				{
					if (!romName.empty())
					{
						romName = "";
						break;
					}

					romName = name;
				}
			}

			if (!romName.empty())
				return file.getFileCrc(romName);
		}
	}

	LOG(LogDebug) << "ApiSystem::getCRC32() is using fileBuffer";
	return Utils::FileSystem::getFileCrc32(fileName);
}

bool ApiSystem::unzipFile(const std::string fileName, const std::string destFolder, const std::function<bool(const std::string)>& shouldExtract)
{
	LOG(LogDebug) << "ApiSystem::unzipFile() >> " << fileName << " to " << destFolder;

	if (!Utils::FileSystem::exists(destFolder))
		Utils::FileSystem::createDirectory(destFolder);

	if (Utils::String::toLower(Utils::FileSystem::getExtension(fileName)) == ".zip")
	{
		LOG(LogDebug) << "ApiSystem::unzipFile() is using ZipFile";

		Utils::Zip::ZipFile file;
		if (file.load(fileName))
		{
			for (auto name : file.namelist())
			{
				if (Utils::String::endsWith(name, "/"))
				{
					Utils::FileSystem::createDirectory(Utils::FileSystem::combine(destFolder, name.substr(0, name.length() - 1)));
					continue;
				}

				if (shouldExtract != nullptr && !shouldExtract(Utils::FileSystem::combine(destFolder, name)))
					continue;

				file.extract(name, destFolder);
			}

			LOG(LogDebug) << "ApiSystem::unzipFile() << OK";
			return true;
		}

		LOG(LogDebug) << "ApiSystem::unzipFile() << KO Bad format ?" << fileName;
		return false;
	}

	LOG(LogDebug) << "ApiSystem::unzipFile() is using 7z";

	std::string cmd = getSevenZipCommand() + " x \"" + Utils::FileSystem::getPreferredPath(fileName) + "\" -y -o\"" + Utils::FileSystem::getPreferredPath(destFolder) + "\"";
	bool ret = executeSystemScript(cmd);
	LOG(LogDebug) << "ApiSystem::unzipFile() <<";
	return ret;
}


void ApiSystem::preloadVLC()
{
	LOG(LogInfo) << "ApiSystem::preloadVLC()";
	executeSystemScript("/usr/local/bin/es-preload_vlc &");
}

std::vector<std::string> ApiSystem::getAudioCards()
{
	LOG(LogInfo) << "ApiSystem::getAudioCards()";

	return executeSystemEnumerationScript(R"(es-sound get audio_cards)");
}

std::vector<std::string> ApiSystem::getAudioDevices()
{
	LOG(LogInfo) << "ApiSystem::getAudioDevices()";

	return executeSystemEnumerationScript(R"(es-sound get audio_devices)");
}

std::vector<std::string> ApiSystem::getOutputDevices()
{
	LOG(LogInfo) << "ApiSystem::getOutputDevices()";

	return executeSystemEnumerationScript(R"(es-sound get output_devices)");
}

std::string ApiSystem::getOutputDevice()
{
	LOG(LogInfo) << "ApiSystem::getOutputDevice()";

	return getShOutput(R"(es-sound get output_device)");
}

bool ApiSystem::setOutputDevice(const std::string device)
{
	LOG(LogInfo) << "ApiSystem::setOutputDevice()";

	return executeSystemScript("es-sound set output_device \"" + device + '"');
}

RemoteServiceInformation ApiSystem::getRemoteServiceStatus(RemoteServicesId id)
{
	LOG(LogInfo) << "ApiSystem::getRemoteServiceStatus() - id: " << std::to_string(id);

	return queryRemoteServiceStatus(id);
}

bool ApiSystem::configRemoteService(RemoteServiceInformation service)
{
	LOG(LogInfo) << "ApiSystem::configRemoteService()";

	return setRemoteServiceStatus(service);
}

void ApiSystem::launchExternalWindow_before(Window *window, const std::string command)
{
	LOG(LogDebug) << "ApiSystem::launchExternalWindow_before";

    // Backup Brightness and Volume
	ApiSystem::backupAfterGameValues();

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();
	InputManager::getInstance()->deinit();
	window->deinit();

	LOG(LogDebug) << "ApiSystem::launchExternalWindow_before OK";

	Scripting::fireEvent("application-start", command);
}

void ApiSystem::launchExternalWindow_after(Window *window, const std::string command)
{
	LOG(LogDebug) << "ApiSystem::launchExternalWindow_after";

	Scripting::fireEvent("application-end", command);

    // Restore Brightness and Volume
	ApiSystem::restoreAfterGameValues();

	window->init();
	InputManager::getInstance()->init();
	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->init();
	window->normalizeNextUpdate();
	window->reactivateGui();

	AudioManager::getInstance()->playRandomMusic();

	LOG(LogDebug) << "ApiSystem::launchExternalWindow_after OK";
}

bool ApiSystem::launchKodi(Window *window)
{
	LOG(LogDebug) << "ApiSystem::launchKodi()";

	std::string command = "Kodi.sh";

	ApiSystem::launchExternalWindow_before(window, command);

	int exitCode = system(command.c_str());

	// WIFEXITED returns a nonzero value if the child process terminated normally with exit or _exit.
	// https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
	if (WIFEXITED(exitCode))
		exitCode = WEXITSTATUS(exitCode);

	ApiSystem::launchExternalWindow_after(window, command);

	// handle end of kodi
	switch (exitCode)
	{
	case 10: // reboot code
		quitES(QuitMode::REBOOT);
		return true;

	case 11: // shutdown code
		quitES(QuitMode::SHUTDOWN);
		return true;
	}

	return exitCode == 0;
}

bool ApiSystem::launchBluetoothConfigurator(Window *window)
{
	std::string command = "Bluetooth.sh";

	ApiSystem::launchExternalWindow_before(window, command);

	int exitCode = system(command.c_str());

	// WIFEXITED returns a nonzero value if the child process terminated normally with exit or _exit.
	// https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
	if (WIFEXITED(exitCode))
		exitCode = WEXITSTATUS(exitCode);

	ApiSystem::launchExternalWindow_after(window, command);

	return exitCode == 0;
}

bool ApiSystem::isBluetoothActive()
{
	LOG(LogInfo) << "ApiSystem::isBluetoothActive()";

	return executeSystemBoolScript("es-bluetooth is_bluetooth_active");
}

bool ApiSystem::isBluetoothEnabled()
{
	LOG(LogInfo) << "ApiSystem::isBluetoothEnabled()";

	return stringToState(queryBluetoothEnabled());
}

bool ApiSystem::enableBluetooth()
{
	LOG(LogInfo) << "ApiSystem::disableBluetooth()";

	return executeSystemScript("es-bluetooth enable");
}

bool ApiSystem::disableBluetooth()
{
	LOG(LogInfo) << "ApiSystem::disableBluetooth()";

	return executeSystemScript("es-bluetooth disable");
}

bool ApiSystem::isBluetoothAudioDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::isBluetoothAudioDevice() - ID: " << id;

	return executeSystemBoolScript("es-bluetooth is_bluetooth_audio_device \"" + id + "\"" );
}

bool ApiSystem::isBluetoothAudioDeviceConnected()
{
	LOG(LogInfo) << "ApiSystem::disableBluetooth()";

	return executeSystemBoolScript("es-bluetooth is_bluetooth_audio_device_connected");
}

std::vector<BluetoothDevice> ApiSystem::toBluetoothDevicesVector(std::vector<std::string> btDevices)
{
	LOG(LogInfo) << "ApiSystem::getBluetoothDevices()";

	std::vector<BluetoothDevice> result;
	for (auto btDevice : btDevices)
	{
		BluetoothDevice bt_device;

		if (Utils::String::startsWith(btDevice, "<device "))
		{
			bt_device.id = Utils::String::extractString(btDevice, "id=\"", "\"", false);
			bt_device.name = Utils::String::extractString(btDevice, "name=\"", "\"", false);
			bt_device.type = Utils::String::extractString(btDevice, "type=\"", "\"", false);
			bt_device.connected = Utils::String::toBool( Utils::String::extractString(btDevice, "connected=\"", "\"", false) );
			bt_device.paired = Utils::String::toBool( Utils::String::extractString(btDevice, "paired=\"", "\"", false) );

			if (Utils::String::startsWith(bt_device.type, "audio-"))
				bt_device.isAudioDevice = true;
		}
		else
			bt_device.id = btDevice;

		result.push_back(bt_device);
	}
	return result;
}

std::vector<BluetoothDevice> ApiSystem::getBluetoothNewDevices()
{
	LOG(LogInfo) << "ApiSystem::getBluetoothNewDevices()";

	return toBluetoothDevicesVector( executeEnumerationScript("es-bluetooth new_devices") );
}

std::vector<BluetoothDevice> ApiSystem::getBluetoothPairedDevices()
{
	LOG(LogInfo) << "ApiSystem::getBluetoothPairedDevices()";

	return toBluetoothDevicesVector( executeEnumerationScript("es-bluetooth paired_devices") );
}

std::vector<BluetoothDevice> ApiSystem::getBluetoothConnectedDevices()
{
	LOG(LogInfo) << "ApiSystem::getBluetoothConnectedDevices()";

	return toBluetoothDevicesVector( executeEnumerationScript("es-bluetooth connected_devices") );
}

bool ApiSystem::pairBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::pairBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth pair_device \"" + id + "\"" );
}

BluetoothDevice ApiSystem::getBluetoothDeviceInfo(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::getBluetoothDeviceInfo() - ID: " << id;

	std::string device_info = getShOutput("es-bluetooth info_device \"" + id + "\"" );

	BluetoothDevice bt_device;

	if (Utils::String::startsWith(device_info, "<device "))
	{
		bt_device.id = Utils::String::extractString(device_info, "id=\"", "\"", false);
		bt_device.name = Utils::String::extractString(device_info, "name=\"", "\"", false);
		bt_device.type = Utils::String::extractString(device_info, "type=\"", "\"", false);
		bt_device.connected = Utils::String::toBool( Utils::String::extractString(device_info, "connected=\"", "\"", false) );
		bt_device.paired = Utils::String::toBool( Utils::String::extractString(device_info, "paired=\"", "\"", false) );

		if (Utils::String::startsWith(bt_device.type, "audio-"))
			bt_device.isAudioDevice = true;
	}
	else
		bt_device.id = id;

	return bt_device;
}

bool ApiSystem::connectBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::connectBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth connect_device \"" + id + "\"" );
}

bool ApiSystem::disconnectBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::disconnectBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth disconnect_device \"" + id + "\"" );
}

bool ApiSystem::disconnectAllBluetoothDevices()
{
	LOG(LogInfo) << "ApiSystem::disconnectAllBluetoothDevices() ";

	return executeSystemScript("es-bluetooth disconnect_all_devices" );
}

bool ApiSystem::deleteBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::deleteBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth delete_device_connection \"" + id + "\"" );
}

bool ApiSystem::deleteAllBluetoothDevices()
{
	LOG(LogInfo) << "ApiSystem::deleteAllBluetoothDevices()";

	return executeSystemScript("es-bluetooth delete_all_device_connections" );
}

std::string ApiSystem::getBluetoothAudioDevice()
{
	LOG(LogInfo) << "ApiSystem::getBluetoothAudioDevice()";

	return getShOutput(R"(es-bluetooth audio_device_connected)");
}

bool ApiSystem::startBluetoothLiveScan()
{
	LOG(LogInfo) << "ApiSystem::startBluetoothLiveScan()";

	return executeSystemScript("es-bluetooth scan_on &");
}

bool ApiSystem::stopBluetoothLiveScan()
{
	LOG(LogInfo) << "ApiSystem::stopBluetoothLiveScan()";

	return executeSystemScript("es-bluetooth scan_off &");
}

bool ApiSystem::startAutoConnectBluetoothAudioDevice()
{
	LOG(LogInfo) << "ApiSystem::startAutoConnectBluetoothAudioDevice()";

	return executeSystemScript("es-bluetooth auto_connect_audio_device_on &");
}

bool ApiSystem::stopAutoConnectBluetoothAudioDevice()
{
	LOG(LogInfo) << "ApiSystem::stopAutoConnectBluetoothAudioDevice()";

	return executeSystemScript("es-bluetooth auto_connect_audio_device_off &");
}

void ApiSystem::backupAfterGameValues()
{
	LOG(LogInfo) << "ApiSystem::backupAfterGameValues()";

	if (Settings::getInstance()->getBool("RestoreBrightnesAfterGame"))
		ApiSystem::backupBrightnessLevel();

	if (Settings::getInstance()->getBool("RestoreVolumeAfterGame"))
		ApiSystem::backupVolume();
}

void ApiSystem::restoreAfterGameValues()
{
	LOG(LogInfo) << "ApiSystem::restoreAfterGameValues()";

	if (Settings::getInstance()->getBool("RestoreBrightnesAfterGame"))
		ApiSystem::restoreBrightnessLevel();

	if (Settings::getInstance()->getBool("RestoreVolumeAfterGame"))
		ApiSystem::restoreVolume();
}
