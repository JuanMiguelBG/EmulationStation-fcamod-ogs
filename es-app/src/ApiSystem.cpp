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
#include "VolumeControl.h"
#include "EsLocale.h"
#include <algorithm>

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
	LOG(LogInfo) << "ApiSystem::executeScript() - Running -> " << command;

	FILE *pipe = popen(command.c_str(), "r");
	if (pipe == NULL)
	{
		LOG(LogError) << "Error executing " << command;
		return std::pair<std::string, int>("Error starting command : " + command, -1);
	}

	char line[1024];
	while (fgets(line, 1024, pipe))
	{
		strtok(line, "\n");

		// Long theme names/URL can crash the GUI MsgBox
		// "48" found by trials and errors. Ideally should be fixed
		// in es-core MsgBox -- FIXME
		if (strlen(line) > 48)
			line[47] = '\0';

		if (func != nullptr)
			func(std::string(line));
	}

	int exitCode = WEXITSTATUS(pclose(pipe));
	return std::pair<std::string, int>(line, exitCode);
}

bool ApiSystem::executeScript(const std::string command)
{
	return executeSystemScript(command);
}

bool ApiSystem::isScriptingSupported(ScriptId script)
{
	std::vector<std::string> executables;

	switch (script)
	{
		case TIMEZONE:
				executables.push_back("timezones");
				break;
		case POWER_KEY:
				executables.push_back("es-powerkey");
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
	case ApiSystem::RETROACHIVEMENTS:
				executables.push_back("es-cheevos");
				break;
	case ApiSystem::LANGUAGE:
				executables.push_back("es-language");
				break;
	case ApiSystem::SYSTEM_INFORMATION:
				executables.push_back("es-system_inf");
				break;
	case ApiSystem::AUTO_SUSPEND:
				executables.push_back("es-auto_suspend");
				break;
	case ApiSystem::OPTMIZE_SYSTEM:
				executables.push_back("es-optimize_system");
				break;
/*
	case ApiSystem::RETROACHIVEMENTS:
#ifdef CHEEVOS_DEV_LOGIN
		return true;
#endif
		break;
	case ApiSystem::KODI:
		executables.push_back("kodi");
		break;
	case ApiSystem::WIFI:
		executables.push_back("batocera-wifi");
		break;
	case ApiSystem::BLUETOOTH:
		executables.push_back("batocera-bluetooth");
		break;
	case ApiSystem::RESOLUTION:
		executables.push_back("batocera-resolution");
		break;
	case ApiSystem::BIOSINFORMATION:
		executables.push_back("batocera-systems");
		break;
	case ApiSystem::DISKFORMAT:
		executables.push_back("batocera-format");
		break;
	case ApiSystem::OVERCLOCK:
		executables.push_back("batocera-overclock");
		break;
	case ApiSystem::THEMESDOWNLOADER:
		executables.push_back("batocera-es-theme");
		break;
	case ApiSystem::NETPLAY:
		executables.push_back("7zr");
		break;
	case ApiSystem::PDFEXTRACTION:
		executables.push_back("pdftoppm");
		executables.push_back("pdfinfo");
		break;
	case ApiSystem::BATOCERASTORE:
		executables.push_back("batocera-store");
		break;
	case ApiSystem::THEBEZELPROJECT:
		executables.push_back("batocera-es-thebezelproject");
		break;
	case ApiSystem::PADSINFO:
		executables.push_back("batocera-padsinfo");
		break;
	case ApiSystem::EVMAPY:
		executables.push_back("evmapy");
		break;
*/
	}

//	if (executables.size() == 0)
//		return true;

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
	LOG(LogDebug) << "ApiSystem::getFreeSpaceGB";

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

std::string ApiSystem::getFreeSpaceUsbDriveInfo(const std::string mountpoint)
{
	LOG(LogDebug) << "ApiSystem::getFreeSpaceUsbDriveInfo() - mount point: " << mountpoint;
	if ( isUsbDriveMounted(mountpoint) )
		return getFreeSpaceInfo(mountpoint);

	return "";
}

std::string ApiSystem::getFreeSpaceInfo(const std::string mountpoint)
{
	LOG(LogDebug) << "ApiSystem::getFreeSpaceInfo() - mount point: " << mountpoint;

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

bool ApiSystem::isFreeSpaceUsbDriveLimit(const std::string mountpoint) {
	if ( isUsbDriveMounted(mountpoint) )
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
	LOG(LogDebug) << "ApiSystem::getVersion()";
	return querySoftwareInformation(true).version;
}

std::string ApiSystem::getApplicationName()
{
	LOG(LogDebug) << "ApiSystem::getApplicationName()";
	return querySoftwareInformation(true).application_name;
}

std::string ApiSystem::getHostname()
{
	LOG(LogDebug) << "ApiSystem::getHostname()";
	return queryHostname();
}

bool ApiSystem::setHostname(std::string hostname)
{
	LOG(LogDebug) << "ApiSystem::setHostname()";
	return setCurrentHostname(hostname);
}

std::string ApiSystem::getIpAddress()
{
	LOG(LogDebug) << "ApiSystem::getIpAddress()";

	std::string result = queryIPAddress(); // platform.h
	if (result.empty())
		return "___.___.___.___/__";

	return result;
}

float ApiSystem::getLoadCpu()
{
	LOG(LogDebug) << "ApiSystem::getLoadCpu()";
	return queryLoadCpu();
}

int ApiSystem::getFrequencyCpu()
{
	LOG(LogDebug) << "ApiSystem::getFrequencyCpu()";
	return queryFrequencyCpu();
}

float ApiSystem::getTemperatureCpu()
{
	LOG(LogDebug) << "ApiSystem::getTemperatureCpu()";
	return queryTemperatureCpu();
}

float ApiSystem::getTemperatureGpu()
{
	LOG(LogDebug) << "ApiSystem::getTemperatureGpu()";
	return queryTemperatureGpu();
}

int ApiSystem::getFrequencyGpu()
{
	LOG(LogDebug) << "ApiSystem::getFrequencyGpu()";
	return queryFrequencyGpu();
}

bool ApiSystem::isNetworkConnected()
{
	LOG(LogDebug) << "ApiSystem::isNetworkConnected()";
	return queryNetworkConnected();
}

NetworkInformation ApiSystem::getNetworkInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getNetworkInformation()";

	return queryNetworkInformation(summary); // platform.h
}

BatteryInformation ApiSystem::getBatteryInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getBatteryInformation()";

	return queryBatteryInformation(summary); // platform.h
}

