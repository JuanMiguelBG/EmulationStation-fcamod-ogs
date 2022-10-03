#include "Settings.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
//#include "Log.h"
#include "Scripting.h"
#include "platform.h"
#include <pugixml/src/pugixml.hpp>
#include <algorithm>
#include <vector>
#include "resources/ResourceManager.h"
#include <iostream>

Settings* Settings::sInstance = NULL;
static std::string mEmptyString = "";

// these values are NOT saved to es_settings.xml
// since they're set through command-line arguments, and not the in-program settings menu
std::vector<const char*> settings_dont_save {
	{ "Debug" },
	{ "DebugGrid" },
	{ "DebugText" },
	{ "DebugImage" },
	{ "ForceKid" },
	{ "ForceKiosk" },
	{ "IgnoreGamelist" },
	{ "ShowExit" },
	{ "SplashScreen" },
	{ "SplashScreenProgress" },
	{ "VSync" },
	{ "MusicDirectory" },
	{ "UserMusicDirectory" },
	{ "ThemeRandomSet" },
	{ "global.retroachievements.username" },
	{ "global.retroachievements.password" },
	{ "system.hostname" },
	{ "wifi.enabled" },
	{ "wifi.key" },
//	{ "wifi.ssid" },
	{ "wifi.dns1" },
	{ "wifi.dns2" },
	{ "wait.process.loading" }
};

Settings::Settings()
{
	setDefaults();
	loadFile();
}

Settings* Settings::getInstance()
{
	if(sInstance == NULL)
		sInstance = new Settings();

	return sInstance;
}

