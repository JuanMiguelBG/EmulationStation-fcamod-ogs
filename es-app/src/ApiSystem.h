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
	std::map<RemoteServicesId, RemoteServiceInformation> toRemoteServicesStatusVector(std::vector<std::string> remoteServices);
	bool loadSystemHdmiInfoToSystemConf(const std::string& hdmiInfo);
	bool loadSystemAudioInfoToSystemConf(const std::string& audioInfo);
	bool loadSystemWifiInfoToSystemConf(const std::string& wifiInfo);
	bool loadSystemBluetoothInfoToSystemConf(const std::string& btInfo);
	SoftwareInformation loadSoftwareInformation(const std::string& softwareInfo);
	DeviceInformation loadDeviceInformation(const std::string& deviceInfo);
	DisplayAndGpuInformation loadDisplayAndGpuInformation(const std::string& gpuInfo);
	RamMemoryInformation loadRamMemoryInformation(const std::string& memoryInfo);
	std::vector<StorageDevice> toStorageDevicesVector(std::vector<std::string> stDevices);
	CpuAndSocketInformation loadCpuAndChipsetInformation(const std::string& socInfo);
	SystemSummaryInfo loadSummaryInformation(const std::string& summaryInfo);

protected:
	ApiSystem();

	virtual bool executeScript(const std::string command);
	virtual std::pair<std::string, int> executeScript(const std::string command, const std::function<void(const std::string)>& func);
	virtual std::vector<std::string> executeEnumerationScript(const std::string command);
	virtual bool executeBoolScript(const std::string command);

	static ApiSystem* instance;

	void launchExternalWindow_before(Window *window, const std::string command, bool silent = false);
	void launchExternalWindow_after(Window *window, const std::string command, bool silent = false);

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
		REMOTE_SERVICES = 14,
		LOG_SCRIPTS = 15,
		GAMELIST = 16,
		KODI = 17,
		TEST_INPUT = 18,
		CALIBRATE_TV = 19,
		CONTROLLERS = 20,
		DATETIME = 21
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

	std::string getFreeSpaceDriveInfo(const std::string mountpoint);
	std::string getFreeSpaceInfo(const std::string mountpoint);

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
	CpuAndSocketInformation getCpuAndChipsetInformation();
	RamMemoryInformation getRamMemoryInformation();
	DisplayAndGpuInformation getDisplayAndGpuInformation();
	SoftwareInformation getSoftwareInformation();
	DeviceInformation getDeviceInformation();
	std::vector<StorageDevice> getStorageDevices(bool all = true);
	SystemSummaryInfo getSummaryInformation();

	int getBrightness();
	int getBrightnessLevel();
	void setBrightnessLevel(int brightnessLevel);
	void backupBrightnessLevel();
	void restoreBrightnessLevel();
	void setGammaLevel(int gammaLevel);
	int getGammaLevel();
	void setContrastLevel(int contrastLevel);
	int getContrastLevel();
	void setSaturationLevel(int saturationLevel);
	int getSaturationLevel();
	void setHueLevel(int hueLevel);
	int getHueLevel();
	void resetDisplayPanelSettings();
	bool loadSystemHdmiInfo();
	bool setHdmiResolution(const std::string resolution);

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

	bool loadSystemWifiInfo();
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
	bool isWifiPowerSafeEnabled();
	void setWifiPowerSafe(bool state);
	std::vector<std::string> getKnowedWifiNetworks();
	bool forgetWifiNetwork(const std::string ssid);
	bool forgetAllKnowedWifiNetworks();

	virtual std::vector<std::string> getRetroachievementsSoundsList();

	bool setOptimizeSystem(bool state);
	std::vector<std::string> getSuspendModes();
	std::string getSuspendMode();
	bool setSuspendMode(const std::string suspend_mode);

	bool isEsScriptsLoggingActivated();
	bool setEsScriptsLoggingActivated(bool state, const std::string level = "default", bool logWithNanoSeconds = false);

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

	int getVolume();
	void setVolume(int volumeLevel);
	void backupVolume();
	void restoreVolume();
	bool configSystemAudio();
	bool loadSystemAudioInfo();
	bool setAudioCard(const std::string& audio_card);
	bool setOutputDevice(const std::string& device);

	std::map<RemoteServicesId, RemoteServiceInformation> getAllRemoteServiceStatus();
	RemoteServiceInformation getRemoteServiceStatus(RemoteServicesId id);
	bool configRemoteService(RemoteServiceInformation service);

	virtual bool launchKodi(Window *window, bool silent = false);
	virtual bool launchTestControls(Window *window);
	virtual bool launchCalibrateTv(Window *window);

	bool loadSystemBluetoothInfo();
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
	bool unpairBluetoothDevice(const std::string id);
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
	bool setBluetoothDeviceAlias(const std::string id, const std::string alias);
	bool setBluetoothXboxOneCompatible(bool compatible);

    bool setDateTime(const std::string datetime);

	void backupAfterGameValues();
	void restoreAfterGameValues();

	bool clearLastPlayedData(const std::string system = "");

	void loadOtherSettings(bool loadAllData = true);
};

#endif