CpuAndSocketInformation ApiSystem::getCpuAndChipsetInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getCpuAndChipsetInformation()";

	return queryCpuAndChipsetInformation(summary); // platform.h
}

RamMemoryInformation ApiSystem::getRamMemoryInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getRamMemoryInformation()";

	return queryRamMemoryInformation(summary); // platform.h
}

DisplayAndGpuInformation ApiSystem::getDisplayAndGpuInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getDisplayAndGpuInformation()";

	return queryDisplayAndGpuInformation(summary); // platform.h
}

SoftwareInformation ApiSystem::getSoftwareInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getSoftwareInformation()";

	return querySoftwareInformation(summary); // platform.h
}

DeviceInformation ApiSystem::getDeviceInformation(bool summary)
{
	LOG(LogDebug) << "ApiSystem::getDeviceInformation()";

	return queryDeviceInformation(summary); // platform.h
}

int ApiSystem::getBrightnessLevel()
{
	LOG(LogDebug) << "ApiSystem::getBrightnessLevel()";

	return queryBrightnessLevel();
}

void ApiSystem::setBrightnessLevel(int brightnessLevel)
{
	LOG(LogDebug) << "ApiSystem::setBrightnessLevel()";

	saveBrightnessLevel(brightnessLevel);
}

int ApiSystem::getBrightness()
{
	LOG(LogDebug) << "ApiSystem::getBrightness()";

	return queryBrightness();
}

int ApiSystem::getVolume()
{
	LOG(LogDebug) << "ApiSystem::getVolume()";

	return VolumeControl::getInstance()->getVolume();
}

void ApiSystem::setVolume(int volumeLevel)
{
	LOG(LogDebug) << "ApiSystem::setVolume()";

	VolumeControl::getInstance()->setVolume(volumeLevel);
}

int ApiSystem::getBatteryLevel()
{
	LOG(LogDebug) << "ApiSystem::getBatteryLevel()";

	return queryBatteryLevel();
}

bool ApiSystem::isBatteryCharging()
{
	LOG(LogDebug) << "ApiSystem::isBatteryCharging()";

	return queryBatteryCharging();
}

float ApiSystem::getBatteryVoltage()
{
	LOG(LogDebug) << "ApiSystem::getBatteryVoltage()";

	return queryBatteryVoltage();
}

std::string ApiSystem::getDeviceName()
{
	LOG(LogDebug) << "ApiSystem::getDeviceName()";

	return queryDeviceName();
}

