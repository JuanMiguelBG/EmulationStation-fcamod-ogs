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
	void openCollectionSystemSettings();
	void openConfigInput();
	void openAdvancedSettings();
	void openQuitMenu();
	void openControllersSettings();
	void openScraperSettings();
	void openScreensaverOptions();
	void openSoundSettings();
	void openUISettings();
	void openSystemInformation();
	void openQuitSettings();
	void openPowerkeySettings();
	void openMenusSettings();
	void openAutoSuspendSettings();
	void openSystemHotkeyEventsSettings();
	void openRetroAchievementsSettings();
	void preloadNetworkSettings();
	void openNetworkSettings(bool selectWifiEnable = false, bool selectManualWifiDnsEnable = false);
	void resetNetworkSettings(GuiSettings *gui);

	static void openWifiSettings(Window* win, std::string title, std::string data, const std::function<bool(std::string)>& onsave);

	void openUpdateSettings();
	void openEmulatorSettings();
	void openSystemEmulatorSettings(SystemData* system);
	void openDisplaySettings();
	void openDisplayAutoDimSettings();
	void openRemoteServicesSettings();

	static std::string formatNetworkStatus(bool isConnected);
	static std::string formatIconsBatteryStatus(int level, bool isCharging);
	static std::string formatIconsSoundStatus(int level);
	static std::string formatIconsBrightnessStatus(int level);
	static std::string formatIconsNetworkStatus(bool status);

	MenuComponent mMenu;
	TextComponent mVersion;

};

#endif // ES_APP_GUIS_GUI_MENU_H
