#ifndef API_SYSTEM
#define API_SYSTEM

#include <string>
#include <functional>
#include <vector>
#include "platform.h"

class Window;

namespace UpdateState
{
	enum State
	{
		NO_UPDATE,
		UPDATER_RUNNING,
		UPDATE_READY
	};
}

struct ThemeDownloadInfo
{
	bool installed;
	std::string name;
	std::string url;
};

class ApiSystem 
{
private:
	std::string stateToString(bool state);
	bool stringToState(const std::string state);

protected:
	ApiSystem();

	virtual bool executeScript(const std::string command);
	virtual std::pair<std::string, int> executeScript(const std::string command, const std::function<void(const std::string)>& func);
	virtual std::vector<std::string> executeEnumerationScript(const std::string command);

	static ApiSystem* instance;

public:
	enum ScriptId : unsigned int
	{
		TIMEZONE = 0,
		POWER_KEY = 1,
		DISPLAY = 2,
		SYSTEM_HOTKEY_EVENTS = 3,
		WIFI = 4,
		RETROACHIVEMENTS = 5,
		LANGUAGE = 6,
		SYSTEM_INFORMATION = 7,
		AUTO_SUSPEND = 8,
		OPTMIZE_SYSTEM = 9
/*
		RETROACHIVEMENTS = 1,
		BLUETOOTH = 2,
		RESOLUTION = 3,
		BIOSINFORMATION = 4,
		NETPLAY = 5,
		KODI = 6,
		GAMESETTINGS = 7,
		DECORATIONS = 8,
		SHADERS = 9,
		DISKFORMAT = 10,
		OVERCLOCK = 11,
		PDFEXTRACTION = 12,
		BATOCERASTORE = 13,
		EVMAPY = 14,
		THEMESDOWNLOADER = 15,
		THEBEZELPROJECT = 16,
		PADSINFO = 17
*/
	};

	virtual bool isScriptingSupported(ScriptId script);

	static ApiSystem* getInstance();
	virtual void deinit() { };

	unsigned long getFreeSpaceGB(std::string mountpoint);

	std::string getFreeSpaceBootInfo();
	std::string getFreeSpaceSystemInfo();
	std::string getFreeSpaceUserInfo();
	std::string getFreeSpaceUsbDriveInfo(const std::string mountpoint);
	std::string getFreeSpaceInfo(const std::string mountpoint);

	std::vector<std::string> getUsbDriveMountPoints();

	bool isFreeSpaceBootLimit();
	bool isFreeSpaceSystemLimit();
	bool isFreeSpaceUserLimit();
	bool isFreeSpaceUsbDriveLimit(const std::string mountpoint);

	bool isFreeSpaceLimit(const std::string mountpoint, int limit = 1); // GB
	bool isTemperatureLimit(float temperature, float limit = 70.0f); // ° C
	bool isLoadCpuLimit(float load_cpu, float limit = 90.0f); // %
	bool isMemoryLimit(float total_memory, float free_memory, int limit = 10); // %
	bool isBatteryLimit(float battery_level, int limit = 15); // %

	std::string getVersion();
	std::string getApplicationName();
	std::string getHostname();
	bool setHostname(std::string hostname);
	std::string getIpAddress();
	float getLoadCpu();
	float getTemperatureCpu();
	int getFrequencyCpu();
	float getTemperatureGpu();
	int getFrequencyGpu();
	bool isNetworkConnected();
	int getBatteryLevel();
	bool isBatteryCharging();
	float getBatteryVoltage();
	std::string getDeviceName();

	NetworkInformation getNetworkInformation(bool summary = true);
	BatteryInformation getBatteryInformation(bool summary = true);
	CpuAndSocketInformation getCpuAndChipsetInformation(bool summary = true);
	RamMemoryInformation getRamMemoryInformation(bool summary = true);
	DisplayAndGpuInformation getDisplayAndGpuInformation(bool summary = true);
	SoftwareInformation getSoftwareInformation(bool summary = true);
	DeviceInformation getDeviceInformation(bool summary = true);

	int getBrightness();
	int getBrightnessLevel();
	void setBrightnessLevel(int brightnessLevel);