void Settings::setDefaults()
{
	mHasConfigRoot = false;
	mWasChanged = false;
	mBoolMap.clear();
	mIntMap.clear();

	mBoolMap["BackgroundJoystickInput"] = false;
	mBoolMap["InvertButtonsAB"] = false;
	mBoolMap["InvertButtonsPU"] = false;
	mBoolMap["InvertButtonsPD"] = false;
	mBoolMap["ParseGamelistOnly"] = false;
	mBoolMap["ShowHiddenFiles"] = false;
	mBoolMap["IgnoreLeadingArticles"] = false;
	mBoolMap["DrawFramerate"] = false;
	mBoolMap["ShowExit"] = true;

	mBoolMap["ShowOnlyExit"] = false;
	mStringMap["OnlyExitAction"] = "shutdown";
	mBoolMap["ShowOnlyExitActionAsMenu"] = false;
	mBoolMap["ConfirmToExit"] = true;
	mBoolMap["ShowQuitMenuWithSelect"] = false;
	mBoolMap["ShowFastQuitActions"] = false;

	mBoolMap["SplashScreen"] = true;
	mBoolMap["SplashScreenProgress"] = true;
	mBoolMap["PreloadUI"] = false;
	mBoolMap["PreloadMedias"] = false;
	mBoolMap["PreloadVLC"] = true;
	mBoolMap["StartupOnGameList"] = false;
	mBoolMap["HideSystemView"] = false;
	mBoolMap["BrightnessPopup"] = true;
	mBoolMap["DisplayBlinkLowBattery"] = false;

	mStringMap["StartupSystem"] = "lastsystem";

	mStringMap["FolderViewMode"] = "never";

	mBoolMap["UseOSK"] = true;
	//mBoolMap["ShowControllerActivity"] = false;
	mBoolMap["ShowBatteryIndicator"] = true;
	mBoolMap["ShowNetworkIndicator"] = true;

	mBoolMap["VSync"] = true;
	mBoolMap["DrawClock"] = true;
	mBoolMap["ClockMode12"] = false;
	mBoolMap["EnableSounds"] = true;
	mBoolMap["ShowHelpPrompts"] = true;
	mBoolMap["ScrapeRatings"] = true;
	mBoolMap["IgnoreGamelist"] = false;
	mBoolMap["QuickSystemSelect"] = true;
	mBoolMap["MoveCarousel"] = true;
	mBoolMap["SaveGamelistsOnExit"] = true;
	mStringMap["ShowBattery"] = "text";
	mBoolMap["OptimizeVRAM"] = true;
	mBoolMap["ThreadedLoading"] = true;
	mBoolMap["OptimizeSystem"] = false;
	mBoolMap["AutoMenuWidth"] = false;
	mIntMap["DescriptionAutoScrollDelay"] = 3; // s to wait before we start to scroll

	mBoolMap["Debug"] = false;
	mBoolMap["DebugGrid"] = false;
	mBoolMap["DebugText"] = false;
	mBoolMap["DebugImage"] = false;

	mIntMap["ScreenSaverTime"] = 5*60*1000; // 5 minutes
	mIntMap["ScraperResizeWidth"] = 400;
	mIntMap["ScraperResizeHeight"] = 0;

	mIntMap["MaxVRAM"] = 150;

	mBoolMap["HideWindow"] = true;

	mStringMap["GameTransitionStyle"] = "fade";
	mStringMap["TransitionStyle"] = "auto";
	mStringMap["Language"] = "en";
	mStringMap["ThemeSet"] = "";
	mStringMap["ScreenSaverBehavior"] = "dim";
	mStringMap["GamelistViewStyle"] = "automatic";
	mStringMap["DefaultGridSize"] = "";
	mBoolMap["ThemeRandom"] = false;
	mBoolMap["ThemeRandomSet"] = false;
	mStringMap["ThemeColorSet"] = "";
	mStringMap["ThemeIconSet"] = "";
	mStringMap["ThemeMenu"] = "";
	mStringMap["ThemeSystemView"] = "";
	mStringMap["ThemeGamelistView"] = "";
	mStringMap["ThemeRegionName"] = "eu";

	mBoolMap["ScreenSaverControls"] = true;
	mStringMap["ScreenSaverGameInfo"] = "never";
	mBoolMap["StretchVideoOnScreenSaver"] = false;
	mStringMap["PowerSaverMode"] = "disabled";
	mBoolMap["StopMusicOnScreenSaver"] = true;

	mIntMap["ScreenSaverSwapImageTimeout"] = 10000;
	mBoolMap["SlideshowScreenSaverStretch"] = false;
	mBoolMap["SlideshowScreenSaverCustomImageSource"] = false;
	mStringMap["SlideshowScreenSaverImageDir"] = Utils::FileSystem::getHomePath() + "/slideshow/image";
	mStringMap["SlideshowScreenSaverImageFilter"] = ".png,.jpg";
	mBoolMap["SlideshowScreenSaverRecurse"] = false;
	mBoolMap["SlideshowScreenSaverGameName"] = true;
	mIntMap["ScreenSaverSwapVideoTimeout"] = 30000;

	mBoolMap["ShowFilenames"] = false;
	mBoolMap["ShowFileBrowser"] = true;

	mBoolMap["VideoAudio"] = true;
	mBoolMap["VolumePopup"] = true;
	mBoolMap["VideoLowersMusic"] = true;
	mIntMap["MusicVolume"] = 128;
	mStringMap["CollectionSystemsAuto"] = "";
	mStringMap["CollectionSystemsCustom"] = "";
	mStringMap["HiddenSystems"] = "";
	mStringMap["SortSystems"] = ""; // "manufacturer" backward compatibility
	mStringMap["ForceFullNameSortSystems"] = false;
	mStringMap["SpecialAlphaSort"] = false;
	mBoolMap["UseCustomCollectionsSystem"] = true;
	mBoolMap["CollectionShowSystemInfo"] = true;
	mBoolMap["HiddenSystemsShowGames"] = true;
	mBoolMap["FavoritesFirst"] = true;

	mBoolMap["LocalArt"] = false;

	// Audio out device for volume control
	mStringMap["AudioDevice"] = "Master";
	mStringMap["AudioCard"] = "default";

	mStringMap["UIMode"] = "Full";
	mStringMap["UIMode_passkey"] = "uuddlrlrba";
	mBoolMap["ForceKiosk"] = false;
	mBoolMap["ForceKid"] = false;
	mBoolMap["ForceDisableFilters"] = false;

	mBoolMap["UseFullscreenPaging"] = false;

	mStringMap["Scraper"] = "ScreenScraper";
	mStringMap["ScrapperImageSrc"] = "ss";
	mStringMap["ScrapperThumbSrc"] = "box-2D";
	mStringMap["ScrapperLogoSrc"] = "wheel";
	mBoolMap["ScrapeVideos"] = false;

	// Audio settings
	mBoolMap["audio.bgmusic"] = true;
	mBoolMap["audio.persystem"] = false;
	mBoolMap["audio.display_titles"] = true;
	mBoolMap["audio.thememusics"] = true;
	mIntMap["audio.display_titles_time"] = 10;

	mStringMap["MusicDirectory"] = "/roms/bgmusic";
	mStringMap["UserMusicDirectory"] = "";

	mBoolMap["updates.enabled"] = false;
	mStringMap["wifi.ssid"] = "";
	mBoolMap["wifi.manual_dns"] = false;
	mStringMap["wifi.already.connection.exist.flag"] = " (**)";

	// Log settings
	mStringMap["LogLevel"] = "default";
	mBoolMap["LogWithMilliseconds"] = false;

	mBoolMap["AnimatedMainMenu"] = true;

	mBoolMap["ShowDetailedSystemInfo"] = false;

	mBoolMap["GuiEditMetadataCloseAllWindows"] = false;

	mBoolMap["wait.process.loading"] = false;

	mDefaultBoolMap = mBoolMap;
	mDefaultIntMap = mIntMap;
	mDefaultFloatMap = mFloatMap;
	mDefaultStringMap = mStringMap;
}

