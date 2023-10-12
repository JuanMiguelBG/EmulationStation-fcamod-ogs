#include "ApiSystem.h"
#include <stdlib.h>
#include <sys/statvfs.h>
#include "HttpReq.h"
#include "utils/AsyncUtil.h"
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
#include "DisplayPanelControl.h"
#include "InputManager.h"
#include "EsLocale.h"
#include <algorithm>
#include "guis/GuiMsgBox.h"
#include "Scripting.h"
#include "EmulationStation.h"
#include "SystemConf.h"

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
		case TEST_INPUT:
				executables.push_back("TestControls.sh");
				break;
		case CALIBRATE_TV:
				executables.push_back("CalibrateTv.sh");
				break;
		case GAMELIST:
				executables.push_back("es-gamelist");
				break;
		case CONTROLLERS:
				executables.push_back("es-controllers");
				break;
		case DATETIME:
				executables.push_back("es-datetime");
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

std::string ApiSystem::getFreeSpaceDriveInfo(const std::string mountpoint)
{
	LOG(LogInfo) << "ApiSystem::getFreeSpaceDriveInfo() - mount point: " << mountpoint;
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

std::string ApiSystem::getHostname()
{
	LOG(LogInfo) << "ApiSystem::getHostname()";

	return getShOutput("es-system_inf get_hostname");
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

bool ApiSystem::loadSystemWifiInfoToSystemConf(const std::string& wifiInfo)
{
	LOG(LogDebug) << "ApiSystem::loadSystemWifiInfoToSystemConf()";

	if (Utils::String::startsWith(wifiInfo, "<wifi_info "))
	{
		SystemConf::getInstance()->setBool("wifi.enabled", Utils::String::toBool(Utils::String::extractString(wifiInfo, "enabled=\"", "\"", false)));
		std::string ssid = Utils::String::extractString(wifiInfo, "ssid=\"", "\"", false);
		if (SystemConf::getInstance()->get("wifi.ssid").empty() || (ssid != SystemConf::getInstance()->get("wifi.ssid")))
			SystemConf::getInstance()->set("wifi.ssid", ssid);

		if (!SystemConf::getInstance()->get("wifi.ssid").empty())
			SystemConf::getInstance()->set("wifi.key", Utils::String::extractString(wifiInfo, "key=\"", "\"", false));

		SystemConf::getInstance()->set("system.hostname", Utils::String::extractString(wifiInfo, "hostname=\"", "\"", false));
		SystemConf::getInstance()->set("wifi.dns1", Utils::String::extractString(wifiInfo, "dns1=\"", "\"", false));
		SystemConf::getInstance()->set("wifi.dns2", Utils::String::extractString(wifiInfo, "dns2=\"", "\"", false));
		SystemConf::getInstance()->set("already.connection.exist.flag", Utils::String::extractString(wifiInfo, "flag=\"", "\"", false));
		return true;
	}

	return false;
}

bool ApiSystem::loadSystemWifiInfo()
{
	LOG(LogInfo) << "ApiSystem::loadSystemWifiInfo()";

	return loadSystemWifiInfoToSystemConf( getShOutput(R"(es-wifi get_wifi_info)") );
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

CpuAndSocketInformation ApiSystem::loadCpuAndChipsetInformation(const std::string& socInfo)
{
	LOG(LogInfo) << "ApiSystem::loadCpuAndChipsetInformation()";

	CpuAndSocketInformation chipset;

	if (Utils::String::startsWith(socInfo, "<soc_info "))
	{
		chipset.soc_name = Utils::String::extractString(socInfo, "name=\"", "\"", false);
		chipset.vendor_id = Utils::String::extractString(socInfo, "vendor=\"", "\"", false);
		chipset.model_name = Utils::String::extractString(socInfo, "model=\"", "\"", false);
		chipset.ncpus = std::atoi( Utils::String::extractString(socInfo, "ncpus=\"", "\"", false).c_str() );
		chipset.architecture = Utils::String::extractString(socInfo, "architecture=\"", "\"", false);
		chipset.nthreads_core = std::atoi( Utils::String::extractString(socInfo, "nthreads_core=\"", "\"", false).c_str() );
		chipset.cpu_load =  std::atof( Utils::String::extractString(socInfo, "cpu_load=\"", "\"", false).c_str() );
		chipset.temperature =  std::atof( Utils::String::extractString(socInfo, "temperature=\"", "\"", false).c_str() );
		chipset.governor = Utils::String::extractString(socInfo, "governor=\"", "\"", false);
		chipset.frequency = std::atoi( Utils::String::extractString(socInfo, "frequency=\"", "\"", false).c_str() );
		chipset.frequency_max = std::atoi( Utils::String::extractString(socInfo, "frequency_max=\"", "\"", false).c_str() );
		chipset.frequency_min = std::atoi( Utils::String::extractString(socInfo, "frequency_min=\"", "\"", false).c_str() );
	}

	return chipset;
}

CpuAndSocketInformation ApiSystem::getCpuAndChipsetInformation()
{
	LOG(LogInfo) << "ApiSystem::getCpuAndChipsetInformation()";

	return loadCpuAndChipsetInformation(getShOutput(R"(es-system_inf get_soc_info)"));
}

RamMemoryInformation ApiSystem::loadRamMemoryInformation(const std::string& memoryInfo)
{
	LOG(LogInfo) << "ApiSystem::loadRamMemoryInformation()";

	RamMemoryInformation memory;

	if (Utils::String::startsWith(memoryInfo, "<memory_info "))
	{
		memory.total = std::atof( Utils::String::extractString(memoryInfo, "total=\"", "\"", false).c_str() );
		memory.free = std::atof( Utils::String::extractString(memoryInfo, "free=\"", "\"", false).c_str() );
		memory.used = std::atof( Utils::String::extractString(memoryInfo, "used=\"", "\"", false).c_str() );
		memory.cached = std::atof( Utils::String::extractString(memoryInfo, "cached=\"", "\"", false).c_str() );
	}

	return memory;
}

RamMemoryInformation ApiSystem::getRamMemoryInformation()
{
	LOG(LogInfo) << "ApiSystem::getRamMemoryInformation()";

	return loadRamMemoryInformation(getShOutput(R"(es-system_inf get_memory_info)"));
}

DisplayAndGpuInformation ApiSystem::loadDisplayAndGpuInformation(const std::string& gpuInfo)
{
	LOG(LogInfo) << "ApiSystem::loadDisplayAndGpuInformation()";

	DisplayAndGpuInformation gpu;

	if (Utils::String::startsWith(gpuInfo, "<gpu_info "))
	{
		gpu.gpu_model = Utils::String::extractString(gpuInfo, "model=\"", "\"", false);
		gpu.resolution = Utils::String::extractString(gpuInfo, "resolution=\"", "\"", false);
		gpu.bits_per_pixel = std::atoi( Utils::String::extractString(gpuInfo, "bpp=\"", "\"", false).c_str() );
		gpu.temperature = std::atof( Utils::String::extractString(gpuInfo, "temperature=\"", "\"", false).c_str() );
		gpu.governor = Utils::String::extractString(gpuInfo, "governor=\"", "\"", false);
		gpu.frequency = std::atoi( Utils::String::extractString(gpuInfo, "frequency=\"", "\"", false).c_str() );
		gpu.frequency_max = std::atoi( Utils::String::extractString(gpuInfo, "frequency_max=\"", "\"", false).c_str());
		gpu.frequency_min = std::atoi( Utils::String::extractString(gpuInfo, "frequency_min=\"", "\"", false).c_str() );
		gpu.brightness_level = std::atoi( Utils::String::extractString(gpuInfo, "brightness=\"", "\"", false).c_str() );
		gpu.gamma_level = std::atoi( Utils::String::extractString(gpuInfo, "gamma=\"", "\"", false).c_str() );
		gpu.contrast_level = std::atoi( Utils::String::extractString(gpuInfo, "contrast=\"", "\"", false).c_str() );
		gpu.saturation_level = std::atoi( Utils::String::extractString(gpuInfo, "saturation=\"", "\"", false).c_str() );
		gpu.hue_level = std::atoi( Utils::String::extractString(gpuInfo, "hue=\"", "\"", false).c_str() );
	}

	return gpu;
}

DisplayAndGpuInformation ApiSystem::getDisplayAndGpuInformation()
{
	LOG(LogInfo) << "ApiSystem::getDisplayAndGpuInformation()";

	return loadDisplayAndGpuInformation(getShOutput(R"(es-system_inf get_gpu_info)"));
}

SoftwareInformation ApiSystem::loadSoftwareInformation(const std::string& softwareInfo)
{
	LOG(LogDebug) << "ApiSystem::loadSoftwareInformation()";

	SoftwareInformation si;

	if (Utils::String::startsWith(softwareInfo, "<software_info "))
	{
		si.application_name = Utils::String::extractString(softwareInfo, "name=\"", "\"", false);
		si.version = Utils::String::extractString(softwareInfo, "version=\"", "\"", false);
		si.es_version = Utils::String::toUpper(PROGRAM_VERSION_STRING);
		si.so_base = Utils::String::extractString(softwareInfo, "base_os=\"", "\"", false);
		si.vlinux = Utils::String::extractString(softwareInfo, "kernel=\"", "\"", false);
		si.hostname = Utils::String::extractString(softwareInfo, "hostname=\"", "\"", false);
		si.es_built = PROGRAM_BUILT_STRING;
	}

	return si;
}

SoftwareInformation ApiSystem::getSoftwareInformation()
{
	LOG(LogInfo) << "ApiSystem::getSoftwareInformation()";

	return loadSoftwareInformation(getShOutput(R"(es-system_inf get_software_info)"));
}

DeviceInformation ApiSystem::loadDeviceInformation(const std::string& deviceInfo)
{
	LOG(LogDebug) << "ApiSystem::loadDeviceInformation()";

	DeviceInformation di;

	if (Utils::String::startsWith(deviceInfo, "<device_info "))
	{
		di.name = Utils::String::extractString(deviceInfo, "name=\"", "\"", false);
		di.hardware = Utils::String::extractString(deviceInfo, "hardware=\"", "\"", false);
		di.revision = Utils::String::extractString(deviceInfo, "revision=\"", "\"", false);
		di.serial = Utils::String::extractString(deviceInfo, "serial=\"", "\"", false);
		di.machine_id = Utils::String::extractString(deviceInfo, "machine_id=\"", "\"", false);
		di.boot_id = Utils::String::extractString(deviceInfo, "boot_id=\"", "\"", false);
	}

	return di;
}
DeviceInformation ApiSystem::getDeviceInformation()
{
	LOG(LogInfo) << "ApiSystem::getDeviceInformation()";

	return loadDeviceInformation(getShOutput(R"(es-system_inf get_device_info)"));
}

int ApiSystem::getBrightnessLevel()
{
	LOG(LogInfo) << "ApiSystem::getBrightnessLevel()";

	return DisplayPanelControl::getInstance()->getBrightnessLevel();
}

void ApiSystem::setBrightnessLevel(int brightnessLevel)
{
	LOG(LogInfo) << "ApiSystem::setBrightnessLevel() - brightnessLevel: " << std::to_string(brightnessLevel);

	DisplayPanelControl::getInstance()->setBrightnessLevel(brightnessLevel);
}

int ApiSystem::getBrightness()
{
	LOG(LogInfo) << "ApiSystem::getBrightness()";

	return DisplayPanelControl::getInstance()->getBrightness();
}

void ApiSystem::backupBrightnessLevel()
{
	LOG(LogInfo) << "ApiSystem::backupBrightnessLevel()";

	Settings::getInstance()->setInt("BrightnessBackup", getBrightnessLevel());
}

void ApiSystem::restoreBrightnessLevel()
{
	LOG(LogInfo) << "ApiSystem::restoreBrightnessLevel()";

	setBrightnessLevel(Settings::getInstance()->getInt("BrightnessBackup"));
}

void ApiSystem::setGammaLevel(int gammaLevel)
{
	LOG(LogInfo) << "ApiSystem::setGammaLevel() - gamma level: " << std::to_string(gammaLevel);

	DisplayPanelControl::getInstance()->setGammaLevel(gammaLevel);
}

int ApiSystem::getGammaLevel()
{
	LOG(LogInfo) << "ApiSystem::getGammaLevel()";

	return DisplayPanelControl::getInstance()->getGammaLevel();
}

void ApiSystem::setContrastLevel(int contrastLevel)
{
	LOG(LogInfo) << "ApiSystem::setContrastLevel() - contrast level: " << std::to_string(contrastLevel);

	DisplayPanelControl::getInstance()->setContrastLevel(contrastLevel);
}

int ApiSystem::getContrastLevel()
{
	LOG(LogInfo) << "ApiSystem::getGammaLevel()";

	return DisplayPanelControl::getInstance()->getContrastLevel();
}

void ApiSystem::setSaturationLevel(int saturationLevel)
{
	LOG(LogInfo) << "ApiSystem::setSaturationLevel() - saturation level: " << std::to_string(saturationLevel);

	DisplayPanelControl::getInstance()->setSaturationLevel(saturationLevel);
}

int ApiSystem::getSaturationLevel()
{
	LOG(LogInfo) << "ApiSystem::getSaturationLevel()";

	return DisplayPanelControl::getInstance()->getSaturationLevel();
}

void ApiSystem::setHueLevel(int hueLevel)
{
	LOG(LogInfo) << "ApiSystem::setHueLevel() - hue level: " << std::to_string(hueLevel);

	DisplayPanelControl::getInstance()->setHueLevel(hueLevel);
}

int ApiSystem::getHueLevel()
{
	LOG(LogInfo) << "ApiSystem::getHueLevel()";

	return DisplayPanelControl::getInstance()->getHueLevel();
}

void ApiSystem::resetDisplayPanelSettings()
{
	LOG(LogInfo) << "ApiSystem::resetDisplayPanelSettings()";

	return DisplayPanelControl::getInstance()->resetDisplayPanelSettings();
}

bool ApiSystem::loadSystemHdmiInfoToSystemConf(const std::string& hdmiInfo)
{
	LOG(LogInfo) << "ApiSystem::loadSystemHdmiInfoToSystemConf()";

	if (Utils::String::startsWith(hdmiInfo, "<hdmi_info "))
	{
		SystemConf::getInstance()->setBool("hdmi.mode", Utils::String::toBool(Utils::String::extractString(hdmiInfo, "active=\"", "\"", false)));
		SystemConf::getInstance()->set("hdmi.resolution", Utils::String::extractString(hdmiInfo, "resolution=\"", "\"", false));
		SystemConf::getInstance()->set("hdmi.resolutions", Utils::String::extractString(hdmiInfo, "resolutions=\"", "\"", false));
		SystemConf::getInstance()->set("hdmi.default.resolution", Utils::String::extractString(hdmiInfo, "default=\"", "\"", false));
		return true;
	}

	return false;
}

bool ApiSystem::loadSystemHdmiInfo()
{
	LOG(LogInfo) << "ApiSystem::loadSystemHdmiInfo()";

	return loadSystemHdmiInfoToSystemConf(getShOutput(R"(es-display get_hdmi_info)"));
}

bool ApiSystem::setHdmiResolution(const std::string resolution)
{
	LOG(LogInfo) << "ApiSystem::setHdmiResolution() - " << resolution;

	return executeSystemScript("es-display set_hdmi_resolution \"" + resolution + "\" &");
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

	return getShOutput("es-system_inf get_device_name");
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

bool ApiSystem::getInternetStatus()
{
	LOG(LogInfo) << "ApiSystem::getInternetStatus()";
	if (doPing())
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

std::vector<std::string> ApiSystem::getKnowedWifiNetworks()
{
	LOG(LogInfo) << "ApiSystem::getKnowedWifiNetworks()";

	return executeEnumerationScript("es-wifi knowed_wifis");
}

bool ApiSystem::forgetWifiNetwork(const std::string ssid)
{
	LOG(LogInfo) << "ApiSystem::forgetWifiNetwork() - ssid: '" << ssid << "'";

	return executeSystemScript("es-wifi forget_wifi \"" + ssid + '"');
}

bool ApiSystem::forgetAllKnowedWifiNetworks()
{
	LOG(LogInfo) << "ApiSystem::forgetAllKnowedWifiNetworks()";

	return executeSystemScript("es-wifi forget_all_wifis");
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
		auto cmd = getSevenZipCommand() + " l -slt \"" + fileName + '"';
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

		LOG(LogDebug) << "ApiSystem::unzipFile() << KO Bad format?" << fileName;
		return false;
	}

	LOG(LogDebug) << "ApiSystem::unzipFile() is using 7z";

	std::string cmd = getSevenZipCommand() + " x \"" + Utils::FileSystem::getPreferredPath(fileName) + "\" -y -o\"" + Utils::FileSystem::getPreferredPath(destFolder) + '"';
	bool ret = executeSystemScript(cmd);
	LOG(LogDebug) << "ApiSystem::unzipFile() <<";
	return ret;
}

void ApiSystem::preloadVLC()
{
	LOG(LogInfo) << "ApiSystem::preloadVLC()";
	executeSystemScript("/usr/local/bin/es-preload_vlc &");
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

bool ApiSystem::configSystemAudio()
{
	return executeSystemScript(R"(es-sound config_sound_card &)");
}

bool ApiSystem::loadSystemAudioInfoToSystemConf(const std::string& audioInfo)
{
	LOG(LogDebug) << "ApiSystem::loadSystemAudioInfoToSystemConf()";

	if (Utils::String::startsWith(audioInfo, "<audio_info "))
	{
		SystemConf::getInstance()->set("sound.card", Utils::String::extractString(audioInfo, "card=\"", "\"", false));
		SystemConf::getInstance()->set("sound.cards", Utils::String::extractString(audioInfo, "cards=\"", "\"", false));
		SystemConf::getInstance()->set("sound.audio.device", Utils::String::extractString(audioInfo, "device=\"", "\"", false));
		SystemConf::getInstance()->set("sound.output.device", Utils::String::extractString(audioInfo, "output=\"", "\"", false));
		SystemConf::getInstance()->set("sound.output.devices", Utils::String::extractString(audioInfo, "outputs=\"", "\"", false));
		return true;
	}

	return false;
}

bool ApiSystem::loadSystemAudioInfo()
{
	LOG(LogInfo) << "ApiSystem::loadSystemAudioInfo()";

	return loadSystemAudioInfoToSystemConf( getShOutput(R"(es-sound get audio_info)") );
}

bool ApiSystem::setAudioCard(const std::string& audio_card)
{
	LOG(LogInfo) << "ApiSystem::setAudioCard() - " << audio_card;

	return executeSystemScript("es-sound set audio_card \"" + audio_card + '"');
}

bool ApiSystem::setOutputDevice(const std::string& device)
{
	LOG(LogInfo) << "ApiSystem::setOutputDevice() - " << device;

	return executeSystemScript("es-sound set output_device \"" + device + '"');
}

std::map<RemoteServicesId, RemoteServiceInformation> ApiSystem::toRemoteServicesStatusVector(std::vector<std::string> remoteServices)
{
	LOG(LogInfo) << "ApiSystem::toRemoteServicesStatusVector()";

	std::map<RemoteServicesId, RemoteServiceInformation> result;
	for (auto remoteService : remoteServices)
	{
		RemoteServiceInformation remote_service;

		if (Utils::String::startsWith(remoteService, "<service "))
		{
			remote_service.platformName = Utils::String::extractString(remoteService, "name=\"", "\"", false);
			remote_service.name = getRemoteServiceNameFromPlatformName(remote_service.platformName);
			remote_service.id = getRemoteServiceIdFromPlatformName(remote_service.platformName);
			remote_service.isActive = Utils::String::toBool( Utils::String::extractString(remoteService, "active=\"", "\"", false) );
			remote_service.isStartOnBoot = Utils::String::toBool( Utils::String::extractString(remoteService, "boot=\"", "\"", false) );
		}
		else
		{
			remote_service.platformName = remoteService;
			remote_service.name = getRemoteServiceNameFromPlatformName(remoteService);
			remote_service.id = getRemoteServiceIdFromPlatformName(remoteService);
		}

		result[remote_service.id] = remote_service;
	}
	return result;
}

std::map<RemoteServicesId, RemoteServiceInformation> ApiSystem::getAllRemoteServiceStatus()
{
	LOG(LogInfo) << "ApiSystem::getAllRemoteServiceStatus()";

	return toRemoteServicesStatusVector( executeEnumerationScript("es-remote_services get_all_status") );
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

void ApiSystem::launchExternalWindow_before(Window *window, const std::string command, bool silent)
{
	LOG(LogDebug) << "ApiSystem::launchExternalWindow_before";

	if (!silent)
	{
		// Backup Brightness and Volume
		ApiSystem::backupAfterGameValues();

		AudioManager::getInstance()->deinit();
		VolumeControl::getInstance()->deinit();
		InputManager::getInstance()->deinit();
		DisplayPanelControl::getInstance()->deinit();
		window->deinit();
	}

	LOG(LogDebug) << "ApiSystem::launchExternalWindow_before OK";

	Scripting::fireEvent("application-start", command);
}

void ApiSystem::launchExternalWindow_after(Window *window, const std::string command, bool silent)
{
	LOG(LogDebug) << "ApiSystem::launchExternalWindow_after";

	Scripting::fireEvent("application-end", command);

	if (!silent)
	{
		// Restore Brightness and Volume
		ApiSystem::restoreAfterGameValues();

		window->init();
		DisplayPanelControl::getInstance()->init();
		InputManager::getInstance()->init();
		VolumeControl::getInstance()->init();
		AudioManager::getInstance()->init();
		window->normalizeNextUpdate();
		window->reactivateGui();

		AudioManager::getInstance()->playRandomMusic();
	}

	LOG(LogDebug) << "ApiSystem::launchExternalWindow_after OK";
}

bool ApiSystem::launchKodi(Window *window, bool silent)
{
	LOG(LogDebug) << "ApiSystem::launchKodi()";

	std::string command = "Kodi.sh";

	ApiSystem::launchExternalWindow_before(window, command, silent);

	int exitCode = system(command.c_str());

	// WIFEXITED returns a nonzero value if the child process terminated normally with exit or _exit.
	// https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
	if (WIFEXITED(exitCode))
		exitCode = WEXITSTATUS(exitCode);

	ApiSystem::launchExternalWindow_after(window, command, silent);

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

bool ApiSystem::launchTestControls(Window *window)
{
	std::string command = "TestControls.sh";

	ApiSystem::launchExternalWindow_before(window, command);

	int exitCode = system(command.c_str());

	// WIFEXITED returns a nonzero value if the child process terminated normally with exit or _exit.
	// https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
	if (WIFEXITED(exitCode))
		exitCode = WEXITSTATUS(exitCode);

	ApiSystem::launchExternalWindow_after(window, command);

	return exitCode == 0;
}

bool ApiSystem::launchCalibrateTv(Window *window)
{
	std::string command = "CalibrateTv.sh";

	ApiSystem::launchExternalWindow_before(window, command);

	int exitCode = system(command.c_str());

	// WIFEXITED returns a nonzero value if the child process terminated normally with exit or _exit.
	// https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
	if (WIFEXITED(exitCode))
		exitCode = WEXITSTATUS(exitCode);

	ApiSystem::launchExternalWindow_after(window, command);

	return exitCode == 0;
}

bool ApiSystem::loadSystemBluetoothInfoToSystemConf(const std::string& btInfo)
{
	LOG(LogDebug) << "ApiSystem::loadSystemBluetoothInfoToSystemConf()";

	if (Utils::String::startsWith(btInfo, "<bt_info "))
	{
		bool btEnabled = Utils::String::toBool(Utils::String::extractString(btInfo, "enabled=\"", "\"", false));
		SystemConf::getInstance()->setBool("bluetooth.enabled", btEnabled);
		std::string btAudioDevice = "";
		if (btEnabled)
			btAudioDevice = Utils::String::extractString(btInfo, "audio=\"", "\"", false);

		SystemConf::getInstance()->set("bluetooth.audio.device", btAudioDevice);
		SystemConf::getInstance()->setBool("bluetooth.audio.connected", !btAudioDevice.empty());
		SystemConf::getInstance()->setBool("bluetooth.xbox_one.compatible", Utils::String::toBool(Utils::String::extractString(btInfo, "xbox_one_compatible=\"", "\"", false)));
		return true;
	}

	return false;
}

bool ApiSystem::loadSystemBluetoothInfo()
{
	LOG(LogInfo) << "ApiSystem::loadSystemBluetoothInfo()";

	return loadSystemBluetoothInfoToSystemConf( getShOutput(R"(es-bluetooth get_bt_info)") );
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

	return executeSystemBoolScript("es-bluetooth is_bluetooth_audio_device \"" + id + '"' );
}

bool ApiSystem::isBluetoothAudioDeviceConnected()
{
	LOG(LogInfo) << "ApiSystem::disableBluetooth()";

	return executeSystemBoolScript("es-bluetooth is_bluetooth_audio_device_connected");
}

std::vector<BluetoothDevice> ApiSystem::toBluetoothDevicesVector(std::vector<std::string> btDevices)
{
	LOG(LogInfo) << "ApiSystem::toBluetoothDevicesVector()";

	std::vector<BluetoothDevice> result;
	for (auto btDevice : btDevices)
	{
		BluetoothDevice bt_device;

		if (Utils::String::startsWith(btDevice, "<device "))
		{
			bt_device.id = Utils::String::extractString(btDevice, "id=\"", "\"", false);
			bt_device.name = Utils::String::extractString(btDevice, "name=\"", "\"", false);
			bt_device.alias = Utils::String::extractString(btDevice, "alias=\"", "\"", false);
			bt_device.type = Utils::String::extractString(btDevice, "type=\"", "\"", false);
			bt_device.paired = Utils::String::toBool( Utils::String::extractString(btDevice, "paired=\"", "\"", false) );
			bt_device.connected = Utils::String::toBool( Utils::String::extractString(btDevice, "connected=\"", "\"", false) );

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

	return executeSystemScript("es-bluetooth pair_device \"" + id + '"' );
}

bool ApiSystem::unpairBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::unpairBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth unpair_device \"" + id + '"' );
}

BluetoothDevice ApiSystem::getBluetoothDeviceInfo(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::getBluetoothDeviceInfo() - ID: " << id;

	std::string device_info = getShOutput("es-bluetooth info_device \"" + id + '"' );

	BluetoothDevice bt_device;

	if (Utils::String::startsWith(device_info, "<device "))
	{
		bt_device.id = Utils::String::extractString(device_info, "id=\"", "\"", false);
		bt_device.name = Utils::String::extractString(device_info, "name=\"", "\"", false);
		bt_device.alias = Utils::String::extractString(device_info, "alias=\"", "\"", false);
		bt_device.type = Utils::String::extractString(device_info, "type=\"", "\"", false);
		bt_device.paired = Utils::String::toBool( Utils::String::extractString(device_info, "paired=\"", "\"", false) );
		bt_device.connected = Utils::String::toBool( Utils::String::extractString(device_info, "connected=\"", "\"", false) );

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

	return executeSystemScript("es-bluetooth connect_device \"" + id + '"' );
}

bool ApiSystem::disconnectBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::disconnectBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth disconnect_device \"" + id + '"' );
}

bool ApiSystem::disconnectAllBluetoothDevices()
{
	LOG(LogInfo) << "ApiSystem::disconnectAllBluetoothDevices() ";

	return executeSystemScript("es-bluetooth disconnect_all_devices" );
}

bool ApiSystem::deleteBluetoothDevice(const std::string id)
{
	LOG(LogInfo) << "ApiSystem::deleteBluetoothDevice() - ID: " << id;

	return executeSystemScript("es-bluetooth delete_device_connection \"" + id + '"' );
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

bool ApiSystem::setBluetoothDeviceAlias(const std::string id, const std::string alias)
{
	LOG(LogInfo) << "ApiSystem::setBluetoothDeviceAlias() - id: " << id << ", alias: " << alias;

	return executeSystemScript("es-bluetooth set_device_alias \"" + id + "\" \"" + alias + '"' );
}

bool ApiSystem::setBluetoothXboxOneCompatible(bool compatible)
{
	LOG(LogInfo) << "ApiSystem::setBluetoothXboxOneCompatible() - compatible: " << Utils::String::boolToString(compatible);

	std::string command = "es-bluetooth ";
	if (compatible)
		command.append("active_xbox_one_pad_compatibility");
	else
		command.append("inactive_xbox_one_pad_compatibility");

	command.append(" &");
	return executeSystemScript(command);
}

bool ApiSystem::setDateTime(const std::string datetime)
{
	LOG(LogInfo) << "ApiSystem::setDateTime() - datetime: " << datetime;

	if (datetime.empty())
		return false;

	return executeSystemScript("es-datetime set datetime \"" + datetime + "\" &");
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

bool ApiSystem::clearLastPlayedData(const std::string system)
{
	LOG(LogInfo) << "ApiSystem::clearLastPlayedData() - system: " << system;

	if (system.empty())
		return executeSystemScript("es-gamelist clear_all_last_played_data" );
	else
		return executeSystemScript("es-gamelist clear_last_played_data \"" + system + '"' );

}

std::vector<StorageDevice> ApiSystem::toStorageDevicesVector(std::vector<std::string> stDevices)
{
	LOG(LogInfo) << "ApiSystem::toStorageDevicesVector()";

	std::vector<StorageDevice> result;
	for (auto stDevice : stDevices)
	{
		StorageDevice st_device;

		if (Utils::String::startsWith(stDevice, "<device "))
		{
			st_device.name = Utils::String::extractString(stDevice, "name=\"", "\"", false);
			st_device.mount_point = Utils::String::extractString(stDevice, "mount_point=\"", "\"", false);
			st_device.used = Utils::String::extractString(stDevice, "used=\"", "\"", false);
			st_device.used_percent = Utils::String::extractString(stDevice, "used_percent=\"", "\"", false);
			st_device.avail = Utils::String::extractString(stDevice, "avail=\"", "\"", false);
		}

		result.push_back(st_device);
	}
	return result;
}

std::vector<StorageDevice> ApiSystem::getStorageDevices(bool all)
{
	LOG(LogInfo) << "ApiSystem::getAllStorageDevices()";
	std::string command("es-system_inf ");
	if (all)
		command.append("get_all_storage_info");
	else
		command.append("get_user_storage_info");
	
	return toStorageDevicesVector( executeEnumerationScript(command) );
}

SystemSummaryInfo ApiSystem::loadSummaryInformation(const std::string& summaryInfo)
{
	LOG(LogInfo) << "ApiSystem::loadSummaryInformation()";

	SystemSummaryInfo info;

	if (Utils::String::startsWith(summaryInfo, "<summary_info "))
	{
		info.device = Utils::String::extractString(summaryInfo, "device=\"", "\"", false);
		info.cpu_load = std::atof( Utils::String::extractString(summaryInfo, "cpu_load=\"", "\"", false).c_str() );
		info.cpu_temperature = std::atof( Utils::String::extractString(summaryInfo, "cpu_temperature=\"", "\"", false).c_str() );
		info.gpu_temperature = std::atof( Utils::String::extractString(summaryInfo, "gpu_temperature=\"", "\"", false).c_str() );
		info.batt_temperature = std::atof( Utils::String::extractString(summaryInfo, "batt_temperature=\"", "\"", false).c_str() );
		info.wifi_connected = Utils::String::toBool( Utils::String::extractString(summaryInfo, "wifi_connected=\"", "\"", false) );
		info.internet_status = Utils::String::toBool( Utils::String::extractString(summaryInfo, "internet_status=\"", "\"", false) );
		info.wifi_ssid = Utils::String::extractString(summaryInfo, "ssid=\"", "\"", false);
		info.ip_address = Utils::String::extractString(summaryInfo, "ip_address=\"", "\"", false);
	}

	return info;
}

SystemSummaryInfo ApiSystem::getSummaryInformation()
{
	LOG(LogInfo) << "ApiSystem::getSummaryInformation()";
	return loadSummaryInformation(getShOutput(R"(es-system_inf get_summary_info)"));
}

void ApiSystem::loadOtherSettings(bool loadAllData)
{
	LOG(LogInfo) << "ApiSystem::loadOtherSettings() - executing";
	Utils::Async::run( [loadAllData] (void)
		{
			if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::WIFI))
				ApiSystem::getInstance()->loadSystemWifiInfo();

			if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::BLUETOOTH))
				ApiSystem::getInstance()->loadSystemBluetoothInfo();

			if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SOUND))
				ApiSystem::getInstance()->loadSystemAudioInfo();

			if (loadAllData)
			{
				if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::OPTMIZE_SYSTEM))
				SystemConf::getInstance()->set("suspend.device.mode", ApiSystem::getInstance()->getSuspendMode());

				if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SYSTEM_INFORMATION))
				{
					SoftwareInformation software = ApiSystem::getInstance()->getSoftwareInformation();
					SystemConf::getInstance()->set("software.application_name", software.application_name);
					SystemConf::getInstance()->set("software.version", software.version);
					SystemConf::getInstance()->set("software.es_version", software.es_version);
				}
			}
		});
	LOG(LogDebug) << "ApiSystem::loadOtherSettings() - exit";
}
