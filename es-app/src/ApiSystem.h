#ifndef API_SYSTEM
#define API_SYSTEM

#include <string>
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
	std::vector<BluetoothDevice> toBluetoothDevicesVector(std::vector<std::string> btDevices);

protected:
	ApiSystem();

	virtual bool executeScript(const std::string command);
	virtual std::pair<std::string, int> executeScript(const std::string command, const std::function<void(const std::string)>& func);
	virtual std::vector<std::string> executeEnumerationScript(const std::string command);
	virtual bool executeBoolScript(const std::string command);

	static ApiSystem* instance;

	void launchExternalWindow_before(Window *window, const std::string command);
	void launchExternalWindow_after(Window *window, const std::string command);

public:
	enum ScriptId : unsigned int
	{
		TIMEZONE = 0,
		DISPLAY = 1,
		SYSTEM_HOTKEY_EVENTS = 2,
		WIFI = 3,
		RETROACHIVEMENTS = 4,
		LANGUAGE = 5,
		SYSTEM_INFORMATION = 6,
		AUTO_SUSPEND = 7,
		OPTMIZE_SYSTEM = 8,
		SHOW_FPS = 9,
		OVERCLOCK = 10,
		PRELOAD_VLC = 11,
		SOUND = 12,
		BLUETOOTH = 13,
		KODI = 14,
		REMOTE_SERVICES = 15,
		LOG_SCRIPTS = 16
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
	std::string getFreeSpaceUser2Info();
	bool isUser2Mounted();
	std::string getFreeSpaceUsbDriveInfo(const std::string mountpoint);
	std::string getFreeSpaceInfo(const std::string mountpoint);

	std::vector<std::string> getUsbDriveMountPoints();

	bool isFreeSpaceBootLimit();
	bool isFreeSpaceSystemLimit();
	bool isFreeSpaceUserLimit();
	bool isFreeSpaceUser2Limit();
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
	float getTemperatureBattery();
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
	void backupBrightnessLevel();
	void restoreBrightnessLevel();

	int getVolume();
	void setVolume(int volumeLevel);
	void backupVolume();
	void restoreVolume();

	std::string getTimezones();
	std::string getCurrentTimezone();
	bool setTimezone(std::string timezone);

	bool setDisplayBlinkLowBattery(bool state);

	bool isSystemHotkeyBrightnessEvent();
	int getSystemHotkeyBrightnessStep();
	bool isSystemHotkeyVolumeEvent();
	int getSystemHotkeyVolumeStep();
	bool isSystemHotkeyWifiEvent();
	bool isSystemHotkeyBluetoothEvent();
	bool isSystemHotkeySpeakerEvent();
	bool isSystemHotkeySuspendEvent();
	bool setSystemHotkeysValues(SystemHotkeyValues values);
	SystemHotkeyValues getSystemHotkeyValues();

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
	bool connectWifi(const std::string ssid, const std::string pwd);
	bool disconnectWifi(const std::string ssid);
	bool enableWifi(bool background = true);
	bool disableWifi(bool background = true);
	bool resetWifi(const std::string ssid);
	bool isWifiEnabled();
	bool enableManualWifiDns(const std::string ssid, const std::string dnsOne, const std::string dnsTwo);
	bool disableManualWifiDns(const std::string ssid);
	std::string getWifiSsid();
	std::string getWifiPsk(const std::string ssid);
	std::string getDnsOne();
	std::string getDnsTwo();
	std::string getWifiNetworkExistFlag();
	bool isWifiPowerSafeEnabled();
	void setWifiPowerSafe(bool state);

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
	bool isEsScriptsLoggingActivated();
	bool setEsScriptsLoggingActivated(bool state, const std::string level = "default", bool logWithNanoSeconds = false);
	
	bool setShowRetroarchFps(bool state);
	bool isShowRetroarchFps();

	bool setOverclockSystem(bool state);
	bool isOverclockSystem();

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

	void preloadVLC();

	std::vector<std::string> getAudioCards();
	std::vector<std::string> getAudioDevices();
	std::vector<std::string> getOutputDevices();
	std::string getOutputDevice();
	bool setOutputDevice(const std::string device);

	RemoteServiceInformation getRemoteServiceStatus(RemoteServicesId id);
	bool configRemoteService(RemoteServiceInformation service);

	virtual bool launchKodi(Window *window);
	virtual bool launchBluetoothConfigurator(Window *window);

	bool isBluetoothActive();
	bool isBluetoothEnabled();
	bool enableBluetooth();
	bool disableBluetooth();
	bool isBluetoothAudioDevice(const std::string id);
	bool isBluetoothAudioDeviceConnected();
	std::vector<BluetoothDevice> getBluetoothNewDevices();
	std::vector<BluetoothDevice> getBluetoothPairedDevices();
	std::vector<BluetoothDevice> getBluetoothConnectedDevices();
	bool pairBluetoothDevice(const std::string id);
	BluetoothDevice getBluetoothDeviceInfo(const std::string id);
	bool connectBluetoothDevice(const std::string id);
	bool disconnectBluetoothDevice(const std::string id);
	bool disconnectAllBluetoothDevices();
	bool deleteBluetoothDevice(const std::string id);
	bool deleteAllBluetoothDevices();
	std::string getBluetoothAudioDevice();
	bool startBluetoothLiveScan();
	bool stopBluetoothLiveScan();
	bool startAutoConnectBluetoothAudioDevice();
	bool stopAutoConnectBluetoothAudioDevice();

	void backupAfterGameValues();
	void restoreAfterGameValues();
};

#endif
