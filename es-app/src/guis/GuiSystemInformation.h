#pragma once
#ifndef ES_APP_GUIS_GUI_SYSTEM_INFORMATION_H
#define ES_APP_GUIS_GUI_SYSTEM_INFORMATION_H

#include "UpdatableGuiSettings.h"
#include "platform.h"
#include "Settings.h"

class UpdatableGuiSettings;

class GuiSystemInformation : public UpdatableGuiSettings
{
public:
	GuiSystemInformation(Window* window);
	~GuiSystemInformation();

private:
	void initializeMenu();
	void showSummarySystemInfo();
	void showDetailedSystemInfo();
	void openCpuAndSocket();
	void openRamMemory();
	void openDisplayAndGpu();
	void openStorage();
	void openNetwork();
	void openBattery(const BatteryInformation *bi);
	void openSoftware();
	void openDevice();

	void configUsbDriveDevices(UpdatableGuiSettings *parent, const std::shared_ptr<Font>& font, unsigned int color);

	static std::string formatTemperature  (float temp_raw, bool warning = false);
	static std::string formatFrequency    (int freq_raw);
	static std::string formatBattery      (int bat_level);
	static std::string formatWifiSignal   (int wifi_signal);
	static std::string formatMemory       (float mem_raw, float total_memory = 0.f, bool show_percent = false);
	static std::string formatLoadCpu      (float cpu_load);
	static std::string formatVoltage      (float voltage_raw);
	static std::string formatNetworkStatus(bool isConnected);
	static std::string formatNetworkRate  (int rate, std::string units);

};

#endif // ES_APP_GUIS_GUI_SYSTEM_INFORMATION_H
