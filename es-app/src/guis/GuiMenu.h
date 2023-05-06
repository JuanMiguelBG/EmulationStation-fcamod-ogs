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
	enum CursortId : unsigned int
	{
		FIRST_ELEMENT = 0,
		KODI = 1,
		DISPLAY_SETTINGS = 2,
		UI_SETTINGS = 3,
		CONTROLLER_SETTINGS = 4,
		SOUND_SETTINGS = 5,
		GAME_COLL_SETTINGS = 6,
		EMULATOR_SETTINGS = 7,
		NET_SETTINGS = 8,
		BT_SETTINGS = 9,
		SCRAPPER_SETTINGS = 10,
		ADVANCED_SETTINGS = 11,
		SYSTEM_INFORMATION = 12,
	};

	GuiMenu(Window* window, bool animate = true, CursortId cursor = CursortId::FIRST_ELEMENT);

	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	static void openQuitMenu_static(Window *window, bool quickAccessMenu = false, bool animate = true);
	HelpStyle getHelpStyle() override;

	static void openThemeConfiguration(Window* mWindow, GuiComponent* s, std::shared_ptr<OptionListComponent<std::string>> theme_set, const std::string systemTheme = "");

	static void updateGameLists(Window* window, bool confirm = true);
	static void clearLastPlayedData(Window* window, const std::string system = "", bool confirm = true);

private:
	void addEntry(std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName = "", bool setCursorHere = false);

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
	void openManageKnowedWifiNetworks(GuiSettings *gui);

	static void openWifiSettings(Window* window, std::string title, std::string data, const std::function<bool(std::string)>& onsave);

	void openBluetoothSettings();
	void openBluetoothDevicesAlias(Window* window, std::string title);
	void openBluetoothScanDevices(Window* window, std::string title);
	void openBluetoothPairedDevices(Window* window, std::string title);
	void openBluetoothConnectedDevices(Window* window, std::string title);
	bool displayBluetoothAudioRestartDialog(Window *window, bool force = false);

	void openUpdateSettings();
	void openEmulatorSettings();
	void openSystemEmulatorSettings(SystemData* system);
	void openDisplaySettings();
	void openDisplayPanelOptions();
	void openDisplayAutoDimSettings();
	void openRemoteServicesSettings();

	void stopSoundComponentsAndMusic();
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
	bool mWaitingLoad;
};

#endif // ES_APP_GUIS_GUI_MENU_H
