#include "platform.h"
#include <SDL_events.h>

#include <unistd.h>

#include "Window.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Scripting.h"

#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <vector>
#include <map>

int runShutdownCommand()
{
	LOG(LogInfo) << "Paltform::runShutdownCommand()";
	Log::flush();
	return system("sudo shutdown -h now");
}

int runRestartCommand()
{
	LOG(LogInfo) << "Paltform::runRestartCommand()";
	Log::flush();
	return system("sudo shutdown -r now");
}

int runSuspendCommand()
{
	LOG(LogInfo) << "Paltform::runSuspendCommand()";
	Log::flush();
	return system("sudo systemctl suspend");
}

void splitCommand(std::string cmd, std::string* executable, std::string* parameters)
{
	std::string c = Utils::String::trim(cmd);
	size_t exec_end;

	if (c[0] == '\"')
	{
		exec_end = c.find_first_of('\"', 1);
		if (std::string::npos != exec_end)
		{
			*executable = c.substr(1, exec_end - 1);
			*parameters = c.substr(exec_end + 1);
		}
		else
		{
			*executable = c.substr(1, exec_end);
			std::string().swap(*parameters);
		}
	}
	else
	{
		exec_end = c.find_first_of(' ', 0);
		if (std::string::npos != exec_end)
		{
			*executable = c.substr(0, exec_end);
			*parameters = c.substr(exec_end + 1);
		}
		else
		{
			*executable = c.substr(0, exec_end);
			std::string().swap(*parameters);
		}
	}
}

int runSystemCommand(const std::string& cmd_utf8, const std::string& name, Window* window)
{
	return system(cmd_utf8.c_str());
}

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
	quitMode = mode;

	switch (quitMode)
	{
		case QuitMode::QUIT:
			Scripting::fireEvent("quit");
			break;

		case QuitMode::REBOOT:
		case QuitMode::FAST_REBOOT:
			Scripting::fireEvent("quit", "reboot");
			Scripting::fireEvent("reboot");
			break;

		case QuitMode::SHUTDOWN:
		case QuitMode::FAST_SHUTDOWN:
			Scripting::fireEvent("quit", "shutdown");
			Scripting::fireEvent("shutdown");
			break;

		case QuitMode::SUSPEND:
			Scripting::fireEvent("quit", "suspend");
			Scripting::fireEvent("suspend");
			break;
	}

	SDL_Event* quit = new SDL_Event();
	quit->type = SDL_QUIT;
	SDL_PushEvent(quit);
	return 0;
}

bool executeSystemScript(const std::string command)
{
	LOG(LogInfo) << "Platform::executeSystemScript() - Running -> " << command;

	if (system(command.c_str()) == 0)
		return true;

	LOG(LogError) << "Platform::executeSystemScript() - Error executing " << command;
	return false;
}

void touch(const std::string& filename)
{
	int fd = open(filename.c_str(), O_CREAT|O_WRONLY, 0644);
	if (fd >= 0)
		close(fd);
}

std::vector<std::string> executeSystemEnumerationScript(const std::string command)
{
	LOG(LogDebug) << "Platform::executeSystemEnumerationScript -> " << command;

	std::vector<std::string> res;

	FILE *pipe = popen(command.c_str(), "r");

	if (pipe == NULL)
		return res;

	char line[1024];
	while (fgets(line, 1024, pipe))
	{
		strtok(line, "\n");
		std::string linestr = Utils::String::replace( line, "\n", "" );
		if (!linestr.empty())
			res.push_back(linestr);
	}

	pclose(pipe);
	return res;
}

std::map<std::string,std::string> executeSystemMapScript(const std::string command, const char separator)
{
	LOG(LogDebug) << "Platform::executeSystemMapScript -> " << command;

	std::map<std::string,std::string> res;

	FILE *pipe = popen(command.c_str(), "r");

	if (pipe == NULL)
		return res;

	char line[1024];
	while (fgets(line, 1024, pipe))
	{
		strtok(line, "\n");
		std::string linestr = Utils::String::replace( line, "\n", "" );
		std::vector<std::string> data = Utils::String::split(linestr, separator);
		if (data.size() == 1)
			res[data[0]] = "";
		else if (data.size() > 1)
			res[data[0]] = data[1];
	}

	pclose(pipe);
	return res;
}

