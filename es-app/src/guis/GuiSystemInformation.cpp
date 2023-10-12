#include "guis/GuiSystemInformation.h"

#include "components/MenuComponent.h"
#include "guis/GuiSettings.h"
#include "guis/UpdatableGuiSettings.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "Window.h"
#include "ApiSystem.h"
#include "Log.h"
#include "components/UpdatableTextComponent.h"
#include "ApiSystem.h"
#include "renderers/Renderer.h"
#include "SystemConf.h"


GuiSystemInformation::GuiSystemInformation(Window* window) : UpdatableGuiSettings(window, _("SYSTEM INFORMATION").c_str())
{
	initializeMenu();
}

GuiSystemInformation::~GuiSystemInformation()
{

}

void GuiSystemInformation::initializeMenu()
{
	if (Settings::getInstance()->getBool("ShowDetailedSystemInfo"))
		showDetailedSystemInfo();
	else
		showSummarySystemInfo();
}

void GuiSystemInformation::showSummarySystemInfo()
{
	LOG(LogInfo) << "GuiSystemInformation::showSummarySystemInfo()";
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	UpdatableGuiSettings *pthis = this;

	SystemSummaryInfo summary = ApiSystem::getInstance()->getSummaryInformation();
	RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation();

	// device name
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SYSTEM_INFORMATION))
		addWithLabel(_("DEVICE"), std::make_shared<TextComponent>(window, summary.device, font, color));

	addGroup(_("CPU"));
	// CPU load
	bool warning = isCpuLoadLimit(summary.cpu_load);
	auto loadCpu = std::make_shared<UpdatableTextComponent>(window, formatLoadCpu( summary.cpu_load ), font, warning ? 0xFF0000FF : color);
	loadCpu->setUpdatableFunction([loadCpu, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update load CPU";
			float load_cpu_value = ApiSystem::getInstance()->getLoadCpu();
			bool warning = isCpuLoadLimit(load_cpu_value);
			loadCpu->setText(formatLoadCpu( load_cpu_value ));
			loadCpu->setColor(warning ? 0xFF0000FF : color);
		}, 4000);
	addUpdatableComponent(loadCpu.get());
	addWithLabel(_("CPU LOAD"), loadCpu);

	// CPU temperature
	warning = isTemperatureLimit(summary.cpu_temperature);
	auto temperatureCpu = std::make_shared<UpdatableTextComponent>(window, formatTemperature( summary.cpu_temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperatureCpu->setUpdatableFunction([temperatureCpu, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture CPU";
			float temp_cpu_value = ApiSystem::getInstance()->getTemperatureCpu();
			bool warning = isTemperatureLimit(temp_cpu_value);
			temperatureCpu->setText(formatTemperature( temp_cpu_value, warning ));
			temperatureCpu->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	addUpdatableComponent(temperatureCpu.get());
	addWithLabel(_("TEMPERATURE"), temperatureCpu);

	addGroup(_("OTHER INFORMATION"));
	// GPU temperature
	warning = isTemperatureLimit( summary.gpu_temperature );
	auto temperature_gpu = std::make_shared<UpdatableTextComponent>(window, formatTemperature( summary.gpu_temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature_gpu->setUpdatableFunction([temperature_gpu, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture GPU";
			float temp_gpu_value = ApiSystem::getInstance()->getTemperatureGpu();
			bool warning = isTemperatureLimit( temp_gpu_value );
			temperature_gpu->setText(formatTemperature( temp_gpu_value, warning ));
			temperature_gpu->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	addUpdatableComponent(temperature_gpu.get());
	addWithLabel(_("GPU") + " - " + _("TEMPERATURE"), temperature_gpu);

	// battery temperature
	warning = isTemperatureLimit( summary.batt_temperature );
	auto temperature_battery = std::make_shared<UpdatableTextComponent>(window, formatTemperature( summary.batt_temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature_battery->setUpdatableFunction([temperature_battery, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture Battery";
			float temp_bat_value = ApiSystem::getInstance()->getTemperatureBattery();
			bool warning = isTemperatureLimit( temp_bat_value );
			temperature_battery->setText(formatTemperature( temp_bat_value, warning ));
			temperature_battery->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	addUpdatableComponent(temperature_battery.get());
	addWithLabel(_("BATTERY") + " - " + _("TEMPERATURE"), temperature_battery);

	// free ram
	warning = isMemoryLimit( memory.total, memory.free );
	auto memoryFree = std::make_shared<UpdatableTextComponent>(window, formatMemory( memory.free, memory.total, true ), font, warning ? 0xFF0000FF : color);
	memoryFree->setUpdatableFunction([memoryFree, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update memory free";
			RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation();
			bool warning = isMemoryLimit( memory.total, memory.free );
			memoryFree->setText(formatMemory( memory.free, memory.total, true ));
			memoryFree->setColor(warning ? 0xFF0000FF : color);
		}, 10000);
	addUpdatableComponent(memoryFree.get());
	addWithLabel(_("FREE RAM MEMORY"), memoryFree);

	addGroup(_("STORAGE"));
	std::vector<StorageDevice> storageDevices = ApiSystem::getInstance()->getStorageDevices(false);

	for (StorageDevice storageDevice : storageDevices)
	{
		bool warning = isStorageLimit(storageDevice.used_percent);
		auto sd_component = std::make_shared<TextComponent>(window, formatStorage(storageDevice.used, storageDevice.avail, storageDevice.used_percent),
															font, warning ? 0xFF0000FF : color);
		std::string label(Utils::String::toUpper(storageDevice.name));
		label.append(" ").append(_("DISK USAGE"));
		addWithLabel(label, sd_component);
	}

	addGroup(_("NETWORK"));

	// connected to network
	auto networkStatus = std::make_shared<UpdatableTextComponent>(window, formatNetworkStatus( summary.wifi_connected ), font, color);

	// acces to internet
	auto internetStatus = std::make_shared<TextComponent>(window, formatNetworkStatus( summary.internet_status ), font, color);

	// Wifi ssid
	auto wifiSsid = std::make_shared<TextComponent>(window, summary.wifi_ssid, font, color);
	// IP address
	auto ipAddress = std::make_shared<TextComponent>(window, summary.ip_address, font, color);
	networkStatus->setUpdatableFunction([networkStatus, internetStatus, wifiSsid, ipAddress, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update network status";
			NetworkInformation ni = ApiSystem::getInstance()->getNetworkInformation();
			networkStatus->setText(formatNetworkStatus( ni.isConnected ));
			internetStatus->setText(formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ));
			if (ni.isConnected)
			{
				wifiSsid->setText(ni.ssid);
				ipAddress->setText(ni.ip_address);
			}
			else
			{
				wifiSsid->setText("");
				ipAddress->setText("");
			}
		}, 5000);
	addUpdatableComponent(networkStatus.get());
	addWithLabel(_("STATUS"), networkStatus);
	addWithLabel(_("INTERNET STATUS"), internetStatus);
	addWithLabel(_("WIFI SSID"), wifiSsid);
	addWithLabel(_("IP ADDRESS"), ipAddress);
}

void GuiSystemInformation::showDetailedSystemInfo()
{
	LOG(LogInfo) << "GuiSystemInformation::showDetailedSystemInfo()";

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SYSTEM_INFORMATION))
	{
		addSubMenu(_("CPU AND SOCKET"), [this]() { openCpuAndSocket(); });

		addSubMenu(_("RAM MEMORY"), [this]() { openRamMemory(); });
		addSubMenu(_("DISPLAY AND GPU"), [this]() { openDisplayAndGpu(); });
		addSubMenu(_("STORAGE"), [this]() { openStorage(); });
	}
	addSubMenu(_("NETWORK"), [this]() { openNetwork(); });

	BatteryInformation bi = ApiSystem::getInstance()->getBatteryInformation(false);
	//LOG(LogDebug) << "GuiSystemInformation::showDetailedSystemInfo() - has battery: " << Utils::String::boolToString(bi.hasBattery);
	if (bi.hasBattery)
		addSubMenu(_("BATTERY"), [this, bi]() { openBattery(&bi); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SYSTEM_INFORMATION))
	{
		addSubMenu(_("SOFTWARE"), [this]() { openSoftware(); });
		addSubMenu(_("DEVICE"), [this]() { openDevice(); });
	}
}

void GuiSystemInformation::openCpuAndSocket()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("CPU AND SOCKET"));

	CpuAndSocketInformation csi = ApiSystem::getInstance()->getCpuAndChipsetInformation();

	// SOC name
	s->addWithLabel(_("SOC"), std::make_shared<TextComponent>(window, csi.soc_name,font, color));

	// vendor ID
	s->addWithLabel(_("VENDOR ID"), std::make_shared<TextComponent>(window, csi.vendor_id,font, color));

	// model
	s->addWithLabel(_("MODEL NAME"), std::make_shared<TextComponent>(window, csi.model_name, font, color));

	// CPU(s)
	s->addWithLabel(_("Nº CPU"), std::make_shared<TextComponent>(window, std::to_string( csi.ncpus ), font, color));

	// architecture
	s->addWithLabel(_("ARCHITECTURE"), std::make_shared<TextComponent>(window, Utils::String::toUpper( csi.architecture ), font, color));

	// threads per core
	s->addWithLabel(_("THREAD(S) PER CORE"), std::make_shared<TextComponent>(window, std::to_string( csi.nthreads_core ), font, color));

	// CPU load
	bool warning = isCpuLoadLimit( csi.cpu_load );
	auto load = std::make_shared<UpdatableTextComponent>(window, formatLoadCpu( csi.cpu_load ), font, warning ? 0xFF0000FF : color);
	load->setUpdatableFunction([load, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update load CPU";
			float load_cpu_value = ApiSystem::getInstance()->getLoadCpu();
			bool warning = isCpuLoadLimit(load_cpu_value);
			load->setText(formatLoadCpu( load_cpu_value ));
			load->setColor(warning ? 0xFF0000FF : color);
		}, 2000);
	s->addWithLabel(_("CPU LOAD"), load);
	s->addUpdatableComponent(load.get());

	// temperature
	warning = isTemperatureLimit( csi.temperature );
	auto temperature = std::make_shared<UpdatableTextComponent>(window, formatTemperature( csi.temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature->setUpdatableFunction([temperature, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture CPU";
			float temp_cpu_value = ApiSystem::getInstance()->getTemperatureCpu();
			bool warning = isTemperatureLimit( temp_cpu_value );
			temperature->setText(formatTemperature( temp_cpu_value, warning ));
			temperature->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	s->addWithLabel(_("TEMPERATURE"), temperature);
	s->addUpdatableComponent(temperature.get());

	// governor
	s->addWithLabel(_("GOVERNOR"), std::make_shared<TextComponent>(window, _( Utils::String::toUpper( csi.governor ) ), font, color));

	// frequency
	auto frequency = std::make_shared<UpdatableTextComponent>(window, formatFrequency( csi.frequency ), font, color);
	frequency->setUpdatableFunction([frequency]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update frequency CPU";
			frequency->setText(formatFrequency( ApiSystem::getInstance()->getFrequencyCpu() ));
		}, 5000);
	s->addWithLabel(_("FREQUENCY"), frequency);
	s->addUpdatableComponent(frequency.get());

	// maximus frequency
	s->addWithLabel(_("FREQUENCY MAX"), std::make_shared<TextComponent>(window, formatFrequency( csi.frequency_max ), font, color));

	// minimus frequency
	s->addWithLabel(_("FREQUENCY MIN"), std::make_shared<TextComponent>(window, formatFrequency( csi.frequency_min ), font, color));


	window->pushGui(s);
}

void GuiSystemInformation::openRamMemory()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("RAM MEMORY"));

	RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation();

	// total ram
	s->addWithLabel(_("TOTAL"), std::make_shared<TextComponent>(window, formatMemory( memory.total ), font, color));

	// free ram
	bool warning = isMemoryLimit( memory.total, memory.free );
	auto memoryFree = std::make_shared<UpdatableTextComponent>(window, formatMemory( memory.free, memory.total, true ), font, warning ? 0xFF0000FF : color);
	auto memoryUsed = std::make_shared<TextComponent>(window, formatMemory( memory.used, memory.total, true ), font, warning ? 0xFF0000FF : color);
	auto memoryCached = std::make_shared<TextComponent>(window, formatMemory( memory.cached, memory.total, true ), font, warning ? 0xFF0000FF : color);

	memoryFree->setUpdatableFunction([memoryFree, memoryUsed, memoryCached, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update memory free";
			RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation();
			bool warning = isMemoryLimit( memory.total, memory.free );
			memoryFree->setText(formatMemory( memory.free, memory.total, true ));
			memoryFree->setColor(warning ? 0xFF0000FF : color);
			memoryUsed->setText(formatMemory( memory.used, memory.total, true ));
			memoryCached->setText(formatMemory( memory.cached, memory.total, true ));
		}, 5000);
	s->addUpdatableComponent(memoryFree.get());
	s->addWithLabel(_("FREE"), memoryFree);
	s->addWithLabel(_("USED"), memoryUsed);
	s->addWithLabel(_("BUFFERED / CACHED"), memoryCached);

	window->pushGui(s);
}

void GuiSystemInformation::openDisplayAndGpu()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("DISPLAY AND GPU"));

	DisplayAndGpuInformation di = ApiSystem::getInstance()->getDisplayAndGpuInformation();

	// GPU model
	s->addWithLabel(_("GPU"), std::make_shared<TextComponent>(window, di.gpu_model, font, color));

	auto info = Renderer::getDriverInformation();
	if (!info.empty())
	{
		// graphics API
		s->addWithLabel(_("GRAPHICS API"), std::make_shared<TextComponent>(window, info.at(0).second, font, color));
	}

	// resolution
	s->addWithLabel(_("RESOLUTION"), std::make_shared<TextComponent>(window, di.resolution, font, color));

	// bits per pixel
	s->addWithLabel(_("BITS PER PIXEL"), std::make_shared<TextComponent>(window, std::to_string( di.bits_per_pixel ), font, color));

	s->addWithLabel(_("HDMI MODE"), std::make_shared<TextComponent>(window, ( SystemConf::getInstance()->getBool("hdmi.mode") ? _("YES") : _("NO")), font, color));

	// temperature
	bool warning = isTemperatureLimit( di.temperature );
	auto temperature = std::make_shared<UpdatableTextComponent>(window, formatTemperature( di.temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature->setUpdatableFunction([temperature, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture GPU";
			float temp_gpu_value = ApiSystem::getInstance()->getTemperatureGpu();
			bool warning = isTemperatureLimit( temp_gpu_value );
			temperature->setText(formatTemperature( temp_gpu_value, warning ));
			temperature->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	s->addUpdatableComponent(temperature.get());
	s->addWithLabel(_("TEMPERATURE"), temperature);

	// governor
	s->addWithLabel(_("GOVERNOR"), std::make_shared<TextComponent>(window, _( Utils::String::toUpper( di.governor ) ), font, color));

	// frequency
	auto frequency = std::make_shared<UpdatableTextComponent>(window, formatFrequency( di.frequency ), font, color);
	frequency->setUpdatableFunction([frequency]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture GPU";
			frequency->setText(formatFrequency( ApiSystem::getInstance()->getFrequencyGpu() ));
		}, 5000);
	s->addUpdatableComponent(frequency.get());
	s->addWithLabel(_("FREQUENCY"), frequency);

	// maximus frequency
	s->addWithLabel(_("FREQUENCY MAX"), std::make_shared<TextComponent>(window, formatFrequency( di.frequency_max ), font, color));

	// minimus frequency
	s->addWithLabel(_("FREQUENCY MIN"), std::make_shared<TextComponent>(window, formatFrequency( di.frequency_min ), font, color));

	if (!SystemConf::getInstance()->getBool("hdmi.mode"))
	{
		// brightness %
		auto brightness = std::make_shared<UpdatableTextComponent>(window, formatPercent( di.brightness_level ), font, color);
		brightness->setUpdatableFunction([brightness]
			{
				//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update brightness display";
				brightness->setText(formatPercent( ApiSystem::getInstance()->getBrightnessLevel() ));
			}, 5000);
		s->addUpdatableComponent(brightness.get());
		s->addWithLabel(_("BRIGHTNESS"), brightness);

		// Display panel values
		s->addWithLabel(_("GAMMA"), std::make_shared<TextComponent>(window, formatPercent( di.gamma_level ), font, color));
		s->addWithLabel(_("CONTRAST"), std::make_shared<TextComponent>(window, formatPercent( di.contrast_level ), font, color));
		s->addWithLabel(_("SATURATION"), std::make_shared<TextComponent>(window, formatPercent( di.saturation_level ), font, color));
		s->addWithLabel(_("HUE"), std::make_shared<TextComponent>(window, formatPercent( di.hue_level ), font, color));
	}
	
	window->pushGui(s);
}

void GuiSystemInformation::openStorage()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new GuiSettings(window, _("STORAGE"));

	std::vector<StorageDevice> storageDevices = ApiSystem::getInstance()->getStorageDevices();

	for (StorageDevice storageDevice : storageDevices)
	{
		bool warning = isStorageLimit(storageDevice.used_percent);
		auto sd_component = std::make_shared<TextComponent>(window, formatStorage(storageDevice.used, storageDevice.avail, storageDevice.used_percent),
															font, warning ? 0xFF0000FF : color);
		std::string label(Utils::String::toUpper(storageDevice.name));
		label.append(" ").append(_("DISK USAGE"));
		s->addWithLabel(label, sd_component);
	}

	window->pushGui(s);
}

void GuiSystemInformation::openNetwork()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("NETWORK"));

	NetworkInformation ni = ApiSystem::getInstance()->getNetworkInformation(false);

// connected to network
	auto status = std::make_shared<UpdatableTextComponent>(window, formatNetworkStatus( ni.isConnected ), font, color);
	auto internetStatus = std::make_shared<TextComponent>(window, formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ), font, color);
	auto isWifi = std::make_shared<TextComponent>(window, _( Utils::String::boolToString(ni.isWifi, true) ), font, color);
	auto address = std::make_shared<TextComponent>(window, ni.ip_address, font, color);
	auto netmask = std::make_shared<TextComponent>(window, ni.netmask, font, color);
	auto gateway = std::make_shared<TextComponent>(window, ni.gateway, font, color);
	auto mac = std::make_shared<TextComponent>(window, ni.mac, font, color);
	auto dns1 = std::make_shared<TextComponent>(window, ni.dns1, font, color);
	auto dns2 = std::make_shared<TextComponent>(window, ni.dns2, font, color);
	auto rate = std::make_shared<TextComponent>(window, formatNetworkRate(ni.rate, ni.rate_unit), font, color);
	auto ssid = std::make_shared<TextComponent>(window, ni.ssid, font, color);
	auto signal = std::make_shared<TextComponent>(window, formatWifiSignal( ni.signal ), font, color);
	auto channel = std::make_shared<TextComponent>(window, std::to_string(ni.channel), font, color);
	auto security = std::make_shared<TextComponent>(window, ni.security, font, color);

	status->setUpdatableFunction([status, internetStatus, isWifi, ssid, address, netmask, gateway, mac, dns1, dns2, signal, channel, security, rate]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update network";
			NetworkInformation ni = ApiSystem::getInstance()->getNetworkInformation(false);
			status->setText( formatNetworkStatus( ni.isConnected ) );
			internetStatus->setText( formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ) );
			isWifi->setText( "" );
			address->setText( "" );
			netmask->setText( "" );
			gateway->setText( "" );
			mac->setText( "" );
			dns1->setText( "" );
			dns2->setText( "" );
			rate->setText( "" );
			ssid->setText( "" );
			signal->setText( "" );
			channel->setText( "" );
			security->setText( "" );
			if (ni.isConnected)
			{
				isWifi->setText( _( Utils::String::boolToString(ni.isWifi, true) ) );
				address->setText( ni.ip_address );
				netmask->setText( ni.netmask );
				gateway->setText( ni.gateway );
				mac->setText( ni.mac );
				dns1->setText( ni.dns1 );
				dns2->setText( ni.dns2 );
				rate->setText(formatNetworkRate(ni.rate, ni.rate_unit) );
				if (ni.isWifi)
				{
					ssid->setText( ni.ssid );
					signal->setText( formatWifiSignal( ni.signal ) );
					channel->setText( std::to_string(ni.channel) );
					security->setText( ni.security );
				}
			}
		}, 10000);
	s->addUpdatableComponent(status.get());

	s->addWithLabel(_("STATUS"), status);
	s->addWithLabel(_("INTERNET STATUS"), internetStatus);
	s->addWithLabel(_("WIFI"), isWifi);
	s->addWithLabel(_("IP ADDRESS"), address);
	s->addWithLabel(_( (ni.isIPv6 ? "SUBNET" : "NETMASK") ), netmask);
	s->addWithLabel(_("GATEWAY"), gateway);
	s->addWithLabel(_("MAC"), mac);
	s->addWithLabel(_("DNS1"), dns1);
	s->addWithLabel(_("DNS2"), dns2);
	s->addWithLabel(_("CONNECTION RATE"), rate);
	s->addGroup(_("WIFI"));
	s->addWithLabel(_("WIFI SSID"), ssid);
	s->addWithLabel(_("WIFI SIGNAL"), signal);
	s->addWithLabel(_("WIFI CHANNEL"), channel);
	s->addWithLabel(_("WIFI SECURITY"), security);

	window->pushGui(s);
}

void GuiSystemInformation::openBattery(const BatteryInformation *bi)
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("BATTERY"));

	// level
	bool warning = isBatteryLimit(bi->level);
	auto level = std::make_shared<UpdatableTextComponent>(window, formatPercent( bi->level ), font, warning ? 0xFF0000FF : color);
	level->setUpdatableFunction([level, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update battery level";
			int battery_level_value = ApiSystem::getInstance()->getBatteryLevel();
			bool warning = isBatteryLimit( battery_level_value );
			level->setText(formatPercent( battery_level_value ));
			level->setColor(warning ? 0xFF0000FF : color);
		}, 30000);
	s->addUpdatableComponent(level.get());
	s->addWithLabel(_("LEVEL"), level);

	// is charging
	auto charging = std::make_shared<UpdatableTextComponent>(window, _(Utils::String::boolToString(bi->isCharging, true) ), font, color);
	charging->setUpdatableFunction([charging, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update battery charging";
			charging->setText(_(Utils::String::boolToString(ApiSystem::getInstance()->isBatteryCharging(), true) ));
		}, 10000);
	s->addUpdatableComponent(charging.get());
	s->addWithLabel(_("CHARGING"), charging);

	// health
	s->addWithLabel(_("HEALTH"), std::make_shared<TextComponent>(window, _( Utils::String::toUpper(bi->health) ), font, color));

	// max capacity
	s->addWithLabel(_("CAPACITY"), std::make_shared<TextComponent>(window, std::to_string(bi->max_capacity) + " mA", font, color));

	// voltage
	auto voltage = std::make_shared<UpdatableTextComponent>(window, formatVoltage( bi->voltage ), font, color);
	voltage->setUpdatableFunction([voltage, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update battery voltage";
			voltage->setText(formatVoltage(ApiSystem::getInstance()->getBatteryVoltage()));
		}, 5000);
	s->addUpdatableComponent(voltage.get());
	s->addWithLabel(_("VOLTAGE"), voltage);

	// temperature
	warning = isTemperatureLimit( bi->temperature );
	auto temperature = std::make_shared<UpdatableTextComponent>(window, formatTemperature( bi->temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature->setUpdatableFunction([temperature, color]
		{
			//LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture Battery";
			float temp_bat_value = ApiSystem::getInstance()->getTemperatureBattery();
			bool warning = isTemperatureLimit( temp_bat_value );
			temperature->setText(formatTemperature( temp_bat_value, warning ));
			temperature->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	s->addWithLabel(_("TEMPERATURE"), temperature);
	s->addUpdatableComponent(temperature.get());

	window->pushGui(s);
}

void GuiSystemInformation::openSoftware()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new GuiSettings(window, _("SOFTWARE"));

	SoftwareInformation si = ApiSystem::getInstance()->getSoftwareInformation();

	// kernel
	s->addWithLabel(_("HOSTNAME"), std::make_shared<TextComponent>(window, si.hostname, font, color));

	// application Name
	s->addWithLabel(_("SO"), std::make_shared<TextComponent>(window, si.application_name, font, color));

	// application Name
	s->addWithLabel(_("VERSION"), std::make_shared<TextComponent>(window, si.version, font, color));

	// SO base
	s->addWithLabel(_("SO BASE"), std::make_shared<TextComponent>(window, si.so_base, font, color));

	// kernel
	s->addWithLabel(_("KERNEL"), std::make_shared<TextComponent>(window, si.vlinux, font, color));

	// ES version
	s->addWithLabel(_("EMULATIONSTATION"), std::make_shared<TextComponent>(window, si.es_version, font, color));

	// ES built
	s->addWithLabel(_("BUILT"), std::make_shared<TextComponent>(window, si.es_built, font, color));

	window->pushGui(s);
}

void GuiSystemInformation::openDevice()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new GuiSettings(window, _("DEVICE"));

	DeviceInformation di = ApiSystem::getInstance()->getDeviceInformation();

	// device name
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::SYSTEM_INFORMATION))
		s->addWithLabel(_("NAME"), std::make_shared<TextComponent>(window, di.name, font, color));

	// device hardware
	s->addWithLabel(_("HARDWARE"), std::make_shared<TextComponent>(window, di.hardware, font, color));

	// revision
	s->addWithLabel(_("REVISION"), std::make_shared<TextComponent>(window, di.revision, font, color));

	// serial
	s->addWithLabel(_("SERIAL"), std::make_shared<TextComponent>(window, di.serial, font, color));

	// machine ID
	s->addWithLabel(_("MACHINE ID"), std::make_shared<TextComponent>(window, di.machine_id, font, color));

	// boot ID
	s->addWithLabel(_("BOOT ID"), std::make_shared<TextComponent>(window, di.boot_id, font, color));

	window->pushGui(s);
}

