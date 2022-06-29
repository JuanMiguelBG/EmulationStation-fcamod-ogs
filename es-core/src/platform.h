#pragma once
#ifndef ES_CORE_PLATFORM_H
#define ES_CORE_PLATFORM_H

#include <string>
#include <vector>

class Window;

enum QuitMode
{
	QUIT = 0,
	RESTART = 1,
	SHUTDOWN = 2,
	REBOOT = 3,
	SUSPEND = 4,
	FAST_SHUTDOWN = 5,
	FAST_REBOOT = 6
};

int runSystemCommand(const std::string& cmd_utf8, const std::string& name, Window* window); // run a utf-8 encoded in the shell (requires wstring conversion on Windows)
bool executeSystemScript(const std::string command);
std::vector<std::string> executeSystemEnumerationScript(const std::string command);
int quitES(QuitMode mode = QuitMode::QUIT);
bool isFastShutdown();
void processQuitMode();

#if !defined(TRACE)
	#define TRACE(s) 
#endif

class StopWatch
{
public:
	StopWatch(std::string name)
	{

	}

	~StopWatch()
	{
	}

private:
	int mTicks;
	std::string mName;
};

struct BatteryInformation
{
	BatteryInformation()
	{
		hasBattery = false;
		level = 0;
		isCharging = false;
		std::string health("Good");
		max_capacity = 0;
		voltage = 0.f;
		temperature = 0.f;
	}

	bool hasBattery;
	int  level;
	bool isCharging;
	std::string health;
	int max_capacity;
	float voltage;
	float temperature;
};

BatteryInformation queryBatteryInformation(bool summary);
int queryBatteryLevel();
bool queryBatteryCharging();
float queryBatteryVoltage();
float queryBatteryTemperature();

struct NetworkInformation
{
	NetworkInformation()
	{
		isConnected = false;
		iface = "";
		isEthernet = false;
		isWifi = false; // wifi
		isIPv6 = false;
		netmask = "";
		ip_address = "";
		mac = "";
		dns1 = "";
		dns2 = "";
		gateway = "";
		ssid = ""; // wifi
		signal = -1; // wifi
		channel = -1; // wifi
		security = ""; // wifi
		rate = 0;
		rate_unit = "";
	}

	bool isConnected;
	std::string iface;
	bool isEthernet;
	bool isWifi;
	bool isIPv6;
	std::string netmask;
	std::string ip_address;
	std::string mac;
	std::string gateway;
	std::string dns1;
	std::string dns2;
	std::string ssid;  // wifi
	int signal; // wifi
	int channel; // wifi
	std::string security; // wifi
	int rate;
	std::string rate_unit;
};

NetworkInformation queryNetworkInformation(bool summary);
std::string queryIPAddress();
bool queryWifiEnabled();
std::string queryWifiSsid();
std::string queryWifiPsk(std::string ssid = queryWifiSsid());
std::string queryDnsOne();
std::string queryDnsTwo();
bool queryNetworkConnected();
std::string queryWifiNetworkExistFlag();
bool queryNetworkConnectedFast();


struct CpuAndSocketInformation
{
	CpuAndSocketInformation()
	{
		soc_name = "N/A";
		vendor_id = "N/A";
		model_name = "N/A";
		ncpus = 0;
		architecture = "N/A";
		nthreads_core = 0;
		cpu_load = 0.f;
		temperature = 0.f;
		governor = "N/A";
		frequency = 0;
		frequency_max = 0;
		frequency_min = 0;
	}

	std::string soc_name;
	std::string vendor_id;
	std::string model_name;
	int ncpus;
	std::string architecture;
	int nthreads_core;
	float cpu_load;
	float temperature;
	std::string governor;
	int frequency;
	int frequency_max;
	int frequency_min;
};

CpuAndSocketInformation queryCpuAndChipsetInformation(bool summary);
float queryLoadCpu();
float queryTemperatureCpu();
int queryFrequencyCpu();
std::string querySocName();

struct RamMemoryInformation
{
	RamMemoryInformation()
	{
		total = 0.f;
		free = 0.f;
		used = 0.f;
		cached = 0.f;
	}

	float total;
	float free;
	float used;
	float cached;
};

RamMemoryInformation queryRamMemoryInformation(bool summary);

struct DisplayAndGpuInformation
{
	DisplayAndGpuInformation()
	{
		gpu_model = "N/A";
		resolution = "N/A";
		bits_per_pixel = 0;
		temperature = 0.f;
		governor = "N/A";
		frequency = 0;
		frequency_max = 0;
		frequency_min = 0;
		brightness_level = 0;
		brightness_system = 0;
		brightness_system_max = 0;
	}

	std::string gpu_model;
	std::string resolution;
	int bits_per_pixel;
	float temperature;
	std::string governor;
	int frequency;
	int frequency_max;
	int frequency_min;
	int brightness_level;
	int brightness_system;
	int brightness_system_max;
};

DisplayAndGpuInformation queryDisplayAndGpuInformation(bool summary);
float queryTemperatureGpu();
int queryFrequencyGpu();
int queryBrightness();
int queryMaxBrightness();
int queryBrightnessLevel();
void saveBrightnessLevel(int brightness_level);

struct SoftwareInformation
{
	SoftwareInformation()
	{
		hostname = "N/A";
		application_name = "N/A";
		version = "N/A";
		so_base = "N/A";
		linux = "N/A";
	}

	std::string hostname;
	std::string application_name;
	std::string version;
	std::string so_base;
	std::string linux;
};

std::string queryHostname();
bool setCurrentHostname(std::string hostname);
SoftwareInformation querySoftwareInformation(bool summary);

struct DeviceInformation
{
	DeviceInformation()
	{
		name = "N/A";
		hardware = "N/A";
		revision = "N/A";
		serial = "N/A";
		machine_id = "N/A";
		boot_id = "N/A";
	}

	std::string name;
	std::string hardware;
	std::string revision;
	std::string serial;
	std::string machine_id;
	std::string boot_id;
};

std::string queryDeviceName();
DeviceInformation queryDeviceInformation(bool summary);

bool isUsbDriveMounted(std::string device);
std::string queryUsbDriveMountPoint(std::string device);
std::vector<std::string> queryUsbDriveMountPoints();

std::string queryTimezones();
std::string queryCurrentTimezone();
bool setCurrentTimezone(std::string timezone);

#ifdef _DEBUG
uint32_t getVolume();
#endif

std::string queryBluetoothEnabled();

std::string getShOutput(const std::string& mStr);

#endif // ES_CORE_PLATFORM_H
