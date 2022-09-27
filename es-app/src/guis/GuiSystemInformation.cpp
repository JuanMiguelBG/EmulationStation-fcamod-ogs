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

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	UpdatableGuiSettings *pthis = this;

	CpuAndSocketInformation csi = ApiSystem::getInstance()->getCpuAndChipsetInformation();
	DisplayAndGpuInformation di = ApiSystem::getInstance()->getDisplayAndGpuInformation();
	RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation();
	NetworkInformation ni = ApiSystem::getInstance()->getNetworkInformation();

	// device name
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SYSTEM_INFORMATION))
		addWithLabel(_("DEVICE"), std::make_shared<TextComponent>(mWindow, ApiSystem::getInstance()->getDeviceName(), font, color));

	addGroup(_("CPU"));
	// CPU load
	bool warning = ApiSystem::getInstance()->isLoadCpuLimit( csi.cpu_load );
	auto loadCpu = std::make_shared<UpdatableTextComponent>(mWindow, formatLoadCpu( csi.cpu_load ), font, warning ? 0xFF0000FF : color);
	loadCpu->setUpdatableFunction([loadCpu, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update load CPU";
			float load_cpu_value = ApiSystem::getInstance()->getLoadCpu();
			bool warning = ApiSystem::getInstance()->isLoadCpuLimit( load_cpu_value );
			loadCpu->setText(formatLoadCpu( load_cpu_value ));
			loadCpu->setColor(warning ? 0xFF0000FF : color);
		}, 4000);
	addUpdatableComponent(loadCpu.get());
	addWithLabel(_("CPU LOAD"), loadCpu);

	// CPU temperature
	warning = ApiSystem::getInstance()->isTemperatureLimit( csi.temperature );
	auto temperatureCpu = std::make_shared<UpdatableTextComponent>(mWindow, formatTemperature( csi.temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperatureCpu->setUpdatableFunction([temperatureCpu, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture CPU";
			float temp_cpu_value = ApiSystem::getInstance()->getTemperatureCpu();
			bool warning = ApiSystem::getInstance()->isTemperatureLimit( temp_cpu_value );
			temperatureCpu->setText(formatTemperature( temp_cpu_value, warning ));
			temperatureCpu->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	addUpdatableComponent(temperatureCpu.get());
	addWithLabel(_("TEMPERATURE"), temperatureCpu);

	addGroup(_("OTHER INFORMATION"));
	// GPU temperature
	warning = ApiSystem::getInstance()->isTemperatureLimit( di.temperature );
	auto temperature_gpu = std::make_shared<UpdatableTextComponent>(mWindow, formatTemperature( di.temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature_gpu->setUpdatableFunction([temperature_gpu, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture GPU";
			float temp_gpu_value = ApiSystem::getInstance()->getTemperatureGpu();
			bool warning = ApiSystem::getInstance()->isTemperatureLimit( temp_gpu_value );
			temperature_gpu->setText(formatTemperature( temp_gpu_value, warning ));
			temperature_gpu->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	addUpdatableComponent(temperature_gpu.get());
	addWithLabel(_("GPU") + " - " + _("TEMPERATURE"), temperature_gpu);

	// battery temperature
	float temp_bat_value = ApiSystem::getInstance()->getTemperatureBattery();
	warning = ApiSystem::getInstance()->isTemperatureLimit( temp_bat_value );
	auto temperature_battery = std::make_shared<UpdatableTextComponent>(mWindow, formatTemperature( temp_bat_value, warning ), font, warning ? 0xFF0000FF : color);
	temperature_battery->setUpdatableFunction([temperature_battery, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture Battery";
			float temp_bat_value = ApiSystem::getInstance()->getTemperatureBattery();
			bool warning = ApiSystem::getInstance()->isTemperatureLimit( temp_bat_value );
			temperature_battery->setText(formatTemperature( temp_bat_value, warning ));
			temperature_battery->setColor(warning ? 0xFF0000FF : color);
		}, 5000);
	addUpdatableComponent(temperature_battery.get());
	addWithLabel(_("BATTERY") + " - " + _("TEMPERATURE"), temperature_battery);

	// free ram
	warning = ApiSystem::getInstance()->isMemoryLimit( memory.total, memory.free );
	auto memoryFree = std::make_shared<UpdatableTextComponent>(mWindow, formatMemory( memory.free, memory.total, true ), font, warning ? 0xFF0000FF : color);
	memoryFree->setUpdatableFunction([memoryFree, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update memory free";
			RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation();
			bool warning = ApiSystem::getInstance()->isMemoryLimit( memory.total, memory.free );
			memoryFree->setText(formatMemory( memory.free, memory.total, true ));
			memoryFree->setColor(warning ? 0xFF0000FF : color);
		}, 10000);
	addUpdatableComponent(memoryFree.get());
	addWithLabel(_("FREE RAM MEMORY"), memoryFree);

	addGroup(_("STORAGE"));
	// roms
	warning = ApiSystem::getInstance()->isFreeSpaceUserLimit();
	auto userSpace = std::make_shared<UpdatableTextComponent>(mWindow, ApiSystem::getInstance()->getFreeSpaceUserInfo(), font, warning ? 0xFF0000FF : color);
	userSpace->setUpdatableFunction([userSpace, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update user space";
			bool warning = ApiSystem::getInstance()->isFreeSpaceUserLimit();
			userSpace->setText(ApiSystem::getInstance()->getFreeSpaceUserInfo());
			userSpace->setColor(warning ? 0xFF0000FF : color);
		}, 30000);
	addUpdatableComponent(userSpace.get());
	addWithLabel(_("ROMS DISK USAGE"), userSpace);

	// roms2
	if (ApiSystem::getInstance()->isUser2Mounted())
	{
		warning = ApiSystem::getInstance()->isFreeSpaceUser2Limit();
		auto userSpace2 = std::make_shared<UpdatableTextComponent>(mWindow, ApiSystem::getInstance()->getFreeSpaceUser2Info(), font, warning ? 0xFF0000FF : color);
		userSpace2->setUpdatableFunction([userSpace2, color]
			{
				LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update user space 2";
				bool warning = ApiSystem::getInstance()->isFreeSpaceUser2Limit();
				userSpace2->setText(ApiSystem::getInstance()->getFreeSpaceUser2Info());
				userSpace2->setColor(warning ? 0xFF0000FF : color);
			}, 30000);
		addUpdatableComponent(userSpace2.get());
		addWithLabel(_("ROMS2 DISK USAGE"), userSpace2);
	}

	// usbdrive
	configUsbDriveDevices(pthis, font, color);

	addGroup(_("NETWORK"));

	// connected to network
	auto networkStatus = std::make_shared<UpdatableTextComponent>(mWindow, formatNetworkStatus( ni.isConnected ), font, color);

	// acces to internet
	auto internetStatus = std::make_shared<TextComponent>(mWindow, formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ), font, color);

	// Wifi ssid
	auto wifiSsid = std::make_shared<TextComponent>(mWindow, ni.ssid, font, color);
	// IP address
	auto ipAddress = std::make_shared<TextComponent>(mWindow, ni.ip_address, font, color);
	networkStatus->setUpdatableFunction([networkStatus, internetStatus, wifiSsid, ipAddress, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update network status";
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

void GuiSystemInformation::configUsbDriveDevices(UpdatableGuiSettings *parent, const std::shared_ptr<Font>& font, unsigned int color)
{
	std::vector<std::string> usbmps = ApiSystem::getInstance()->getUsbDriveMountPoints();
	LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - USB partition size: " << std::to_string(usbmps.size());
	if (!usbmps.empty())
	{
		for (auto mount_point = begin (usbmps); mount_point != end (usbmps); ++mount_point)
		{
			if (mount_point->empty())
				continue;
			std::string value = ApiSystem::getInstance()->getFreeSpaceUsbDriveInfo(*mount_point);
			bool warning = ApiSystem::getInstance()->isFreeSpaceUsbDriveLimit(*mount_point);
			auto usbdrive = std::make_shared<UpdatableTextComponent>(mWindow, value, font, warning ? 0xFF0000FF : color);
			usbdrive->setVisible(!value.empty());
			if (usbdrive->isVisible())
			{
				usbdrive->setUpdatableFunction([usbdrive, color, mount_point]
					{
						LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update usbdrive: \"" << *mount_point << '"';
						if (usbdrive->isVisible())
						{
							std::string value = ApiSystem::getInstance()->getFreeSpaceUsbDriveInfo(*mount_point);
							bool warning = ApiSystem::getInstance()->isFreeSpaceUsbDriveLimit(*mount_point);
							usbdrive->setVisible(!value.empty());
							usbdrive->setText(value);
							usbdrive->setColor(warning ? 0xFF0000FF : color);
						}
					}, 30000);
				parent->addUpdatableComponent(usbdrive.get());
				parent->addWithLabel(_("USB DISK USAGE") + " " + *mount_point, usbdrive);
			}
		}
	}
}

void GuiSystemInformation::showDetailedSystemInfo()
{
	LOG(LogInfo) << "GuiSystemInformation::showDetailedSystemInfo()";

	addSubMenu(_("CPU AND SOCKET"), [this]() { openCpuAndSocket(); });
	addSubMenu(_("RAM MEMORY"), [this]() { openRamMemory(); });
	addSubMenu(_("DISPLAY AND GPU"), [this]() { openDisplayAndGpu(); });
	addSubMenu(_("STORAGE"), [this]() { openStorage(); });
	addSubMenu(_("NETWORK"), [this]() { openNetwork(); });

	BatteryInformation bi = ApiSystem::getInstance()->getBatteryInformation(false);
	LOG(LogDebug) << "GuiSystemInformation::showDetailedSystemInfo() - has battery: " << Utils::String::boolToString(bi.hasBattery);
	if (bi.hasBattery)
		addSubMenu(_("BATTERY"), [this, bi]() { openBattery(&bi); });

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SYSTEM_INFORMATION))
		addSubMenu(_("SOFTWARE"), [this]() { openSoftware(); });

	addSubMenu(_("DEVICE"), [this]() { openDevice(); });
}

void GuiSystemInformation::openCpuAndSocket()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("CPU AND SOCKET"));

	CpuAndSocketInformation csi = ApiSystem::getInstance()->getCpuAndChipsetInformation(false);

	// SOC name
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SYSTEM_INFORMATION))
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
	bool warning = ApiSystem::getInstance()->isLoadCpuLimit( csi.cpu_load );
	auto load = std::make_shared<UpdatableTextComponent>(window, formatLoadCpu( csi.cpu_load ), font, warning ? 0xFF0000FF : color);
	load->setUpdatableFunction([load, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update load CPU";
			float load_cpu_value = ApiSystem::getInstance()->getLoadCpu();
			bool warning = ApiSystem::getInstance()->isLoadCpuLimit( load_cpu_value );
			load->setText(formatLoadCpu( load_cpu_value ));
			load->setColor(warning ? 0xFF0000FF : color);
		}, 2000);
	s->addWithLabel(_("CPU LOAD"), load);
	s->addUpdatableComponent(load.get());

	// temperature
	warning = ApiSystem::getInstance()->isTemperatureLimit( csi.temperature );
	auto temperature = std::make_shared<UpdatableTextComponent>(window, formatTemperature( csi.temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature->setUpdatableFunction([temperature, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture CPU";
			float temp_cpu_value = ApiSystem::getInstance()->getTemperatureCpu();
			bool warning = ApiSystem::getInstance()->isTemperatureLimit( temp_cpu_value );
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
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update frequency CPU";
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

	RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation(false);

	// total ram
	s->addWithLabel(_("TOTAL"), std::make_shared<TextComponent>(window, formatMemory( memory.total ), font, color));

	// free ram
	bool warning = ApiSystem::getInstance()->isMemoryLimit( memory.total, memory.free );
	auto memoryFree = std::make_shared<UpdatableTextComponent>(window, formatMemory( memory.free, memory.total, true ), font, warning ? 0xFF0000FF : color);
	auto memoryUsed = std::make_shared<TextComponent>(window, formatMemory( memory.used, memory.total, true ), font, warning ? 0xFF0000FF : color);
	auto memoryCached = std::make_shared<TextComponent>(window, formatMemory( memory.cached, memory.total, true ), font, warning ? 0xFF0000FF : color);

	memoryFree->setUpdatableFunction([memoryFree, memoryUsed, memoryCached, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update memory free";
			RamMemoryInformation memory = ApiSystem::getInstance()->getRamMemoryInformation(false);
			bool warning = ApiSystem::getInstance()->isMemoryLimit( memory.total, memory.free );
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

	DisplayAndGpuInformation di = ApiSystem::getInstance()->getDisplayAndGpuInformation(false);

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

	// temperature
	bool warning = ApiSystem::getInstance()->isTemperatureLimit( di.temperature );
	auto temperature = std::make_shared<UpdatableTextComponent>(window, formatTemperature( di.temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature->setUpdatableFunction([temperature, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture GPU";
			float temp_gpu_value = ApiSystem::getInstance()->getTemperatureGpu();
			bool warning = ApiSystem::getInstance()->isTemperatureLimit( temp_gpu_value );
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
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture GPU";
			frequency->setText(formatFrequency( ApiSystem::getInstance()->getFrequencyGpu() ));
		}, 5000);
	s->addUpdatableComponent(frequency.get());
	s->addWithLabel(_("FREQUENCY"), frequency);

	// maximus frequency
	s->addWithLabel(_("FREQUENCY MAX"), std::make_shared<TextComponent>(window, formatFrequency( di.frequency_max ), font, color));

	// minimus frequency
	s->addWithLabel(_("FREQUENCY MIN"), std::make_shared<TextComponent>(window, formatFrequency( di.frequency_min ), font, color));

	// brightness %
	auto brightness = std::make_shared<UpdatableTextComponent>(window, formatBattery( di.brightness_level ), font, color);

	// brightness raw
	auto brightness_system = std::make_shared<TextComponent>(window, std::to_string( di.brightness_system ), font, color);
	brightness->setUpdatableFunction([brightness, brightness_system]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update brightness display";
			brightness->setText(formatBattery( ApiSystem::getInstance()->getBrightnessLevel() ));
			brightness_system->setText( std::to_string( ApiSystem::getInstance()->getBrightness() ) );
		}, 5000);
	s->addUpdatableComponent(brightness.get());

	s->addWithLabel(_("BRIGHTNESS"), brightness);
	s->addWithLabel(_("SYSTEM BRIGHTNESS"), brightness_system);

	// maximus brightness raw
	s->addWithLabel(_("MAX SYSTEM BRIGHTNESS"), std::make_shared<TextComponent>(window, std::to_string( di.brightness_system_max ), font, color));

	window->pushGui(s);
}

void GuiSystemInformation::openStorage()
{
	auto pthis = this;
	Window* window = mWindow;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	auto s = new UpdatableGuiSettings(window, _("STORAGE"));

	// boot
	bool warning = ApiSystem::getInstance()->isFreeSpaceBootLimit();
	auto bootSpace = std::make_shared<TextComponent>(window, ApiSystem::getInstance()->getFreeSpaceBootInfo(), font, warning ? 0xFF0000FF : color);

	// system
	warning = ApiSystem::getInstance()->isFreeSpaceSystemLimit();
	auto systemSpace = std::make_shared<TextComponent>(window, ApiSystem::getInstance()->getFreeSpaceSystemInfo(), font, warning ? 0xFF0000FF : color);

	// roms
	warning = ApiSystem::getInstance()->isFreeSpaceUserLimit();
	auto userSpace = std::make_shared<UpdatableTextComponent>(window, ApiSystem::getInstance()->getFreeSpaceUserInfo(), font, warning ? 0xFF0000FF : color);

	userSpace->setUpdatableFunction([bootSpace, systemSpace, userSpace, color]//usbdrive, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update storage";
			bool warning = ApiSystem::getInstance()->isFreeSpaceBootLimit();
			bootSpace->setText(ApiSystem::getInstance()->getFreeSpaceBootInfo());
			bootSpace->setColor(warning ? 0xFF0000FF : color);

			warning = ApiSystem::getInstance()->isFreeSpaceSystemLimit();
			systemSpace->setText(ApiSystem::getInstance()->getFreeSpaceSystemInfo());
			systemSpace->setColor(warning ? 0xFF0000FF : color);

			warning = ApiSystem::getInstance()->isFreeSpaceUserLimit();
			userSpace->setText(ApiSystem::getInstance()->getFreeSpaceUserInfo());
			userSpace->setColor(warning ? 0xFF0000FF : color);
		}, 30000);
	s->addUpdatableComponent(userSpace.get());
	s->addWithLabel(_("BOOT DISK USAGE"), bootSpace);
	s->addWithLabel(_("SYSTEM DISK USAGE"), systemSpace);
	s->addWithLabel(_("ROMS DISK USAGE"), userSpace);

	// roms2
	if (ApiSystem::getInstance()->isUser2Mounted())
	{
		warning = ApiSystem::getInstance()->isFreeSpaceUser2Limit();
		auto userSpace2 = std::make_shared<UpdatableTextComponent>(mWindow, ApiSystem::getInstance()->getFreeSpaceUser2Info(), font, warning ? 0xFF0000FF : color);
		userSpace2->setUpdatableFunction([userSpace2, color]
			{
				LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update user space 2";
				bool warning = ApiSystem::getInstance()->isFreeSpaceUser2Limit();
				userSpace2->setText(ApiSystem::getInstance()->getFreeSpaceUser2Info());
				userSpace2->setColor(warning ? 0xFF0000FF : color);
			}, 30000);
		addUpdatableComponent(userSpace2.get());
		s->addWithLabel(_("ROMS2 DISK USAGE"), userSpace2);
	}

	// usbdrive
	configUsbDriveDevices(s, font, color);

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
	auto internetStatus = std::make_shared<TextComponent>(mWindow, formatNetworkStatus( ApiSystem::getInstance()->getInternetStatus() ), font, color);
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
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update network";
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
	bool warning = ApiSystem::getInstance()->isBatteryLimit(bi->level);
	auto level = std::make_shared<UpdatableTextComponent>(window, formatBattery( bi->level ), font, warning ? 0xFF0000FF : color);
	level->setUpdatableFunction([level, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update battery level";
			int battery_level_value = ApiSystem::getInstance()->getBatteryLevel();
			bool warning = ApiSystem::getInstance()->isBatteryLimit( battery_level_value );
			level->setText(formatBattery( battery_level_value ));
			level->setColor(warning ? 0xFF0000FF : color);
		}, 30000);
	s->addUpdatableComponent(level.get());
	s->addWithLabel(_("LEVEL"), level);

	// is charging
	auto charging = std::make_shared<UpdatableTextComponent>(window, _(Utils::String::boolToString(bi->isCharging, true) ), font, color);
	charging->setUpdatableFunction([charging, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update battery charging";
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
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update battery voltage";
			voltage->setText(formatVoltage(ApiSystem::getInstance()->getBatteryVoltage()));
		}, 5000);
	s->addUpdatableComponent(voltage.get());
	s->addWithLabel(_("VOLTAGE"), voltage);

	// temperature
	warning = ApiSystem::getInstance()->isTemperatureLimit( bi->temperature );
	auto temperature = std::make_shared<UpdatableTextComponent>(window, formatTemperature( bi->temperature, warning ), font, warning ? 0xFF0000FF : color);
	temperature->setUpdatableFunction([temperature, color]
		{
			LOG(LogDebug) << "GuiSystemInformation::showSummarySystemInfo() - update temperture Battery";
			float temp_bat_value = ApiSystem::getInstance()->getTemperatureBattery();
			bool warning = ApiSystem::getInstance()->isTemperatureLimit( temp_bat_value );
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

	SoftwareInformation si = ApiSystem::getInstance()->getSoftwareInformation(false);

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

	DeviceInformation di = ApiSystem::getInstance()->getDeviceInformation(false);

	// device name
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SYSTEM_INFORMATION))
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

std::string GuiSystemInformation::formatBattery(int bat_level)
{
	return std::to_string( bat_level ) + " %";
}

std::string GuiSystemInformation::formatWifiSignal(int wifi_signal)
{
	return formatBattery( wifi_signal );
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