std::string GuiSystemInformation::formatTemperature(float temp_raw, bool warning)
{
	char buffer [16];
	sprintf (buffer, "%.2f° C", temp_raw);
	std::string temperature(buffer);
	return (warning ? _U("\uF2C7  ") : "  ") + temperature;
}

std::string GuiSystemInformation::formatFrequency(int freq_raw)
{
	char buffer [16];
	sprintf (buffer, "%d MHZ", freq_raw);
	std::string freq(buffer);
	return freq;
}

std::string GuiSystemInformation::formatPercent(int bat_level)
{
	return std::to_string( bat_level ) + " %";
}

std::string GuiSystemInformation::formatWifiSignal(int wifi_signal)
{
	return formatPercent( wifi_signal );
}

std::string GuiSystemInformation::formatMemory(float mem_raw, float total_memory, bool show_percent)
{
	char buffer [32];
	float percent = 0.f;
	std::string format("%3.2f MB");
	if (show_percent)
	{
		format.append(" (%2.1f %%)");
		percent = (mem_raw * 100) / total_memory;
	}

	sprintf (buffer, format.c_str(), mem_raw, percent);
	std::string mem(buffer);
	return mem;
}

std::string GuiSystemInformation::formatLoadCpu(float cpu_load_raw)
{
	char buffer [16];
	sprintf (buffer, "%3.2f %%", cpu_load_raw);
	std::string cpu_load(buffer);
	return cpu_load;
}