	int getVolume();
	void setVolume(int volumeLevel);

	std::string getTimezones();
	std::string getCurrentTimezone();
	bool setTimezone(std::string timezone);

	bool isPowerkeyState();
	int getPowerkeyTimeInterval();
	std::string getPowerkeyAction();
	bool setPowerkeyValues(const std::string action, bool two_push_state, int time_interval);

	bool setDisplayBlinkLowBattery(bool state);

	bool isSystemHotkeyBrightnessEvent();
	bool isSystemHotkeyVolumeEvent();
	bool isSystemHotkeyWifiEvent();
	bool isSystemHotkeyPerformanceEvent();
	bool isSystemHotkeySuspendEvent();
	bool setSystemHotkeysValues(bool brightness_state, bool volume_state, bool wifi_state, bool performance_state, bool suspend_state);

	bool isDeviceAutoSuspend();
	bool isDeviceAutoSuspendStayAwakeCharging();
	bool isDeviceAutoSuspendByTime();
	int getAutoSuspendTimeout();
	bool isDeviceAutoSuspendByBatteryLevel();
	int getAutoSuspendBatteryLevel();
	bool setDeviceAutoSuspendValues(bool stay_awake_charging_state, bool time_state, int timeout, bool battery_state, int battery_level);

	bool isDisplayAutoDimStayAwakeCharging();
	bool isDisplayAutoDimByTime();
	int getDisplayAutoDimTimeout();
	int getDisplayAutoDimBrightness();
	bool setDisplayAutoDimValues(bool stay_awake_charging_state, bool time_state, int timeout, int brightness_level);

	virtual bool ping();
	bool getInternetStatus();
	std::vector<std::string> getWifiNetworks(bool scan = false);
	bool enableWifi(std::string ssid, std::string key);
	bool disconnectWifi(std::string ssid);
	bool disableWifi();
	bool isWifiEnabled();
	bool enableManualWifiDns(std::string ssid, std::string dnsOne, std::string dnsTwo);
	bool disableManualWifiDns(std::string ssid);
	std::string getWifiSsid();
	std::string getWifiPsk(std::string ssid);
	std::string getDnsOne();
	std::string getDnsTwo();


	bool setLanguage(std::string language);

	bool getRetroachievementsEnabled();
	bool getRetroachievementsHardcoreEnabled();
	bool getRetroachievementsLeaderboardsEnabled();
	bool getRetroachievementsVerboseEnabled();
	bool getRetroachievementsAutomaticScreenshotEnabled();
	bool getRetroachievementsUnlockSoundEnabled();
	virtual std::vector<std::string> getRetroachievementsSoundsList();
	std::string getRetroachievementsUsername();
	std::string getRetroachievementsPassword();
	bool getRetroachievementsChallengeIndicators();
	bool getRetroachievementsRichpresenceEnable();
	bool getRetroachievementsBadgesEnable();
	bool getRetroachievementsTestUnofficial();
	bool getRetroachievementsStartActive();
	bool setRetroachievementsValues(bool retroachievements_state, bool hardcore_state, bool leaderboards_state, bool verbose_state, bool automatic_screenshot_state, bool challenge_indicators_state, bool richpresence_state, bool badges_state, bool test_unofficial_state, bool start_active_state, const std::string sound, const std::string username, const std::string password);

	bool setOptimizeSystem(bool state);

	virtual std::string getSevenZipCommand() { return "7zr"; }

	virtual std::string getCRC32(const std::string fileName, bool fromZipContents = true);
	virtual std::string getMD5(const std::string fileName, bool fromZipContents = true);

	virtual bool unzipFile(const std::string fileName, const std::string destFolder = "", const std::function<bool(const std::string)>& shouldExtract = nullptr);

	static UpdateState::State state;

	std::pair<std::string, int> updateSystem(const std::function<void(const std::string)>& func = nullptr);
	std::string checkUpdateVersion();
	void startUpdate(Window* c);

	std::vector<ThemeDownloadInfo> getThemesList();
	std::pair<std::string, int> installTheme(std::string themeName, const std::function<void(const std::string)>& func = nullptr);
};

#endif