template <typename K, typename V>
void saveMap(pugi::xml_node& doc, std::map<K, V>& map, const char* type, std::map<K, V>& defaultMap, V defaultValue)
{
	for(auto iter = map.cbegin(); iter != map.cend(); iter++)
	{
		// key is on the "don't save" list, so don't save it
		if(std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), iter->first) != settings_dont_save.cend())
			continue;

		auto def = defaultMap.find(iter->first);
		if (def != defaultMap.cend() && def->second == iter->second)
			continue;

		if (def == defaultMap.cend() && iter->second == defaultValue)
			continue;

		pugi::xml_node node = doc.append_child(type);
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second);
	}
}

bool Settings::saveFile()
{
	if (!mWasChanged)
		return false;

	mWasChanged = false;

	//LOG(LogDebug) << "Settings::saveFile() : Saving Settings to file.";
	const std::string path = Utils::FileSystem::getEsConfigPath() + "/es_settings.cfg";

	std::cout << "Loading settings file path '" << path << "'...";

	pugi::xml_document doc;
	pugi::xml_node root = doc;

	if (mHasConfigRoot)
		root = doc.append_child("config"); // batocera, root element

	saveMap<std::string, bool>(root, mBoolMap, "bool", mDefaultBoolMap, false);
	saveMap<std::string, int>(root, mIntMap, "int", mDefaultIntMap, 0);
	saveMap<std::string, float>(root, mFloatMap, "float", mDefaultFloatMap, 0);

	//saveMap<std::string, std::string>(doc, mStringMap, "string");
	for(auto iter = mStringMap.cbegin(); iter != mStringMap.cend(); iter++)
	{
		// key is on the "don't save" list, so don't save it
		if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), iter->first) != settings_dont_save.cend())
			continue;

		auto def = mDefaultStringMap.find(iter->first);
		if (def == mDefaultStringMap.cend() && iter->second.empty())
			continue;

		if (def != mDefaultStringMap.cend() && def->second == iter->second)
			continue;

		pugi::xml_node node = root.append_child("string");
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second.c_str());
	}

	doc.save_file(path.c_str());

	Scripting::fireEvent("config-changed");
	Scripting::fireEvent("settings-changed");

	return true;
}

void Settings::loadFile()
{
	const std::string path = Utils::FileSystem::getEsConfigPath() + "/es_settings.cfg";

	if(!Utils::FileSystem::exists(path))
	{
		std::cerr << "Could not finde Settings file '" << path << "'!\n   ";
		return;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());
	if(!result)
	{
		//LOG(LogError) << "Could not parse Settings file!\n   " << result.description();
		std::cerr << "Could not parse Settings file '" << path << "'!\n   " << result.description();
		return;
	}

	pugi::xml_node root = doc;

	// Batocera has a <config> root element, learn reading them
	pugi::xml_node config = doc.child("config");
	if (config)
	{
		mHasConfigRoot = true;
		root = config;
	}

	for(pugi::xml_node node = root.child("bool"); node; node = node.next_sibling("bool"))
		setBool(node.attribute("name").as_string(), node.attribute("value").as_bool());
	for(pugi::xml_node node = root.child("int"); node; node = node.next_sibling("int"))
		setInt(node.attribute("name").as_string(), node.attribute("value").as_int());
	for(pugi::xml_node node = root.child("float"); node; node = node.next_sibling("float"))
		setFloat(node.attribute("name").as_string(), node.attribute("value").as_float());
	for(pugi::xml_node node = root.child("string"); node; node = node.next_sibling("string"))
		setString(node.attribute("name").as_string(), node.attribute("value").as_string());

	mWasChanged = false;
}

//Print a warning message if the setting we're trying to get doesn't already exist in the map, then return the value in the map.
#define SETTINGS_GETSET(type, mapName, getMethodName, setMethodName, defaultValue) \
type Settings::getMethodName(const std::string& name) \
{ \
	if(mapName.find(name) == mapName.cend()) \
	{ \
		/*std::cerr << "Tried to use unset setting " << name << "!";*/ \
		return defaultValue; \
	} \
	return mapName[name]; \
} \
bool Settings::setMethodName(const std::string& name, type value) \
{ \
	if (mapName.count(name) == 0 || mapName[name] != value) { \
		mapName[name] = value; \
\
		if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), name) == settings_dont_save.cend()) \
			mWasChanged = true; \
\
		return true; \
	} \
	return false; \
}

SETTINGS_GETSET(bool, mBoolMap, getBool, setBool, false);
SETTINGS_GETSET(int, mIntMap, getInt, setInt, 0);
SETTINGS_GETSET(float, mFloatMap, getFloat, setFloat, 0.0f);
//SETTINGS_GETSET(const std::string&, mStringMap, getString, setString, mEmptyString);

std::string Settings::getString(const std::string& name)
{
	if (mStringMap.find(name) == mStringMap.cend())
		return mEmptyString;

	return mStringMap[name];
}

bool Settings::setString(const std::string& name, const std::string& value)
{
	if (mStringMap.count(name) == 0 || mStringMap[name] != value)
	{
		if (value == "" && mStringMap.count(name) == 0)
			return false;

		mStringMap[name] = value;

		if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), name) == settings_dont_save.cend())
			mWasChanged = true;

		return true;
	}

	return false;
}