std::string GuiSystemInformation::formatVoltage(float voltage_raw)
{
	char buffer [16];
	sprintf (buffer, "%1.2f V", voltage_raw);
	std::string voltage(buffer);
	return voltage;
}

std::string GuiSystemInformation::formatNetworkStatus(bool isConnected)
{
	return (isConnected ? "    " + _("CONNECTED") + " " : _("NOT CONNECTED"));
}

std::string GuiSystemInformation::formatNetworkRate(int rate, std::string units)
{
	return std::to_string(rate) + " " + units;
}

std::string GuiSystemInformation::formatStorage(std::string used, std::string avail, std::string used_percent)
{
	float used_value = std::atof(used.c_str()),
		  avail_value = std::atof(avail.c_str()),
		  total = .0f;
	std::string used_unit = used.substr(used.length() - 2),
			   avail_unit = used.substr(avail.length() - 2),
			   total_unit = "GB";

	LOG(LogDebug) << "GuiSystemInformation::formatStorage() - used: " << used << ", avail: " << avail << ", used_percent: " << used_percent;

	if (used_unit != avail_unit)
	{
		if (used_unit == "MB")
			used_value = used_value / 1024; // convert to GB
		else if (avail_unit == "MB")
			avail_value = avail_value / 1024; // convert to GB
	}
	else
		total_unit = used_unit;

	total = used_value + avail_value;
	if ((total > 1024) && (total_unit == "MB"))
	{
		total = total / 1024; // convert to GB
	}
	char buffer [16];
	sprintf (buffer, "%.2f", total);
	std::string result;
	result.append(used).append("/").append(buffer).append(total_unit).append(" (").append(used_percent).append(")");
	return result;
}

bool GuiSystemInformation::isTemperatureLimit(float temperature)
{
	return temperature > 70.0f;
}

bool GuiSystemInformation::isStorageLimit(std::string used_percent)
{
	return std::atoi(used_percent.c_str()) > 90;
}
bool GuiSystemInformation::isCpuLoadLimit(float cpu_load)
{
	return cpu_load > 90.0f;
}

bool GuiSystemInformation::isBatteryLimit(int battery_level)
{
	return battery_level < 15;
}

bool GuiSystemInformation::isMemoryLimit(float total_memory, float free_memory)
{
	int percent = ( (int) ( (free_memory * 100) / total_memory ) );
	return percent < 10;
}