std::string ApiSystem::getTimezones()
{
	LOG(LogDebug) << "ApiSystem::getTimezones()";

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

bool ApiSystem::isPowerkeyState()
{
	LOG(LogInfo) << "ApiSystem::isPowerkeyState()";

	return stringToState(getShOutput(R"(es-powerkey get two_push_shutdown)"));
}

int ApiSystem::getPowerkeyTimeInterval()
{
	LOG(LogInfo) << "ApiSystem::getPowerkeyTimeInterval()";
	
	std::string time_interval = Utils::String::replace(getShOutput(R"(es-powerkey get max_interval_time)"), "\n", "");
	if (time_interval.empty())
		return 5;

	int time = std::atoi( time_interval.c_str() );
	if (time < 1)
		time = 1;
	else if (time > 10)
		time = 10;

	return time;
}

std::string ApiSystem::getPowerkeyAction()
{
	LOG(LogInfo) << "ApiSystem::getPowerkeyAction()";

	std::string action = Utils::String::replace(getShOutput(R"(es-powerkey get action)"), "\n", "");
	if (action.empty())
		return "shutdown";

	return action;
}

bool ApiSystem::setPowerkeyValues(const std::string action, bool two_push_state, int time_interval)
{
	LOG(LogInfo) << "ApiSystem::setPowerkeyValues()";

	if (time_interval < 1)
		time_interval = 1;
	else if (time_interval > 10)
		time_interval = 10;

	return executeScript("es-powerkey set_all_values " + action + " " + stateToString(two_push_state) + " " + std::to_string(time_interval) + " &");
}

bool ApiSystem::setDisplayBlinkLowBattery(bool state)
{
	LOG(LogInfo) << "ApiSystem::setDisplayBlinkLowBattery()";

	return executeScript("es-display set blink_low_battery " + stateToString(state) + " &");
}

bool ApiSystem::isSystemHotkeyBrightnessEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyBrightnessEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get brightness)"));
}

bool ApiSystem::isSystemHotkeyVolumeEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyVolumeEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get volume)"));
}

bool ApiSystem::isSystemHotkeyWifiEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyWifiEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get wifi)"));
}

bool ApiSystem::isSystemHotkeyPerformanceEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeyPerformanceEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get performance)"));
}

bool ApiSystem::isSystemHotkeySuspendEvent()
{
	LOG(LogInfo) << "ApiSystem::isSystemHotkeySuspendEvent()";

	return stringToState(getShOutput(R"(es-system_hotkey get suspend)"));
}

bool ApiSystem::setSystemHotkeysValues(bool brightness_state, bool volume_state, bool wifi_state, bool performance_state, bool suspend_state)
{
	LOG(LogInfo) << "ApiSystem::setSystemHotkeysValues()";

	return executeScript("es-system_hotkey set_all_values " + stateToString(brightness_state) + " " + stateToString(volume_state) + " " + stateToString(wifi_state) + " " + stateToString(performance_state) + " " + stateToString(suspend_state) + " &");
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

	return executeScript("es-auto_suspend set_all_values " + stateToString(stay_awake_charging_state) + " " + stateToString(time_state) + " " + std::to_string(timeout) + " " + stateToString(battery_state) + " " + std::to_string(battery_level) + " &");

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

	return executeScript("es-display set_auto_dim_all_values " + stateToString(stay_awake_charging_state) + " " + stateToString(time_state) + " " + std::to_string(timeout) + " " + std::to_string(brightness_level) + " &");
}

bool ApiSystem::ping()
{
	if (!executeScript("timeout 1 ping -c 1 -t 255 8.8.8.8")) // ping Google DNS
		return executeScript("timeout 2 ping -c 1 -t 255 8.8.4.4"); // ping Google secondary DNS & give 2 seconds

	return true;
}

bool ApiSystem::getInternetStatus()
{
	LOG(LogInfo) << "ApiSystem::getInternetStatus()";
	if (ping())
		return true;

	return executeScript("es-wifi internet_status");
}

std::vector<std::string> ApiSystem::getWifiNetworks(bool scan)
{
	LOG(LogInfo) << "ApiSystem::getWifiNetworks()";

	return executeEnumerationScript(scan ? "es-wifi scanlist" : "es-wifi list");
}

bool ApiSystem::enableWifi(std::string ssid, std::string key)
{
	LOG(LogInfo) << "ApiSystem::enableWifi()";

	return executeScript("es-wifi enable \"" + ssid + "\" \"" + key + '"');
}

bool ApiSystem::disconnectWifi(std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::disconnectWifi() - SSID: " << ssid;

	return executeScript("es-wifi disconnect \"" + ssid + '"');
}