void processQuitMode()
{
	switch (quitMode)
	{
	case QuitMode::RESTART:
		LOG(LogInfo) << "Platform::processQuitMode() - Restarting EmulationStation";
		touch("/tmp/es-restart");
		break;
	case QuitMode::REBOOT:
	case QuitMode::FAST_REBOOT:
		LOG(LogInfo) << "Platform::processQuitMode() - Rebooting system";
		touch("/tmp/es-sysrestart");
		runRestartCommand();
		break;
	case QuitMode::SHUTDOWN:
	case QuitMode::FAST_SHUTDOWN:
		LOG(LogInfo) << "Platform::processQuitMode() - Shutting system down";
		touch("/tmp/es-shutdown");
		runShutdownCommand();
		break;
	case QuitMode::SUSPEND:
		LOG(LogInfo) << "Platform::processQuitMode() - Suspend system";
		runSuspendCommand();
		break;
	}
}

bool isFastShutdown()
{
	return quitMode == QuitMode::FAST_REBOOT || quitMode == QuitMode::FAST_SHUTDOWN;
}

std::string queryBatteryRootPath()
{
	static std::string batteryRootPath;

	if (batteryRootPath.empty())
	{
		auto files = Utils::FileSystem::getDirContent("/sys/class/power_supply");
		for (auto file : files)
		{
			if (Utils::String::toLower(file).find("/bat") != std::string::npos)
			{
				batteryRootPath = file;
				break;
			}
		}
	}
	return batteryRootPath;
}

BatteryInformation queryBatteryInformation(bool summary)
{
	BatteryInformation ret;

	std::string batteryRootPath = queryBatteryRootPath();

	// Find battery path - only at the first call
	if (!queryBatteryRootPath().empty())
	{
		try
		{
			ret.hasBattery = true;
			ret.level = queryBatteryLevel();
			ret.isCharging = queryBatteryCharging();
			if (!summary)
			{
				ret.health = Utils::String::toLower( Utils::String::replace( Utils::FileSystem::readAllText(batteryRootPath + "/health"), "\n", "" ) );
				ret.max_capacity = std::atoi(Utils::FileSystem::readAllText(batteryRootPath + "/charge_full").c_str()) / 1000; // milli amperes
				ret.voltage = queryBatteryVoltage();
				ret.temperature = queryBatteryTemperature();
			}
		} catch (...) {
			LOG(LogError) << "Platform::queryBatteryInformation() - Error reading battery data!!!";
		}
	}

	return ret;
}

int queryBatteryLevel()
{
	std::string batteryCapacityPath = queryBatteryRootPath() + "/capacity";
	if ( Utils::FileSystem::exists(batteryCapacityPath) )
		return std::atoi(Utils::FileSystem::readAllText(batteryCapacityPath).c_str());

	return 0;
}

bool queryBatteryCharging()
{
	std::string batteryStatusPath = queryBatteryRootPath() + "/status";
	if ( Utils::FileSystem::exists(batteryStatusPath) )
		return Utils::String::compareIgnoreCase( Utils::String::replace(Utils::FileSystem::readAllText(batteryStatusPath), "\n", ""), "discharging" );

	return false;
}

float queryBatteryVoltage()
{
	std::string batteryVoltagePath = queryBatteryRootPath() + "/voltage_now";
	if ( Utils::FileSystem::exists(batteryVoltagePath) )
		return std::atof(Utils::FileSystem::readAllText(batteryVoltagePath).c_str()) / 1000000; // volts

	return false;
}

