#pragma once
#ifndef ES_APP_GUIS_GUI_MENU_H
#define ES_APP_GUIS_GUI_MENU_H

#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "GuiComponent.h"

class GuiSettings;
class SystemData;

class GuiMenu : public GuiComponent
{
public:
	GuiMenu(Window* window, bool animate = true);

	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	static void openQuitMenu_static(Window *window, bool quickAccessMenu = false, bool animate = true);
	HelpStyle getHelpStyle() override;

	static void openThemeConfiguration(Window* mWindow, GuiComponent* s, std::shared_ptr<OptionListComponent<std::string>> theme_set, const std::string systemTheme = "");

	static void updateGameLists(Window* window, bool confirm = true);

private:
	void addEntry(std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName = "");

	void addVersionInfo();
	void openCollectionSystemSettings(bool cursor = false);
	void openConfigInput();
	void openAdvancedSettings();
	void openQuitMenu();
	void openControllersSettings();
	void openScraperSettings();
	void openScreensaverOptions();
	void openSoundSettings();
	void openUISettings();
	void openSystemInformation();
	void openMenusSettings(GuiSettings *parentGui);
	void openQuitSettings(GuiSettings *parentGui);
	void openAutoSuspendSettings();
	void openSystemHotkeyEventsSettings();
	void openRetroAchievementsSettings();
	void openNetworkSettings(bool selectWifiEnable = false, bool selectManualWifiDnsEnable = false);
	void resetNetworkSettings(GuiSettings *gui);

	static void openWifiSettings(Window* win, std::string title, std::string data, const std::function<bool(std::string)>& onsave);

	void openBluetoothSettings();
	void openBluetoothDevicesAlias(Window* window, std::string title);
	void openBluetoothScanDevices(Window* win, std::string title);
	void openBluetoothPairedDevices(Window* win, std::string title);
	void openBluetoothConnectedDevices(Window* win, std::string title);
	bool displayBluetoothAudioRestartDialog(Window *window, bool force = false);

	void openUpdateSettings();
	void openEmulatorSettings();
	void openSystemEmulatorSettings(SystemData* system);
	void openDisplaySettings();
	void openDisplayAutoDimSettings();
	void openRemoteServicesSettings();

	void reinitSoundComponentsAndMusic();

	void addStatusBarInfo(Window* window);

	static std::string formatNetworkStatus(bool isConnected);
	static std::string getIconBattery(int level, bool isCharging);
	static std::string getIconSound(int level);
	static std::string getIconBrightness(int level);
	static std::string getIconNetwork(bool status);
	static std::string getIconBluetooth(bool status);

	MenuComponent mMenu;
	TextComponent mVersion;
};

#endif // ES_APP_GUIS_GUI_MENU_H