bool ApiSystem::disableWifi()
{
	LOG(LogInfo) << "ApiSystem::disableWifi()";

	return executeScript("es-wifi disable");
}

bool ApiSystem::isWifiEnabled()
{
	LOG(LogInfo) << "ApiSystem::disableWifi()";

	return queryWifiEnabled();
}

bool ApiSystem::enableManualWifiDns(std::string ssid, std::string dnsOne, std::string dnsTwo)
{
	LOG(LogInfo) << "ApiSystem::enableManualWifiDns()";

	return executeScript("es-wifi enable_manual_dns \"" + ssid + "\" \"" + dnsOne + "\" \"" + dnsTwo + '"');
}

bool ApiSystem::disableManualWifiDns(std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::disableManualWifiDns()";

	return executeScript("es-wifi disable_manual_dns \"" + ssid + '"');
}

std::string ApiSystem::getWifiSsid()
{
	LOG(LogInfo) << "ApiSystem::getWifiSsid()";

	return queryWifiSsid();
}

std::string ApiSystem::getWifiPsk(std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::getWifiPsk() - ssid: " << ssid;

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

std::string ApiSystem::stateToString(bool state)
{
	return state ? std::string("enabled") : std::string("disabled");
}

bool ApiSystem::stringToState(const std::string state)
{
	return ( Utils::String::replace(state, "\n", "") == "enabled" );
}

bool ApiSystem::setLanguage(std::string language)
{
	LOG(LogInfo) << "ApiSystem::setLanguage()";

	return executeScript("es-language set " + language + " &");
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

	LOG(LogDebug) << "ApiSystem::getRetroAchievementsSoundsList";

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

	return executeScript("es-cheevos set_all_values " + Utils::String::boolToString(retroachievements_state) + " " + Utils::String::boolToString(hardcore_state) + " " + Utils::String::boolToString(leaderboards_state) + " " + Utils::String::boolToString(verbose_state) + " " + Utils::String::boolToString(automatic_screenshot_state) + " " + Utils::String::boolToString(challenge_indicators_state) + " " + Utils::String::boolToString(richpresence_state) + " " + Utils::String::boolToString(badges_state) + " " + Utils::String::boolToString(test_unofficial_state) + " " + Utils::String::boolToString(start_active_state) + " \"" + sound + "\" \"" + username + "\" \"" + password + "\" &");
}

bool ApiSystem::setOptimizeSystem(bool state)
{
	LOG(LogInfo) << "ApiSystem::setOptimizeSystem()";

	return executeScript("es-optimize_system " + Utils::String::boolToString(state));
}


std::string ApiSystem::getMD5(const std::string fileName, bool fromZipContents)
{
	LOG(LogDebug) << "getMD5 >> " << fileName;

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

	LOG(LogDebug) << "getMD5 << " << ret;

	return ret;
}

std::string ApiSystem::getCRC32(std::string fileName, bool fromZipContents)
{
	LOG(LogDebug) << "getCRC32 >> " << fileName;

	std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(fileName));

	if (ext == ".7z" && fromZipContents)
	{
		LOG(LogDebug) << "getCRC32 is using 7z";

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
		LOG(LogDebug) << "getCRC32 is using ZipFile";

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

	LOG(LogDebug) << "getCRC32 is using fileBuffer";
	return Utils::FileSystem::getFileCrc32(fileName);
}

bool ApiSystem::unzipFile(const std::string fileName, const std::string destFolder, const std::function<bool(const std::string)>& shouldExtract)
{
	LOG(LogDebug) << "unzipFile >> " << fileName << " to " << destFolder;

	if (!Utils::FileSystem::exists(destFolder))
		Utils::FileSystem::createDirectory(destFolder);

	if (Utils::String::toLower(Utils::FileSystem::getExtension(fileName)) == ".zip")
	{
		LOG(LogDebug) << "unzipFile is using ZipFile";

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

			LOG(LogDebug) << "unzipFile << OK";
			return true;
		}

		LOG(LogDebug) << "unzipFile << KO Bad format ?" << fileName;
		return false;
	}

	LOG(LogDebug) << "unzipFile is using 7z";

	std::string cmd = getSevenZipCommand() + " x \"" + Utils::FileSystem::getPreferredPath(fileName) + "\" -y -o\"" + Utils::FileSystem::getPreferredPath(destFolder) + "\"";
	bool ret = executeScript(cmd);
	LOG(LogDebug) << "unzipFile <<";
	return ret;
}