float queryBatteryTemperature()
{
	try
	{
		if (Utils::FileSystem::exists("/sys/devices/virtual/thermal/thermal_zone2/temp"))
		{
			float temperature = std::atof( getShOutput(R"(cat /sys/devices/virtual/thermal/thermal_zone2/temp)").c_str() );
			return temperature / 1000;
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryTemperatureCpu() - Error reading Battery temperature  data!!!";
	}
	return 0.f;
}

std::string calculateIp4Netmask(std::string nbits_str)
{
	std::string netmask("");
	int bits = std::atoi( nbits_str.c_str() ),
			parts,
			rest;

	if (bits >= 8)
	{
		parts = bits / 8,
		rest = bits % 8;

		for (int i = 0 ; i < parts; i++)
			netmask.append("255.");
	}
	else
	{
		parts = 0;
		rest = bits;
	}

	std::string binary("");
	binary.append(rest, '1').append(8 - rest, '0');

	netmask.append( std::to_string( std::stoi(binary.c_str(), 0, 2) ) );
	parts++;

	if (parts < 4)
	{
		netmask.append(".");
		for (int i = parts ; i < 4; i++)
		{
			netmask.append( "0" );
			if (i < 3)
				netmask.append(".");
		}
	}

	return netmask;
}

NetworkInformation queryNetworkInformation(bool summary)
{
	NetworkInformation network;

	try
	{
		struct ifaddrs *ifAddrStruct = NULL;
		struct ifaddrs *ifa = NULL;
		void *tmpAddrPtr = NULL;

		getifaddrs(&ifAddrStruct);

		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
		{
			if (!ifa->ifa_addr)
				continue;

			// check it is IP4 is a valid IP4 Address
			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
				char addressBuffer[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

				std::string ifName = ifa->ifa_name;
				if (ifName.find("eth") != std::string::npos || ifName.find("wlan") != std::string::npos || ifName.find("mlan") != std::string::npos || ifName.find("en") != std::string::npos || ifName.find("wl") != std::string::npos || ifName.find("p2p") != std::string::npos)
				{
					network.isConnected = true;
					network.iface = ifName;
					network.ssid = "";
					network.isEthernet = ifName.find("eth") != std::string::npos;
					network.isWifi = ifName.find("wlan") != std::string::npos;
					network.isIPv6 = false;
					network.ip_address = std::string(addressBuffer);
					break;
				}
			}
		}
		// Seeking for ipv6 if no IPV4
		if (!network.isConnected)
		{
			for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
			{
				if (!ifa->ifa_addr)
					continue;

				// check it is IP6 is a valid IP6 Address
				if (ifa->ifa_addr->sa_family == AF_INET6)
				{
					tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
					char addressBuffer[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);

					std::string ifName = ifa->ifa_name;
					if (ifName.find("eth") != std::string::npos || ifName.find("wlan") != std::string::npos || ifName.find("mlan") != std::string::npos || ifName.find("en") != std::string::npos || ifName.find("wl") != std::string::npos || ifName.find("p2p") != std::string::npos)
					{
						network.isConnected = true;
						network.iface = ifName;
						network.ssid = "";
						network.isEthernet = ifName.find("eth") != std::string::npos;
						network.isWifi = ifName.find("wlan") != std::string::npos;
						network.isIPv6 = true;
						network.ip_address = std::string(addressBuffer);
						break;
					}
				}
			}
		}

		if (ifAddrStruct != NULL)
			freeifaddrs(ifAddrStruct);

		if ( queryNetworkConnected() ) // NetworkManager running
		{
			std::string cmd("");

			char result_buffer[256];
			std::string field(""),
									awk_flag(""),
									nmcli_command("nmcli -f %s device show %s | awk '%s {print $2}'");

			if (network.isWifi)
			{
				field.clear();
				field.append( "GENERAL.CONNECTION" ); // wifi network ssid
				snprintf(result_buffer, 128, "nmcli -f %s device show %s | awk '{for (i=2; i<NF; i++) printf $i \" \"; print $NF}'", field.c_str(), network.iface.c_str());
				network.ssid = getShOutput( result_buffer );
			}

			std::string ip_version( (network.isIPv6 ? "ip6" : "ip4") );
			if (summary)
			{
				field.clear();
				field.append( ip_version ).append(".address"); // IP address with netmask prefix
				snprintf(result_buffer, 128, nmcli_command.c_str(), field.c_str(), network.iface.c_str(), "");
				network.ip_address = getShOutput( result_buffer );
			}
			else
			{
				field.clear();
				field.append( ip_version ).append(".address"); // IP address with netmask prefix
				awk_flag.append("BEGIN{FS=\"/\"}"); // get netmask prefix
				snprintf(result_buffer, 128, nmcli_command.c_str(), field.c_str(), network.iface.c_str(), awk_flag.c_str());
				if (!network.isIPv6)
					network.netmask = calculateIp4Netmask( getShOutput( result_buffer ) );
				else
					network.netmask = getShOutput( result_buffer );

				field.clear();
				field.append( ip_version ).append(".gateway"); // gateway
				snprintf(result_buffer, 128, nmcli_command.c_str(), field.c_str(), network.iface.c_str(), "");
				network.gateway = getShOutput( result_buffer );

				field.clear();
				field.append( "GENERAL.HWADDR" ); // mac address
				snprintf(result_buffer, 128, nmcli_command.c_str(), field.c_str(), network.iface.c_str(), "");
				network.mac = getShOutput( result_buffer );

				network.dns1 = queryDnsOne();
				network.dns2 = queryDnsTwo();

				nmcli_command.clear();
				if (network.isWifi)
				{
					nmcli_command.append("nmcli dev wifi | grep '%s' | sed -e 's/^.*%s//' | awk '{print %s}'");
					field.clear();
					field.append( "$5" ) // wifi signal
							 .append( " \" \" $2" ) // wifi channel
							 .append( " \" \"  $3" ) // rate
							 .append( " \" \"  $4" ) // rate unit
							 .append( " \" \"  $7 \" \"  $8 " ); // wifi security
					snprintf(result_buffer, 256, nmcli_command.c_str(), network.ssid.c_str(), network.ssid.c_str(), field.c_str());
					std::vector<std::string> results = Utils::String::split(getShOutput( result_buffer ), ' ');
					network.signal = std::atoi( results.at(0).c_str() ); // wifi signal
					network.channel = std::atoi( results.at(1).c_str() ); // wifi channel
					network.rate = std::atoi( results.at(2).c_str() ); // rate
					network.rate_unit = results.at(3);
					network.security = results.at(4); // wifi security
					if ((results.size() == 6) && !results.at(5).empty())
						network.security.append(" - ").append(results.at(5));
				}
				else
				{
					nmcli_command.append("nmcli -f CAPABILITIES.SPEED dev show %s | awk '{print %s}'");
					field.clear();
					field.append( "$2" ) // rate
							 .append( " \" \" $3" ); // rate unit
					snprintf(result_buffer, 128, nmcli_command.c_str(), network.iface.c_str(), field.c_str());
					std::vector<std::string> results = Utils::String::split(getShOutput( result_buffer ), ' ');
					network.rate = std::atoi( results.at(0).c_str() ); // rate
					network.rate_unit = results.at(1);
				}
			}
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryNetworkInformation() - Error reading network data!!!";
	}

	return network;
}

std::string queryIPAddress()
{
	return queryNetworkInformation(true).ip_address;
}

bool queryNetworkConnected()
{
	return executeSystemScript("es-wifi is_connected");
}

bool queryWifiEnabled()
{
	return executeSystemScript("es-wifi is_enabled");
}

std::string queryWifiSsid()
{
	return getShOutput("es-wifi get_ssid");
}

std::string queryWifiPsk(std::string ssid)
{
	if (ssid.empty())
		return "";

	return getShOutput("es-wifi get_ssid_psk \"" + ssid + '"');
}

std::string queryDnsOne()
{
	if (!queryNetworkConnected())
		return "";

	return getShOutput("es-wifi get_dns1");
}

std::string queryDnsTwo()
{
	if (!queryNetworkConnected())
		return "";

	return getShOutput("es-wifi get_dns2");
}

std::string queryWifiNetworkExistFlag()
{
	return getShOutput("es-wifi get_network_exist_flag");
}

bool queryNetworkConnectedFast()
{
 bool result = false;
	// Wifi
	int fd;
	char buffer[10];
	fd = open("/sys/class/net/wlan0/operstate", O_RDONLY);
	if (fd > 0)
	{
		memset(buffer, 0, 10);
		ssize_t count = read(fd, buffer, 10);
		if( count > 0 )
		{
			if( strstr( buffer, "up") != NULL )
				result = true;
			else
				result = false;
		}
		close(fd);
	}
	return result;
}

std::string querySocName() {
	return getShOutput("es-system_inf get_soc_name");
}

CpuAndSocketInformation queryCpuAndChipsetInformation(bool summary)
{
	CpuAndSocketInformation chipset;

	try
	{
		chipset.cpu_load = queryLoadCpu();
		chipset.temperature = queryTemperatureCpu();

		if (!summary)
		{
			if (Utils::FileSystem::exists("/usr/local/bin/es-system_inf"))
			{
				chipset.soc_name = querySocName();
			}
			if (Utils::FileSystem::exists("/usr/bin/lscpu"))
			{
				chipset.vendor_id = getShOutput(R"(lscpu | egrep 'Vendor ID' | awk '{print $3}')");
				chipset.model_name = getShOutput(R"(lscpu | egrep 'Model name' | awk '{print $3}')");
				chipset.ncpus = std::atoi( getShOutput(R"(lscpu | egrep 'CPU\(s\)' | awk '{print $2}' | grep -v CPU)").c_str() );
				chipset.architecture = getShOutput(R"(lscpu | egrep 'Architecture' | awk '{print $2}')");
				chipset.nthreads_core = std::atoi( getShOutput(R"(lscpu | egrep 'Thread' | awk '{print $4}')").c_str() );
			}
			if (Utils::FileSystem::exists("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor"))
			{
				chipset.governor = Utils::String::replace ( getShOutput(R"(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_governor)"), "_" , " ");
			}
			chipset.frequency = queryFrequencyCpu(); // MHZ
			if (Utils::FileSystem::exists("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq"))
			{
				chipset.frequency_max = std::atoi( getShOutput(R"(sudo cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq)").c_str() );
				chipset.frequency_max = chipset.frequency_max / 1000; // MHZ
			}
			if (Utils::FileSystem::exists("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq"))
			{
				chipset.frequency_min = std::atoi( getShOutput(R"(sudo cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq)").c_str() );
				chipset.frequency_min = chipset.frequency_min / 1000; // MHZ
			}
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryCpuAndChipsetInformation() - Error reading chipset data!!!";
	}

	return chipset;
}

float queryLoadCpu()
{
	try
	{
		if (Utils::FileSystem::exists("/usr/bin/top"))
		{
			float cpu_iddle = std::atof( getShOutput(R"(top -b -n 1 | egrep '%Cpu' | awk '{print $8}')").c_str() );
			return 100.0f - cpu_iddle;
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryLoadCpu() - Error reading load CPU data!!!";
	}
	return 0.f;
}

float queryTemperatureCpu()
{
	try
	{
		if (Utils::FileSystem::exists("/sys/devices/virtual/thermal/thermal_zone0/temp"))
		{
			float temperature = std::atof( getShOutput(R"(cat /sys/devices/virtual/thermal/thermal_zone0/temp)").c_str() );
			return temperature / 1000;
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryTemperatureCpu() - Error reading CPU temperature data!!!";
	}
	return 0.f;
}

int queryFrequencyCpu()
{
	try
	{
		if (Utils::FileSystem::exists("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq"))
		{
			int frequency = std::atoi( getShOutput(R"(sudo cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq)").c_str() );
			return frequency / 1000; // MHZ
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryFrequencyCpu() - Error reading CPU frequency data!!!";
	}
	return 0;
}

RamMemoryInformation queryRamMemoryInformation(bool summary)
{
	RamMemoryInformation memory;
	try
	{
		if (Utils::FileSystem::exists("/usr/bin/top"))
		{
			memory.total = std::atof( getShOutput(R"(top -b -n 1 | egrep 'MiB Mem' | awk '{print $4}')").c_str() );
			memory.free = std::atof( getShOutput(R"(top -b -n 1 | egrep 'MiB Mem' | awk '{print $6}')").c_str() );
			if (!summary)
			{
				memory.used = std::atof( getShOutput(R"(top -b -n 1 | egrep 'MiB Mem' | awk '{print $8}')").c_str() );
				memory.cached = std::atof( getShOutput(R"(top -b -n 1 | egrep 'MiB Mem' | awk '{print $10}')").c_str() );
			}
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryRamMemoryInformation() - Error reading memory data!!!";
	}
	return memory;
}

DisplayAndGpuInformation queryDisplayAndGpuInformation(bool summary)
{
	DisplayAndGpuInformation data;
	try
	{
		data.temperature = queryTemperatureGpu();
		data.brightness_level = queryBrightnessLevel();

		if (!summary)
		{
			if (Utils::FileSystem::exists("/sys/devices/platform/fde60000.gpu/gpuinfo"))
				data.gpu_model = getShOutput(R"(cat /sys/devices/platform/fde60000.gpu/gpuinfo | awk '{print $1}')");

			if (Utils::FileSystem::exists("/sys/devices/platform/display-subsystem/drm/card0/card0-DSI-1/modes"))
				data.resolution = getShOutput(R"(cat /sys/devices/platform/display-subsystem/drm/card0/card0-DSI-1/modes)");

			if (Utils::FileSystem::exists("/sys/devices/platform/display-subsystem/graphics/fb0/bits_per_pixel"))
				data.bits_per_pixel = std::atoi( getShOutput(R"(cat /sys/devices/platform/display-subsystem/graphics/fb0/bits_per_pixel)").c_str() );

			if (Utils::FileSystem::exists("/sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/governor"))
				data.governor = Utils::String::replace ( getShOutput(R"(cat /sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/governor)"), "_" , " ");

			data.frequency = queryFrequencyGpu();

			if (Utils::FileSystem::exists("/sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/max_freq"))
			{
				data.frequency_max = std::atoi( getShOutput(R"(cat /sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/max_freq)").c_str() );
				data.frequency_max = data.frequency_max / 1000000; // MHZ
			}

			if (Utils::FileSystem::exists("/sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/min_freq"))
			{
				data.frequency_min = std::atoi( getShOutput(R"(cat /sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/min_freq)").c_str() );
				data.frequency_min = data.frequency_min / 1000000; // MHZ
			}

			data.brightness_system = queryBrightness();
			data.brightness_system_max = queryMaxBrightness();
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryDisplayAndGpuInformation() - Error reading display and GPU data!!!";
	}
	return data;
}

float queryTemperatureGpu()
{
	try
	{
		if (Utils::FileSystem::exists("/sys/devices/virtual/thermal/thermal_zone1/temp"))
		{
			float temperature = std::atof( getShOutput(R"(cat /sys/devices/virtual/thermal/thermal_zone1/temp)").c_str() );
			return temperature / 1000;
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryTemperatureGpu() - Error reading GPU temperature data!!!";
	}
	return 0.f;
}

int queryFrequencyGpu()
{
	try
	{
		if (Utils::FileSystem::exists("/sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/cur_freq"))
		{
			int frequency = std::atoi( getShOutput(R"(cat /sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu/cur_freq)").c_str() );
			return frequency / 1000000; // MHZ
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryFrequencyGpu() - Error reading GPU frequency data!!!";
	}
	return 0;
}

const char* BACKLIGHT_BRIGHTNESS_NAME = "/sys/class/backlight/backlight/brightness";
const char* BACKLIGHT_BRIGHTNESS_MAX_NAME = "/sys/class/backlight/backlight/max_brightness";
#define BACKLIGHT_BUFFER_SIZE 127

int queryBrightness()
{
	if (Utils::FileSystem::exists("/usr/bin/brightnessctl"))
		return std::atoi( getShOutput(R"(/usr/bin/brightnessctl g)").c_str() );
	else if (Utils::FileSystem::exists("/sys/devices/platform/backlight/backlight/backlight/actual_brightness"))
		return std::atoi( getShOutput(R"(cat /sys/devices/platform/backlight/backlight/backlight/actual_brightness)").c_str() );

	return 0;
}

int queryMaxBrightness()
{
	if (Utils::FileSystem::exists("/usr/bin/brightnessctl"))
		return std::atoi( getShOutput(R"(/usr/bin/brightnessctl m)").c_str() );
	else if (Utils::FileSystem::exists(BACKLIGHT_BRIGHTNESS_MAX_NAME))
		return std::atoi( getShOutput("cat " + *BACKLIGHT_BRIGHTNESS_MAX_NAME).c_str() );

	return 0;
}

int queryBrightnessLevel()
{
	if (Utils::FileSystem::exists("/usr/bin/brightnessctl"))
		return std::atoi( getShOutput(R"(brightnessctl -m | awk -F',|%' '{print $4}')").c_str() );

	int value,
			fd,
			max = 255;
	char buffer[BACKLIGHT_BUFFER_SIZE + 1];
	ssize_t count;

	fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME, O_RDONLY);
	if (fd < 0)
		return false;

	memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

	count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
	if (count > 0)
		max = atoi(buffer);

	close(fd);

	if (max == 0)
		return 0;

	fd = open(BACKLIGHT_BRIGHTNESS_NAME, O_RDONLY);
	if (fd < 0)
		return false;

	memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

	count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
	if (count > 0)
		value = atoi(buffer);

	close(fd);

	return (uint32_t) ((value / (float)max * 100.0f) + 0.5f);
}

void saveBrightnessLevel(int brightness_level)
{
	bool setted = false;
	if (Utils::FileSystem::exists("/usr/bin/brightnessctl"))
		setted = executeSystemScript("/usr/bin/brightnessctl s " + std::to_string(brightness_level) + "% &");

	if (!setted)
	{
		if (brightness_level < 5)
			brightness_level = 5;

		if (brightness_level > 100)
			brightness_level = 100;

		int fd,
				max = 255;
		char buffer[BACKLIGHT_BUFFER_SIZE + 1];
		ssize_t count;

		fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME, O_RDONLY);
		if (fd < 0)
			return;

		memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

		count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
		if (count > 0)
			max = atoi(buffer);

		close(fd);

		if (max == 0)
			return;

		fd = open(BACKLIGHT_BRIGHTNESS_NAME, O_WRONLY);
		if (fd < 0)
			return;

		float percent = (brightness_level / 100.0f * (float)max) + 0.5f;
		sprintf(buffer, "%d\n", (uint32_t)percent);

		count = write(fd, buffer, strlen(buffer));
		if (count < 0)
			LOG(LogError) << "Platform::saveBrightnessLevel failed";
		else
			setted = true;

		close(fd);
	}
}

std::string queryHostname()
{
	std::string hostname = "";

	hostname.append( getShOutput(R"(hostname)") );
	if ( hostname.empty() && Utils::FileSystem::exists("/usr/bin/hostnamectl") )
		hostname.append( getShOutput(R"(hostnamectl | grep -iw hostname | awk '{print $3}')") );

	return hostname;
}

bool setCurrentHostname(std::string hostname)
{
	return executeSystemScript("sudo hostnamectl set-hostname \"" + hostname + "\" &");
}

SoftwareInformation querySoftwareInformation(bool summary)
{
	SoftwareInformation si;

	si.application_name = getShOutput("es-system_inf get_system_name");
	si.version = getShOutput("es-system_inf get_system_version");
	si.hostname = queryHostname();

	if (!summary)
	{
		si.so_base = getShOutput("es-system_inf get_base_os_info");
		si.linux = getShOutput("es-system_inf get_kernel_info");
	}
	return si;
}

std::string queryDeviceName() {
	return getShOutput("es-system_inf get_device_name");
}

DeviceInformation queryDeviceInformation(bool summary)
{
	DeviceInformation di;
	try
	{
		di.name = queryDeviceName();
		if ( Utils::FileSystem::exists("/proc/cpuinfo") )
		{
			di.hardware = getShOutput(R"(cat /proc/cpuinfo | grep -iw hardware | awk '{print $3 " " $4}')");
			di.revision = getShOutput(R"(cat /proc/cpuinfo | grep Revision | awk '{print $3 " " $4}')");
			di.serial = Utils::String::toUpper( getShOutput(R"(cat /proc/cpuinfo | grep -iw serial | awk '{print $3 " " $4}')") );
		}
		if ( Utils::FileSystem::exists("/usr/bin/hostnamectl") )
		{
			di.machine_id = Utils::String::toUpper( getShOutput(R"(hostnamectl | grep -iw machine | awk '{print $3}')") );
			di.boot_id = Utils::String::toUpper( getShOutput(R"(hostnamectl | grep -iw boot | awk '{print $3}')") );
		}
	} catch (...) {
		LOG(LogError) << "Platform::queryDeviceInformation() - Error reading device data!!!";
	}
	return di;
}

// Adapted from emuelec
std::string getShOutput(const std::string& mStr)
{
	std::string result, file;
	FILE* pipe{popen(mStr.c_str(), "r")};
	char buffer[256];

	while(fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		file = buffer;
		result += file.substr(0, file.size() - 1);
	}

	pclose(pipe);
	return result;
}

bool isDriveMounted(std::string device)
{
	return ( Utils::FileSystem::exists(device) && Utils::FileSystem::exists("/bin/findmnt")
		&& !getShOutput("findmnt -rno SOURCE,TARGET \"" + device + '"').empty() );
}

std::string queryDriveMountPoint(std::string device)
{
	std::string dev = "/dev/" + device;
	if ( Utils::FileSystem::exists(dev) && Utils::FileSystem::exists("/bin/lsblk") )
		return getShOutput("lsblk -no MOUNTPOINT " + dev);

	return "";
}

std::vector<std::string> queryUsbDriveMountPoints()
{
	std::vector<std::string> partitions = executeSystemEnumerationScript(R"(cat /proc/partitions | egrep sda. | awk '{print $4}')"),
													 mount_points;
	for (auto partition = begin (partitions); partition != end (partitions); ++partition)
	{
		std::string mp = queryDriveMountPoint(*partition);
		if (!mp.empty())
			mount_points.push_back(mp);
	}

	return mount_points;
}

std::string queryTimezones()
{
	if (Utils::FileSystem::exists("/usr/local/bin/timezones"))
		return getShOutput(R"(/usr/local/bin/timezones available)");
	else if (Utils::FileSystem::exists("/usr/bin/timedatectl"))
		return getShOutput(R"(/usr/bin/timedatectl list-timezones | sort -u | tr '\n' ',')");
	else if (Utils::FileSystem::exists("/usr/share/zoneinfo/zone1970.tab"))
		return getShOutput(R"(cat /usr/share/zoneinfo/zone1970.tab | grep -v "^#" | awk '{ print $3 }' | sort -u | tr '\n' ',')");

	return "Europe/Paris";
}

std::string queryCurrentTimezone()
{
	if (Utils::FileSystem::exists("/usr/local/bin/timezones"))
		return getShOutput(R"(/usr/local/bin/timezones current)");
	else if (Utils::FileSystem::exists("/usr/bin/timedatectl"))
		return getShOutput(R"(/usr/bin/timedatectl | grep -iw \"Time zone\" | awk '{print $3}')");
	else  if (Utils::FileSystem::exists("/bin/readlink"))
		return getShOutput(R"(readlink -f /etc/localtime | sed 's;/usr/share/zoneinfo/;;')");

	return "Europe/Paris";
}

bool setCurrentTimezone(std::string timezone)
{
	if (timezone.empty())
		return false;

	if (Utils::FileSystem::exists("/usr/local/bin/timezones"))
		return executeSystemScript("/usr/local/bin/timezones set \"" + timezone + "\" &");
	else if (Utils::FileSystem::exists("/usr/bin/timedatectl"))
		return executeSystemScript("/usr/bin/sudo timedatectl set-timezone \"" + timezone + "\" &");

	return executeSystemScript("sudo ln -sf \"/usr/share/zoneinfo/" + timezone +"\" /etc/localtime &");
}

RemoteServiceInformation queryRemoteServiceStatus(const std::string &name)
{
	RemoteServiceInformation rsi;

	rsi.name = name;
	if (name == "SAMBA")
		rsi.platformName = "smbd.service";
	else if (name == "NETBIOS")
		rsi.platformName = "nmbd.service";
	else if (name == "NTP")
		rsi.platformName = "ntp";
	else if (name == "FILE-BROWSER")
		rsi.platformName = "filebrowser";
	else
		rsi.platformName =  Utils::String::toLower(name) + ".service";

	std::string result = getShOutput("es-remote_services get_status \"" + rsi.platformName + '"');
	std::vector<std::string> data = Utils::String::split(result, ';');

	if (data.size() == 2)
	{
		rsi.isStartOnBoot = stringToState(data[0]);
		rsi.isActive = stringToState(data[1], "active");
	}

	return rsi;
}

bool setRemoteServiceStatus(RemoteServiceInformation service)
{
	return executeSystemScript("es-remote_services set_status \"" + service.platformName + "\" " + stateToString(service.isStartOnBoot ) + ' ' + stateToString(service.isActive, "active", "inactive"));
}

std::string stateToString(bool state, const std::string &active_value, const std::string &not_active_value)
{
	return state ? active_value : not_active_value;
}

bool stringToState(const std::string state, const std::string &active_value)
{
	return ( Utils::String::replace(state, "\n", "") == active_value );
}

#ifdef _DEBUG
uint32_t getVolume()
{
	uint32_t value = 0;
	if (Utils::FileSystem::exists("/usr/local/bin/current_volume"))
		value = std::atoi( getShOutput(R"(/usr/local/bin/current_volume)").c_str() );
	else
		value = std::atoi( getShOutput(R"(awk -F'[][]' '/Left:/ { print $2 }' <(amixer sget Playback))").c_str() );
	return value;
}
#endif

std::string queryBluetoothEnabled()
{
	return getShOutput(R"(if [ -z $(pidof rtk_hciattach) ]; then echo "disabled"; else echo "enabled"; fi)");
}

